#!/usr/bin/env python3
"""Build interactive Plotly HTML dashboards from output/eigenstates.npz.

Outputs:
- eigenstates_tabs_simple.html: tabbed 3D-only view (one tab per well shape n)
- eigenstates_tabs_detailed.html: tabbed detailed view with 3D plot + well shape + data table
"""

from __future__ import annotations

import argparse
import base64
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
	re.compile(r"^psi_degree_(?P<n>\d+)_state_(?P<state>\d+)$"),
]

ENERGY_PATTERN = re.compile(r"^energies_degree_(?P<n>\d+)$")


@dataclass
class StateRecord:
	state: int
	energy: float
	psi: np.ndarray


def reconstruct_full_wavefunction(half_psi: np.ndarray, quantum_index: int, target_x_end: float) -> tuple[np.ndarray, np.ndarray]:
	half_psi = np.asarray(half_psi, dtype=float)
	if half_psi.size <= 1:
		return np.array([0.0]), half_psi

	x_half = np.linspace(0.0, target_x_end, half_psi.size)
	x_left = -x_half[1:][::-1]

	if quantum_index % 2 == 0:
		psi_left = half_psi[1:][::-1]
	else:
		psi_left = -half_psi[1:][::-1]

	x_full = np.concatenate([x_left, x_half])
	psi_full = np.concatenate([psi_left, half_psi])
	return x_full, psi_full


def parse_key(key: str) -> tuple[int, int] | None:
	for pattern in KEY_PATTERNS:
		match = pattern.match(key)
		if match:
			n_val = int(match.group("n"))
			state_val = int(match.group("state"))
			return n_val, state_val
	return None


def load_eigenstate_archive(npz_path: Path) -> tuple[Dict[int, List[StateRecord]], float]:
	grouped: Dict[int, List[StateRecord]] = {}
	energies_by_n: Dict[int, np.ndarray] = {}
	target_x_end = 8.0  # Default value

	with np.load(npz_path) as archive:
		# Load targetXEnd if available
		if "targetXEnd" in archive.files:
			target_x_end = float(archive["targetXEnd"][0])

		# Pass 1: collect energies arrays by degree.
		for key in archive.files:
			match = ENERGY_PATTERN.match(key)
			if not match:
				continue

			n_val = int(match.group("n"))
			energies_by_n[n_val] = np.asarray(archive[key], dtype=float).ravel()

		# Pass 2: collect psi trajectories and attach their physical eigenvalue.
		for key in archive.files:
			parsed = parse_key(key)
			if parsed is None:
				continue

			n_val, state_val = parsed
			psi = np.asarray(archive[key], dtype=float)
			if psi.size == 0:
				continue

			n_energies = energies_by_n.get(n_val)
			energy_val = np.nan
			if n_energies is not None and 0 <= state_val < n_energies.size:
				energy_val = float(n_energies[state_val])

			grouped.setdefault(n_val, []).append(StateRecord(state=state_val, energy=energy_val, psi=psi))

	for n_val in grouped:
		grouped[n_val].sort(key=lambda r: r.state)

	return grouped, target_x_end


def normalize_x(length: int) -> np.ndarray:
	if length <= 1:
		return np.array([0.0])
	return np.linspace(0.0, 1.0, length)


def make_single_state_2d(rec: StateRecord, n_val: int, target_x_end: float) -> go.Figure:
	"""Create a 2D plot for a single eigenstate."""
	x_full, psi_full = reconstruct_full_wavefunction(rec.psi, rec.state, target_x_end)

	fig = go.Figure()

	# Wavefunction curve
	fig.add_trace(
		go.Scatter(
			x=x_full,
			y=psi_full,
			mode="lines",
			name="ψ(x)",
			line=dict(color="#0f766e", width=2.5),
			hovertemplate="x=%{x:.3f}<br>ψ=%{y:.5f}<extra></extra>",
		)
	)

	# Energy level line
	fig.add_hline(
		y=0,
		line_dash="dash",
		line_color="rgba(0,0,0,0.3)",
		annotation_text=f"E={rec.energy:.3g}",
		annotation_position="right",
	)

	# Potential well backdrop (faint)
	x_pot = np.linspace(-target_x_end, target_x_end, 200)
	v_pot = np.abs(x_pot) ** n_val / float(n_val)
	v_pot_scaled = v_pot / np.max(v_pot) * np.max(np.abs(psi_full)) * 0.3 if np.max(psi_full) > 0 else v_pot
	fig.add_trace(
		go.Scatter(
			x=x_pot,
			y=v_pot_scaled,
			mode="lines",
			name=f"V(x)=|x|^{n_val}/{n_val}",
			line=dict(color="rgba(200,100,100,0.4)", width=1.5),
			fill="tozeroy",
			fillcolor="rgba(200,100,100,0.1)",
			hovertemplate="x=%{x:.3f}<br>V=%{y:.5f}<extra></extra>",
		)
	)

	fig.update_layout(
		title=f"Well n={n_val}, State {rec.state} – E={rec.energy:.6g}",
		xaxis_title="Position x",
		yaxis_title="Wavefunction ψ(x)",
		height=600,
		template="plotly_white",
		margin=dict(l=60, r=60, t=60, b=50),
		hovermode="x unified",
		xaxis=dict(range=[-target_x_end, target_x_end]),
	)

	return fig


def make_simple_3d(records: List[StateRecord], n_val: int, target_x_end: float) -> go.Figure:
	fig = go.Figure()
	for rec in records:
		x_full, psi_full = reconstruct_full_wavefunction(rec.psi, rec.state, target_x_end)
		energy_level = np.full_like(x_full, float(rec.energy))

		fig.add_trace(
			go.Scatter3d(
				x=energy_level,
				y=x_full,
				z=psi_full,
				mode="lines",
				name=f"state {rec.state} E={rec.energy:.3g}",
				hovertemplate=(
					"n=%d<br>state=%d<br>energy=%%{x:.6g}<br>x=%%{y:.3f}<br>psi=%%{z:.5f}<extra></extra>"
					% (n_val, rec.state)
				),
			)
		)

		peak_idx = int(np.argmax(np.abs(psi_full)))
		fig.add_trace(
			go.Scatter3d(
				x=[float(rec.energy)],
				y=[float(x_full[peak_idx])],
				z=[float(psi_full[peak_idx])],
				mode="text",
				text=[f"q={rec.state}, E={rec.energy:.3g}"],
				textposition="top center",
				showlegend=False,
				hoverinfo="skip",
			)
		)

	fig.update_layout(
		title=f"Well n={n_val}: 3D Wavefunction Stack",
		height=None,
		scene=dict(
			xaxis_title="Energy eigenvalue",
			yaxis_title="Position x",
			zaxis_title="Wavefunction ψ(x)",
		),
		margin=dict(l=10, r=10, t=45, b=60),
		template="plotly_white",
		annotations=[
			dict(
				text=f"<b>Potential Well</b><br>V(x) = |x|<sup>{n_val}</sup>/{n_val}",
				xref="paper",
				yref="paper",
				x=0.5,
				y=-0.12,
				showarrow=False,
				font=dict(size=11),
				bgcolor="rgba(240,240,240,0.8)",
				bordercolor="lightgray",
				borderwidth=1,
				borderpad=8,
			)
		],
	)
	return fig


def make_detailed_figure(records: List[StateRecord], n_val: int, target_x_end: float) -> go.Figure:
	fig = make_subplots(
		rows=1,
		cols=2,
		specs=[[{"type": "scene"}, {"type": "xy"}]],
		column_widths=[0.75, 0.25],
		horizontal_spacing=0.06,
		subplot_titles=(
			"Labeled 3D Eigenstate Wavefunctions",
			"Normalized Potential Well Shape",
		),
	)

	pass  # Removed summary table data collection

	for rec in records:
		x_full, psi_full = reconstruct_full_wavefunction(rec.psi, rec.state, target_x_end)
		energy_level = np.full_like(x_full, float(rec.energy))

		fig.add_trace(
			go.Scatter3d(
				x=energy_level,
				y=x_full,
				z=psi_full,
				mode="lines",
				name=f"state {rec.state} E={rec.energy:.3g}",
				hovertemplate=(
					"n=%d<br>state=%d<br>energy=%%{x:.6g}<br>x=%%{y:.3f}<br>psi=%%{z:.5f}<extra></extra>"
					% (n_val, rec.state)
				),
			),
			row=1,
			col=1,
		)

		peak_idx = int(np.argmax(np.abs(psi_full)))
		fig.add_trace(
			go.Scatter3d(
				x=[float(rec.energy)],
				y=[float(x_full[peak_idx])],
				z=[float(psi_full[peak_idx])],
				mode="text",
				text=[f"q={rec.state}, E={rec.energy:.3g}"],
				textposition="top center",
				showlegend=False,
				hoverinfo="skip",
			),
			row=1,
			col=1,
		)

	x_pot = np.linspace(-4.0, 4.0, 500)
	v_pot = np.abs(x_pot) ** n_val / float(n_val)
	fig.add_trace(
		go.Scatter(
			x=x_pot,
			y=v_pot,
			mode="lines",
			line=dict(width=2),
			name=f"V(x)=|x|^{n_val}/{n_val}",
			hovertemplate="x=%{x:.3f}<br>V=%{y:.5f}<extra></extra>",
		),
		row=1,
		col=2,
	)



	fig.update_layout(
		title=f"Well n={n_val}: 3D + Well Shape",
		height=700,
		template="plotly_white",
		margin=dict(l=10, r=10, t=60, b=10),
	)
	fig.update_scenes(
		xaxis_title="Energy eigenvalue",
		yaxis_title="Position x",
		zaxis_title="Wavefunction ψ(x)",
	)
	fig.update_xaxes(showticklabels=False, row=1, col=2)
	fig.update_yaxes(title_text="Potential V(x)", row=1, col=2)

	return fig


def build_tabbed_html_with_states(figs_by_n: Dict[int, go.Figure], records_by_n: Dict[int, List[StateRecord]], output_path: Path, title: str, target_x_end: float = 8.0) -> None:
	"""Build tabbed HTML with sub-tabs for 3D all-states and 2D single-state views."""
	sorted_n = sorted(figs_by_n)
	if not sorted_n:
		raise ValueError("No well data found to render.")

	# Generate 2D plots for each state
	figs_2d_by_n_state: Dict[tuple, go.Figure] = {}
	for n_val in sorted_n:
		records = records_by_n.get(n_val, [])
		for rec in records:
			figs_2d_by_n_state[(n_val, rec.state)] = make_single_state_2d(rec, n_val, target_x_end)

	tabs_html = []
	panels_html = []
	for i, n_val in enumerate(sorted_n):
		active = "active" if i == 0 else ""
		div_id = f"well-tab-{n_val}"
		tabs_html.append(f'<button class="tab-btn {active}" data-target="{div_id}">Well n={n_val}</button>')

		records = records_by_n.get(n_val, [])
		state_list = [rec.state for rec in records]

		# Generate 3D HTML
		fig_html_3d = pio.to_html(
			figs_by_n[n_val],
			full_html=False,
			include_plotlyjs=False,
			default_width="100%",
			default_height="88vh",
		)

		# Generate 2D plot selector and HTML
		state_options = "".join([f'<option value="{s}">State {s} (E={records[i].energy:.3g})</option>' for i, s in enumerate(state_list)])

		figs_2d_html_str = "{" + ", ".join([f'"{s}": `{pio.to_html(figs_2d_by_n_state[(n_val, s)], full_html=False, include_plotlyjs=False, default_width="100%", default_height="88vh")}`' for s in state_list]) + "}"

		# Build sub-tabs HTML
		subtabs_html = f"""<div class="subtabs">
	<button class="subtab-btn active" data-subtarget="{div_id}-3d">All States (3D)</button>
	<button class="subtab-btn" data-subtarget="{div_id}-2d">Single State (2D)</button>
</div>
<section id="{div_id}-3d" class="subtab-panel active">
	<div class="plot-wrapper">
		{fig_html_3d}
	</div>
</section>
<section id="{div_id}-2d" class="subtab-panel">
	<div class="state-selector-container">
		<label for="state-selector-{n_val}">Select State:</label>
		<select id="state-selector-{n_val}" class="state-selector" data-well="{n_val}">
			{state_options}
		</select>
	</div>
	<div class="plot-wrapper">
		<div id="plot-container-{n_val}"></div>
	</div>
</section>"""

		panels_html.append(
			f'<section id="{div_id}" class="tab-panel {active}">{subtabs_html}</section>'
		)

		# Store 2D plots data
		plots_json = ", ".join([f'state_{s}: {repr(pio.to_json(figs_2d_by_n_state[(n_val, s)]))}' for s in state_list])

	html_doc = f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>{html.escape(title)}</title>
  <script src="https://cdn.plot.ly/plotly-2.35.2.min.js"></script>
  <style>
	* {{ box-sizing: border-box; }}
	html, body {{ height: 100%; margin: 0; padding: 0; }}
	body {{ font-family: 'Segoe UI', Tahoma, sans-serif; background: #f7fafc; color: #1f2937; display: flex; flex-direction: column; }}
	.wrap {{ flex: 1; width: 100%; padding: 12px; overflow-y: auto; display: flex; flex-direction: column; }}
	h1 {{ margin: 0 0 8px 0; font-size: 1.4rem; }}
	.sub {{ margin: 0 0 16px 0; color: #475569; }}
	.tabs {{ display: flex; gap: 8px; flex-wrap: wrap; margin-bottom: 12px; }}
	.tab-btn {{ border: 1px solid #cbd5e1; background: #fff; border-radius: 8px; padding: 8px 12px; cursor: pointer; transition: all 0.2s; }}
	.tab-btn:hover {{ background: #f0fdfa; }}
	.tab-btn.active {{ background: #0f766e; color: #fff; border-color: #0f766e; }}
	.tab-panel {{ display: none; background: #fff; border: 1px solid #e2e8f0; border-radius: 10px; padding: 12px; flex: 1; flex-direction: column; min-height: 0; }}
	.tab-panel.active {{ display: flex; flex-direction: column; }}
	.subtabs {{ display: flex; gap: 6px; margin-bottom: 12px; padding: 12px; background: #f0fdfa; border-radius: 8px; flex-wrap: wrap; }}
	.subtab-btn {{ border: 1px solid #cbd5e1; background: #fff; border-radius: 6px; padding: 8px 16px; cursor: pointer; font-size: 14px; transition: all 0.2s; font-weight: 500; }}
	.subtab-btn:hover {{ background: #e0fffc; }}
	.subtab-btn.active {{ background: #0f766e; color: #fff; border-color: #0f766e; }}
	.subtab-panel {{ display: none; flex-direction: column; flex: 1; min-height: 0; }}
	.subtab-panel.active {{ display: flex; flex-direction: column; }}
	.state-selector-container {{ padding: 12px; background: #f7fafc; border-radius: 8px; margin-bottom: 12px; display: flex; align-items: center; gap: 12px; flex-wrap: wrap; }}
	.state-selector {{ padding: 8px 12px; border: 1px solid #cbd5e1; border-radius: 6px; font-size: 14px; cursor: pointer; }}
	.plot-wrapper {{ flex: 1; min-height: 0; width: 100%; position: relative; display: flex; }}
	.plotly-graph-div {{ width: 100% !important; height: 100% !important; flex: 1; }}
  </style>
</head>
<body>
  <div class="wrap">
	<h1>{html.escape(title)}</h1>
	<p class="sub">Tabs represent different well shapes (n). Under each well, switch between 3D all-states view and 2D single-state view.</p>
	<div class="tabs">{''.join(tabs_html)}</div>
	<div style="flex: 1; min-height: 0; display: flex; flex-direction: column;">
	{''.join(panels_html)}
	</div>
  </div>
  <script>
	// Store all 2D plot data by well and state
	const plotsData = {{}};
	"""

	# Add all 2D plot data - use btoa for safe encoding
	for n_val in sorted_n:
		records = records_by_n.get(n_val, [])
		html_doc += f"\n\tplotsData[{n_val}] = {{}};\n"
		for rec in records:
			plot_json = pio.to_json(figs_2d_by_n_state[(n_val, rec.state)])
			# Encode as base64 to safely embed in HTML
			if plot_json is not None:
				encoded = base64.b64encode(plot_json.encode()).decode()
				html_doc += f'\tplotsData[{n_val}][{rec.state}] = "{encoded}";\n'

	html_doc += """
	// Decode base64 plot data
	function getPlotData(well, state) {
	  const encoded = plotsData[well][state];
	  if (!encoded) return null;
	  try {
		const jsonStr = atob(encoded);
		return JSON.parse(jsonStr);
	  } catch(e) {
		console.error('Error decoding plot data:', e);
		return null;
	  }
	}

	// Main tab switching
	const tabButtons = Array.from(document.querySelectorAll('.tab-btn'));
	const tabPanels = Array.from(document.querySelectorAll('.tab-panel'));

	function activateTab(targetId) {
	  tabButtons.forEach(btn => btn.classList.toggle('active', btn.dataset.target === targetId));
	  tabPanels.forEach(p => p.classList.toggle('active', p.id === targetId));
	  setTimeout(() => {
		if (window.Plotly) {
		  const activePanel = document.getElementById(targetId);
		  if (activePanel) {
			activePanel.querySelectorAll('.plotly-graph-div').forEach(div => {
			  try { Plotly.Plots.resize(div); } catch(e) {}
			});
		  }
		}
	  }, 100);
	}

	tabButtons.forEach(btn => btn.addEventListener('click', () => activateTab(btn.dataset.target)));

	// Sub-tab switching
	const subtabButtons = Array.from(document.querySelectorAll('.subtab-btn'));
	subtabButtons.forEach(btn => {
	  btn.addEventListener('click', function(e) {
		e.preventDefault();
		const targetId = this.dataset.subtarget;
		const wellPanel = this.closest('.tab-panel');
		if (!wellPanel) return;

		const subtabs = wellPanel.querySelectorAll('.subtab-btn');
		const subtabPanels = wellPanel.querySelectorAll('.subtab-panel');

		subtabs.forEach(b => b.classList.toggle('active', b.dataset.subtarget === targetId));
		subtabPanels.forEach(p => p.classList.toggle('active', p.id === targetId));

		setTimeout(() => {
		  if (window.Plotly) {
			const activeSubpanel = wellPanel.querySelector(`#${targetId}`);
			if (activeSubpanel) {
			  activeSubpanel.querySelectorAll('.plotly-graph-div').forEach(div => {
				try { Plotly.Plots.resize(div); } catch(e) {}
			  });
			}
		  }
		}, 100);
	  });
	});

	// State selector dropdown
	const selectors = Array.from(document.querySelectorAll('.state-selector'));
	selectors.forEach(selector => {
	  function updatePlot() {
		const well = parseInt(selector.dataset.well);
		const state = parseInt(selector.value);
		const container = document.getElementById(`plot-container-${well}`);

		if (container) {
		  const plotData = getPlotData(well, state);
		  if (plotData) {
			try {
			  Plotly.newPlot(container, plotData.data, plotData.layout, {responsive: true});
			  setTimeout(() => Plotly.Plots.resize(container), 100);
			} catch(e) {
			  console.error('Error rendering plot:', e);
			}
		  }
		}
	  }

	  selector.addEventListener('change', updatePlot);

	  // Trigger initial plot on first load
	  setTimeout(updatePlot, 300);
	});

	// Handle window resize
	window.addEventListener('resize', () => {
	  if (window.Plotly) {
		document.querySelectorAll('.plotly-graph-div').forEach(div => {
		  try { Plotly.Plots.resize(div); } catch(e) {}
		});
	  }
	});

	// Also trigger resize on page load to ensure proper initial sizing
	window.addEventListener('load', () => {
	  setTimeout(() => {
		if (window.Plotly) {
		  document.querySelectorAll('.plotly-graph-div').forEach(div => {
			try { Plotly.Plots.resize(div); } catch(e) {}
		  });
		}
	  }, 200);
	});
  </script>
</body>
</html>
"""

	output_path.parent.mkdir(parents=True, exist_ok=True)
	output_path.write_text(html_doc, encoding="utf-8")


def build_outputs(npz_path: Path, out_dir: Path) -> tuple[Path, Path]:
	grouped, target_x_end = load_eigenstate_archive(npz_path)
	if not grouped:
		raise ValueError(f"No parseable eigenstate arrays found in {npz_path}")

	simple_figs: Dict[int, go.Figure] = {}
	detailed_figs: Dict[int, go.Figure] = {}
	for n_val, records in grouped.items():
		simple_figs[n_val] = make_simple_3d(records, n_val, target_x_end)
		detailed_figs[n_val] = make_detailed_figure(records, n_val, target_x_end)

	simple_out = out_dir / "eigenstates_tabs_simple.html"
	detailed_out = out_dir / "eigenstates_tabs_detailed.html"

	build_tabbed_html_with_states(simple_figs, grouped, simple_out, "Quantum Wells: 3D Wavefunction Stack with 2D Single-State View", target_x_end)
	build_tabbed_html_with_states(detailed_figs, grouped, detailed_out, "Quantum Wells: Detailed 3D + Well Shape with 2D Single-State View", target_x_end)

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