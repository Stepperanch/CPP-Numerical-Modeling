#!/usr/bin/env python3
"""Build interactive Plotly HTML dashboards from sweep_results.npz.

The C++ solver has already normalized and resampled all psi trajectories,
so this script only handles loading, mirroring, plotting, and HTML output.

Key format from C++ output:
  psi_degree_{N}_state_{M}  -> psi half-trajectory for degree N, state M
  energies_degree_{N}       -> energy values for degree N

Outputs:
  eigenstates_tabs_simple.html   : tabbed 3D-only view (one tab per degree)
  eigenstates_tabs_detailed.html : tabbed view with 3D plot + well shape + data table
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


# Matches keys like: psi_degree_2_state_0
PSI_KEY_PATTERN = re.compile(r"^psi_degree_(?P<degree>\d+)_state_(?P<state>\d+)$")
ENERGY_KEY_PATTERN = re.compile(r"^energies_degree_(?P<degree>\d+)$")


@dataclass
class StateRecord:
    state: int      # State index (0, 1, 2, ...)
    energy: float   # Energy eigenvalue
    psi: np.ndarray # Half-trajectory (x=0 to x=xEnd), already normalized


def reconstruct_full_wavefunction(psi: np.ndarray, state: int) -> tuple[np.ndarray, np.ndarray]:
    """Mirror the half-trajectory to produce the full symmetric/antisymmetric wavefunction.

    Even states (state % 2 == 0) are symmetric:  psi(-x) =  psi(x)
    Odd  states (state % 2 == 1) are antisymmetric: psi(-x) = -psi(x)
    """
    psi = np.asarray(psi, dtype=float)
    if psi.size <= 1:
        return np.array([0.0]), psi

    x_half = np.linspace(0.0, 1.0, psi.size)
    x_left = -x_half[1:][::-1]

    if state % 2 == 0:
        psi_left = psi[1:][::-1]        # Symmetric
    else:
        psi_left = -psi[1:][::-1]       # Antisymmetric

    x_full = np.concatenate([x_left, x_half])
    psi_full = np.concatenate([psi_left, psi])
    return x_full, psi_full


def load_eigenstate_archive(npz_path: Path) -> Dict[int, List[StateRecord]]:
    """Load psi trajectories and energies from the C++ output npz file.

    Groups records by degree. Psi data is assumed already normalized and
    resampled by the C++ solver.
    """
    grouped: Dict[int, List[StateRecord]] = {}
    energies_by_degree: Dict[int, np.ndarray] = {}

    with np.load(npz_path) as archive:
        # First pass: load all energy arrays
        for key in archive.files:
            match = ENERGY_KEY_PATTERN.match(key)
            if match:
                degree = int(match.group("degree"))
                energies_by_degree[degree] = np.asarray(archive[key], dtype=float)

        # Second pass: load psi trajectories
        for key in archive.files:
            match = PSI_KEY_PATTERN.match(key)
            if match is None:
                continue

            degree = int(match.group("degree"))
            state  = int(match.group("state"))
            psi    = np.asarray(archive[key], dtype=float)

            if psi.size == 0:
                continue

            energy = float(energies_by_degree[degree][state]) if degree in energies_by_degree else 0.0

            grouped.setdefault(degree, []).append(StateRecord(state=state, energy=energy, psi=psi))

    # Sort each degree's states by state index
    for degree in grouped:
        grouped[degree].sort(key=lambda r: r.state)

    return grouped


def make_simple_3d(records: List[StateRecord], degree: int) -> go.Figure:
    """3D stacked wavefunction plot for a single degree."""
    fig = go.Figure()

    for rec in records:
        x_full, psi_full = reconstruct_full_wavefunction(rec.psi, rec.state)
        energy_level = np.full_like(x_full, rec.energy)

        fig.add_trace(go.Scatter3d(
            x=energy_level,
            y=x_full,
            z=psi_full,
            mode="lines",
            name=f"state {rec.state} (E={rec.energy:.4f})",
            hovertemplate=(
                f"degree={degree}<br>state={rec.state}<br>"
                "E=%{x:.5f}<br>x=%{y:.3f}<br>ψ=%{z:.5f}<extra></extra>"
            ),
        ))

        # Label at peak amplitude
        peak_idx = int(np.argmax(np.abs(psi_full)))
        fig.add_trace(go.Scatter3d(
            x=[rec.energy],
            y=[float(x_full[peak_idx])],
            z=[float(psi_full[peak_idx])],
            mode="text",
            text=[f"n={rec.state}"],
            textposition="top center",
            showlegend=False,
            hoverinfo="skip",
        ))

    fig.update_layout(
        title=f"Degree {degree} — V(x) = x^{degree}/{degree}: 3D Eigenstate Stack",
        height=880,
        scene=dict(
            xaxis_title="Energy",
            yaxis_title="x (normalized)",
            zaxis_title="ψ amplitude",
        ),
        margin=dict(l=10, r=10, t=45, b=10),
        template="plotly_white",
    )
    return fig


def make_detailed_figure(records: List[StateRecord], degree: int) -> go.Figure:
    """3D wavefunction stack + potential well shape + summary table."""
    fig = make_subplots(
        rows=2, cols=2,
        specs=[[{"type": "scene", "colspan": 2}, None],
               [{"type": "xy"}, {"type": "table"}]],
        row_heights=[0.72, 0.28],
        vertical_spacing=0.08,
        horizontal_spacing=0.06,
        subplot_titles=(
            f"Degree {degree} Eigenstate Wavefunctions",
            f"Potential V(x) = x^{degree}/{degree}",
            "State Summary",
        ),
    )

    states_col   = []
    energies_col = []
    npts_col     = []
    psi_min_col  = []
    psi_max_col  = []

    for rec in records:
        x_full, psi_full = reconstruct_full_wavefunction(rec.psi, rec.state)
        energy_level = np.full_like(x_full, rec.energy)

        fig.add_trace(go.Scatter3d(
            x=energy_level,
            y=x_full,
            z=psi_full,
            mode="lines",
            name=f"state {rec.state} (E={rec.energy:.4f})",
            hovertemplate=(
                f"degree={degree}<br>state={rec.state}<br>"
                "E=%{x:.5f}<br>x=%{y:.3f}<br>ψ=%{z:.5f}<extra></extra>"
            ),
        ), row=1, col=1)

        peak_idx = int(np.argmax(np.abs(psi_full)))
        fig.add_trace(go.Scatter3d(
            x=[rec.energy],
            y=[float(x_full[peak_idx])],
            z=[float(psi_full[peak_idx])],
            mode="text",
            text=[f"n={rec.state}"],
            textposition="top center",
            showlegend=False,
            hoverinfo="skip",
        ), row=1, col=1)

        states_col.append(str(rec.state))
        energies_col.append(f"{rec.energy:.6f}")
        npts_col.append(str(psi_full.size))
        psi_min_col.append(f"{float(np.min(psi_full)):.5g}")
        psi_max_col.append(f"{float(np.max(psi_full)):.5g}")

    # Potential well shape
    x_pot = np.linspace(0.0, 1.0, 500)
    v_pot = np.power(x_pot, degree) / float(degree)
    fig.add_trace(go.Scatter(
        x=x_pot, y=v_pot,
        mode="lines",
        line=dict(width=2),
        name=f"V(x)=x^{degree}/{degree}",
        hovertemplate="x=%{x:.3f}<br>V=%{y:.5f}<extra></extra>",
    ), row=2, col=1)

    # Summary table
    fig.add_trace(go.Table(
        header=dict(
            values=["state", "energy", "points", "ψ min", "ψ max"],
            fill_color="#0f766e",
            font=dict(color="white"),
        ),
        cells=dict(values=[states_col, energies_col, npts_col, psi_min_col, psi_max_col]),
    ), row=2, col=2)

    fig.update_layout(
        title=f"Degree {degree} — V(x) = x^{degree}/{degree}: Full Analysis",
        height=980,
        template="plotly_white",
        margin=dict(l=10, r=10, t=60, b=10),
        scene=dict(
            xaxis_title="Energy",
            yaxis_title="x (normalized)",
            zaxis_title="ψ amplitude",
        ),
        xaxis_title="x (normalized)",
        yaxis_title="V(x)",
    )
    return fig


def build_tabbed_html(figs_by_degree: Dict[int, go.Figure], output_path: Path, title: str) -> None:
    """Wrap multiple Plotly figures into a single tabbed HTML file."""
    sorted_degrees = sorted(figs_by_degree)
    if not sorted_degrees:
        raise ValueError("No data found to render.")

    tabs_html   = []
    panels_html = []

    for i, degree in enumerate(sorted_degrees):
        active = "active" if i == 0 else ""
        div_id = f"degree-tab-{degree}"
        tabs_html.append(
            f'<button class="tab-btn {active}" data-target="{div_id}">Degree {degree}</button>'
        )
        fig_html = pio.to_html(
            figs_by_degree[degree],
            full_html=False,
            include_plotlyjs="cdn",
            default_width="100%",
            default_height="88vh",
        )
        panels_html.append(
            f'<section id="{div_id}" class="tab-panel {active}">{fig_html}</section>'
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
    .sub {{ margin: 0 0 16px 0; color: #475569; font-size: 0.9rem; }}
    .tabs {{ display: flex; gap: 8px; flex-wrap: wrap; margin-bottom: 12px; }}
    .tab-btn {{ border: 1px solid #cbd5e1; background: #fff; border-radius: 8px; padding: 8px 14px; cursor: pointer; font-size: 0.95rem; transition: background 0.15s; }}
    .tab-btn:hover {{ background: #e0f2f1; }}
    .tab-btn.active {{ background: #0f766e; color: #fff; border-color: #0f766e; }}
    .tab-panel {{ display: none; background: #fff; border: 1px solid #e2e8f0; border-radius: 10px; padding: 6px; min-height: 88vh; }}
    .tab-panel.active {{ display: block; }}
  </style>
</head>
<body>
  <div class="wrap">
    <h1>{html.escape(title)}</h1>
    <p class="sub">
      Each tab shows eigenstates for a different potential degree V(x) = x^n/n.
      3D axes: energy eigenvalue, position x, and wavefunction amplitude ψ.
    </p>
    <div class="tabs">{''.join(tabs_html)}</div>
    {''.join(panels_html)}
  </div>
  <script>
    const buttons = Array.from(document.querySelectorAll('.tab-btn'));
    const panels  = Array.from(document.querySelectorAll('.tab-panel'));
    function activate(targetId) {{
      buttons.forEach(btn => btn.classList.toggle('active', btn.dataset.target === targetId));
      panels.forEach(p   => p.classList.toggle('active', p.id === targetId));
      if (window.Plotly) {{
        const panel = document.getElementById(targetId);
        panel.querySelectorAll('.plotly-graph-div').forEach(div => Plotly.Plots.resize(div));
      }}
    }}
    buttons.forEach(btn => btn.addEventListener('click', () => activate(btn.dataset.target)));
  </script>
</body>
</html>"""

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(html_doc, encoding="utf-8")
    print(f"Wrote: {output_path}")


def build_outputs(npz_path: Path, out_dir: Path) -> None:
    grouped = load_eigenstate_archive(npz_path)
    if not grouped:
        raise ValueError(f"No parseable eigenstate arrays found in {npz_path}")

    simple_figs:   Dict[int, go.Figure] = {}
    detailed_figs: Dict[int, go.Figure] = {}

    for degree, records in grouped.items():
        simple_figs[degree]   = make_simple_3d(records, degree)
        detailed_figs[degree] = make_detailed_figure(records, degree)

    build_tabbed_html(simple_figs,   out_dir / "eigenstates_tabs_simple.html",   "Quantum Wells: 3D Eigenstate Stack")
    build_tabbed_html(detailed_figs, out_dir / "eigenstates_tabs_detailed.html", "Quantum Wells: Full Analysis")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate tabbed Plotly dashboards from sweep_results.npz")
    parser.add_argument("--input",    type=Path, default=Path("sweep_results.npz"), help="Path to sweep_results.npz")
    parser.add_argument("--out-dir",  type=Path, default=None, dest="out_dir",      help="Output directory (defaults to input file directory)")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if not args.input.exists():
        raise FileNotFoundError(f"Input not found: {args.input}")
    out_dir = args.out_dir if args.out_dir is not None else args.input.parent
    build_outputs(args.input, out_dir)


if __name__ == "__main__":
    main()