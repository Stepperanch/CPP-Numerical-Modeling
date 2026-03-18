#!/usr/bin/env python3
"""Build interactive Plotly HTML dashboards from output/eigenstates.npz.

Outputs:
- eigenstates_tabs_simple.html: tabbed 3D-only view (one tab per well shape n)
- eigenstates_tabs_detailed.html: tabbed detailed view with 3D plot + well shape + data table
"""

from __future__ import annotations

import argparse
import html
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List

import numpy as np
import plotly.graph_objects as go
import plotly.io as pio
from plotly.subplots import make_subplots


KEY_PATTERNS = [
	re.compile(r"^n_(?P<n>\d+)_state_(?P<state>-?\d+)_idx_(?P<idx>\d+)$"),
	re.compile(r"^n_(?P<n>\d+)_state_(?P<state>-?\d+)$"),
]


@dataclass
class StateRecord:
	state: int
	idx: int
	psi: np.ndarray


def sanitize_half_trajectory(psi: np.ndarray, quantum_index: int) -> np.ndarray:
	psi = np.asarray(psi, dtype=float)
	if psi.size < 2:
		return psi

	aggressive = (quantum_index % 2 == 1)

	abs_psi = np.abs(psi)
	blowup_ratio = 50.0
	running_max = max(1e-14, float(abs_psi[0]))
	trim_start = psi.size

	for i in range(1, psi.size - 2):
		if abs_psi[i] > blowup_ratio * running_max and abs_psi[i + 2] > abs_psi[i + 1] > abs_psi[i]:
			trim_start = i
			break
		running_max = max(running_max, float(abs_psi[i]))

	# Also trim sustained monotonic growth suffixes (numerically unstable tails).
	if psi.size >= 10:
		growth_start = psi.size - 1
		while growth_start > 0 and abs_psi[growth_start] > abs_psi[growth_start - 1]:
			growth_start -= 1

		suffix_len = psi.size - growth_start
		if aggressive and suffix_len >= 8 and growth_start > psi.size // 3:
			prefix_max = float(np.max(abs_psi[:growth_start])) if growth_start > 0 else 0.0
			if prefix_max > 0.0 and float(abs_psi[-1]) > 2.0 * prefix_max:
				trim_start = min(trim_start, growth_start)

	# Envelope-minimum criterion to catch odd-state forbidden-region blow-up tails.
	start_window = psi.size // 4
	if aggressive and start_window + 8 < psi.size:
		segment = abs_psi[start_window:]
		min_idx_local = int(np.argmin(segment))
		min_idx = start_window + min_idx_local
		min_val = max(1e-15, float(abs_psi[min_idx]))

		if min_idx + 6 < psi.size and float(abs_psi[-1]) > 20.0 * min_val:
			for j in range(min_idx + 1, psi.size - 3):
				if (
					float(abs_psi[j]) > 5.0 * min_val
					and float(abs_psi[j + 1]) > float(abs_psi[j])
					and float(abs_psi[j + 2]) > float(abs_psi[j + 1])
					and float(abs_psi[j + 3]) > float(abs_psi[j + 2])
				):
					trim_start = min(trim_start, j)
					break

	# Long increasing-run criterion for pathological tails.
	run_length = 24
	run_start = psi.size
	run_count = 0
	for i in range(1, psi.size):
		if float(abs_psi[i]) > float(abs_psi[i - 1]):
			if run_count == 0:
				run_start = i - 1
			run_count += 1
			if aggressive and run_count >= run_length and run_start > psi.size // 6:
				trim_start = min(trim_start, run_start)
				break
		else:
			run_count = 0

	# For even states, only trim catastrophic tails.
	if not aggressive:
		prefix_max = float(np.max(abs_psi[:-1])) if psi.size > 1 else 0.0
		if prefix_max > 0.0 and float(abs_psi[-1]) <= 50.0 * prefix_max:
			trim_start = psi.size

	if trim_start >= 3 and trim_start < psi.size:
		psi = psi[:trim_start]

	if psi.size > 1:
		if hasattr(np, "trapezoid"):
			half_norm = np.trapezoid(psi * psi, dx=1.0)
		else:
			half_norm = np.trapz(psi * psi, dx=1.0)
		full_norm = 2.0 * half_norm
		if np.isfinite(full_norm) and full_norm > 0.0:
			psi = psi / np.sqrt(full_norm)

	return psi


def reconstruct_full_wavefunction(half_psi: np.ndarray, quantum_index: int) -> tuple[np.ndarray, np.ndarray]:
	half_psi = np.asarray(half_psi, dtype=float)
	if half_psi.size <= 1:
		return np.array([0.0]), half_psi

	x_half = np.linspace(0.0, 1.0, half_psi.size)
	x_left = -x_half[1:][::-1]

	if quantum_index % 2 == 0:
		psi_left = half_psi[1:][::-1]
	else:
		psi_left = -half_psi[1:][::-1]

	x_full = np.concatenate([x_left, x_half])
	psi_full = np.concatenate([psi_left, half_psi])
	return x_full, psi_full


def parse_key(key: str) -> tuple[int, int, int] | None:
	for pattern in KEY_PATTERNS:
		match = pattern.match(key)
		if match:
			n_val = int(match.group("n"))
			state_val = int(match.group("state"))
			idx_text = match.groupdict().get("idx")
			idx_val = int(idx_text) if idx_text is not None else 0
			return n_val, state_val, idx_val
	return None


def load_eigenstate_archive(npz_path: Path) -> Dict[int, List[StateRecord]]:
	grouped: Dict[int, List[StateRecord]] = {}

	with np.load(npz_path) as archive:
		for key in archive.files:
			parsed = parse_key(key)
			if parsed is None:
				continue

			n_val, state_val, idx_val = parsed
			psi = sanitize_half_trajectory(np.asarray(archive[key]).astype(float), state_val)
			if psi.size == 0:
				continue

			grouped.setdefault(n_val, []).append(StateRecord(state=state_val, idx=idx_val, psi=psi))

	for n_val in grouped:
		grouped[n_val].sort(key=lambda r: (r.state, r.idx))

	return grouped


def normalize_x(length: int) -> np.ndarray:
	if length <= 1:
		return np.array([0.0])
	return np.linspace(0.0, 1.0, length)


def make_simple_3d(records: List[StateRecord], n_val: int) -> go.Figure:
	fig = go.Figure()
	for rec in records:
		x_full, psi_full = reconstruct_full_wavefunction(rec.psi, rec.state)
		energy_level = np.full_like(x_full, float(rec.state))

		fig.add_trace(
			go.Scatter3d(
				x=energy_level,
				y=x_full,
				z=psi_full,
				mode="lines",
				name=f"state {rec.state}",
				hovertemplate=(
					"n=%d<br>state=%d<br>energy_level_index=%%{x:.0f}<br>x_full=%%{y:.3f}<br>psi=%%{z:.5f}<extra></extra>"
					% (n_val, rec.state)
				),
			)
		)

		peak_idx = int(np.argmax(np.abs(psi_full)))
		fig.add_trace(
			go.Scatter3d(
				x=[float(rec.state)],
				y=[float(x_full[peak_idx])],
				z=[float(psi_full[peak_idx])],
				mode="text",
				text=[f"q={rec.state}"],
				textposition="top center",
				showlegend=False,
				hoverinfo="skip",
			)
		)

	fig.update_layout(
		title=f"Well n={n_val}: 3D Wavefunction Stack",
		height=880,
		scene=dict(
			xaxis_title="Energy Level (quantum index)",
			yaxis_title="Wavefunction Shape (full mirrored x)",
			zaxis_title="Psi amplitude",
		),
		margin=dict(l=10, r=10, t=45, b=10),
		template="plotly_white",
	)
	return fig


def make_detailed_figure(records: List[StateRecord], n_val: int) -> go.Figure:
	fig = make_subplots(
		rows=2,
		cols=2,
		specs=[[{"type": "scene", "colspan": 2}, None], [{"type": "xy"}, {"type": "table"}]],
		row_heights=[0.72, 0.28],
		vertical_spacing=0.08,
		horizontal_spacing=0.06,
		subplot_titles=(
			"Labeled 3D Eigenstate Wavefunctions",
			"Normalized Potential Well Shape",
			"State Summary",
		),
	)

	states = []
	npts = []
	mins = []
	maxs = []
	l2_norms = []
	ho_energy = []

	for rec in records:
		x_full, psi_full = reconstruct_full_wavefunction(rec.psi, rec.state)
		energy_level = np.full_like(x_full, float(rec.state))

		fig.add_trace(
			go.Scatter3d(
				x=energy_level,
				y=x_full,
				z=psi_full,
				mode="lines",
				name=f"state {rec.state}",
				hovertemplate=(
					"n=%d<br>state=%d<br>energy_level_index=%%{x:.0f}<br>x_full=%%{y:.3f}<br>psi=%%{z:.5f}<extra></extra>"
					% (n_val, rec.state)
				),
			),
			row=1,
			col=1,
		)

		peak_idx = int(np.argmax(np.abs(psi_full)))
		fig.add_trace(
			go.Scatter3d(
				x=[float(rec.state)],
				y=[float(x_full[peak_idx])],
				z=[float(psi_full[peak_idx])],
				mode="text",
				text=[f"q={rec.state}"],
				textposition="top center",
				showlegend=False,
				hoverinfo="skip",
			),
			row=1,
			col=1,
		)

		states.append(str(rec.state))
		npts.append(str(psi_full.size))
		mins.append(f"{float(np.min(psi_full)):.5g}")
		maxs.append(f"{float(np.max(psi_full)):.5g}")
		l2_norms.append(f"{float(np.linalg.norm(psi_full)):.5g}")
		ho_energy.append(f"{rec.state + 0.5:.6f}" if n_val == 2 else "-")

	x_pot = np.linspace(0.0, 1.0, 500)
	v_pot = np.power(x_pot, n_val) / float(n_val)
	fig.add_trace(
		go.Scatter(
			x=x_pot,
			y=v_pot,
			mode="lines",
			line=dict(width=2),
			name="V(x)=x^n/n (normalized x)",
			hovertemplate="x_norm=%{x:.3f}<br>V=%{y:.5f}<extra></extra>",
		),
		row=2,
		col=1,
	)

	fig.add_trace(
		go.Table(
			header=dict(values=["state", "points", "psi min", "psi max", "||psi||2", "HO E=(q+1/2)*w"]),
			cells=dict(values=[states, npts, mins, maxs, l2_norms, ho_energy]),
		),
		row=2,
		col=2,
	)

	fig.update_layout(
		title=f"Well n={n_val}: 3D + Well Shape + State Data",
		height=980,
		template="plotly_white",
		margin=dict(l=10, r=10, t=60, b=10),
		scene=dict(
			xaxis_title="Energy Level (quantum index)",
			yaxis_title="Wavefunction Shape (full mirrored x)",
			zaxis_title="Psi amplitude",
		),
		xaxis_title="x normalized",
		yaxis_title="Potential V(x)",
	)

	return fig


def build_tabbed_html(figs_by_n: Dict[int, go.Figure], output_path: Path, title: str) -> None:
	sorted_n = sorted(figs_by_n)
	if not sorted_n:
		raise ValueError("No well data found to render.")

	tabs_html = []
	panels_html = []
	for i, n_val in enumerate(sorted_n):
		active = "active" if i == 0 else ""
		div_id = f"well-tab-{n_val}"
		tabs_html.append(f'<button class="tab-btn {active}" data-target="{div_id}">Well n={n_val}</button>')
		fig_html = pio.to_html(
			figs_by_n[n_val],
			full_html=False,
			include_plotlyjs="cdn",
			default_width="100%",
			default_height="88vh",
		)
		panels_html.append(
			f'<section id="{div_id}" class="tab-panel {active}">' + fig_html + "</section>"
		)

	html_doc = f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>{html.escape(title)}</title>
  <style>
	body {{ font-family: 'Segoe UI', Tahoma, sans-serif; margin: 0; background: #f7fafc; color: #1f2937; }}
	.wrap {{ width: calc(100vw - 24px); max-width: none; margin: 0 auto; padding: 12px; }}
	h1 {{ margin: 0 0 8px 0; font-size: 1.4rem; }}
	.sub {{ margin: 0 0 16px 0; color: #475569; }}
	.tabs {{ display: flex; gap: 8px; flex-wrap: wrap; margin-bottom: 12px; }}
	.tab-btn {{ border: 1px solid #cbd5e1; background: #fff; border-radius: 8px; padding: 8px 12px; cursor: pointer; }}
	.tab-btn.active {{ background: #0f766e; color: #fff; border-color: #0f766e; }}
	.tab-panel {{ display: none; background: #fff; border: 1px solid #e2e8f0; border-radius: 10px; padding: 6px; min-height: 88vh; }}
	.tab-panel.active {{ display: block; }}
  </style>
</head>
<body>
  <div class="wrap">
	<h1>{html.escape(title)}</h1>
	<p class="sub">Tabs represent different well shapes (n). 3D axes: energy level index, wavefunction shape, and amplitude.</p>
	<div class="tabs">{''.join(tabs_html)}</div>
	{''.join(panels_html)}
  </div>
  <script>
	const buttons = Array.from(document.querySelectorAll('.tab-btn'));
	const panels = Array.from(document.querySelectorAll('.tab-panel'));
	function activate(targetId) {{
	  buttons.forEach(btn => btn.classList.toggle('active', btn.dataset.target === targetId));
	  panels.forEach(p => p.classList.toggle('active', p.id === targetId));
	  if (window.Plotly) {{
		const activePanel = document.getElementById(targetId);
		activePanel.querySelectorAll('.plotly-graph-div').forEach(div => Plotly.Plots.resize(div));
	  }}
	}}
	buttons.forEach(btn => btn.addEventListener('click', () => activate(btn.dataset.target)));
  </script>
</body>
</html>
"""

	output_path.parent.mkdir(parents=True, exist_ok=True)
	output_path.write_text(html_doc, encoding="utf-8")


def build_outputs(npz_path: Path, out_dir: Path) -> tuple[Path, Path]:
	grouped = load_eigenstate_archive(npz_path)
	if not grouped:
		raise ValueError(f"No parseable eigenstate arrays found in {npz_path}")

	simple_figs: Dict[int, go.Figure] = {}
	detailed_figs: Dict[int, go.Figure] = {}
	for n_val, records in grouped.items():
		simple_figs[n_val] = make_simple_3d(records, n_val)
		detailed_figs[n_val] = make_detailed_figure(records, n_val)

	simple_out = out_dir / "eigenstates_tabs_simple.html"
	detailed_out = out_dir / "eigenstates_tabs_detailed.html"

	build_tabbed_html(simple_figs, simple_out, "Quantum Wells: Labeled 3D Wavefunction Tabs")
	build_tabbed_html(detailed_figs, detailed_out, "Quantum Wells: Detailed 3D + Well Shape + State Data Tabs")

	return simple_out, detailed_out


def parse_args() -> argparse.Namespace:
	parser = argparse.ArgumentParser(description="Generate tabbed Plotly HTML dashboards from eigenstates.npz")
	parser.add_argument("--input", type=Path, default=Path("output/eigenstates.npz"), help="Path to eigenstates.npz")
	parser.add_argument(
		"--out-dir",
		"--output",
		dest="out_dir",
		type=Path,
		default=None,
		help="Directory for generated HTML files (defaults to the same directory as --input)",
	)
	return parser.parse_args()


def main() -> None:
	args = parse_args()
	input_path = args.input
	if not input_path.exists():
		raise FileNotFoundError(f"Input archive not found: {input_path}")

	out_dir = args.out_dir if args.out_dir is not None else input_path.parent

	simple_out, detailed_out = build_outputs(input_path, out_dir)
	print(f"Wrote: {simple_out}")
	print(f"Wrote: {detailed_out}")


if __name__ == "__main__":
	main()
