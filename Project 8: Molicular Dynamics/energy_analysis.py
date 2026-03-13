import argparse
import os
from typing import Dict, Tuple

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np


def find_output_dir(project_dir: str, run_index: int | None) -> Tuple[str, int]:
    """Return (output_directory, run_index) for requested/latest run."""
    output_root = os.path.join(project_dir, "output")
    if not os.path.isdir(output_root):
        raise FileNotFoundError(f"Output root not found: {output_root}")

    run_dirs = sorted(
        [d for d in os.listdir(output_root) if d.startswith("out_")],
        key=lambda d: int(d.split("_")[1]),
    )
    if not run_dirs:
        raise FileNotFoundError(f"No run directories found in {output_root}")

    if run_index is None:
        run_name = run_dirs[-1]
        idx = int(run_name.split("_")[1])
    else:
        run_name = f"out_{run_index}"
        if run_name not in run_dirs:
            raise FileNotFoundError(
                f"Requested run '{run_name}' does not exist in {output_root}"
            )
        idx = run_index

    return os.path.join(output_root, run_name), idx


def _parse_metadata(metadata: np.ndarray) -> Dict[str, float | int | None]:
    """Handle both 5-value and legacy 4-value metadata formats."""
    md = np.asarray(metadata).flatten()

    if md.size >= 5:
        width = float(md[0])
        height = float(md[1])
        num_particles = int(md[2])
        time_steps = int(md[3])
        final_time = float(md[4])
    elif md.size == 4:
        width = float(md[0])
        height = None
        num_particles = int(md[1])
        time_steps = int(md[2])
        final_time = float(md[3])
    else:
        raise ValueError(f"Unexpected metadata format (size={md.size}): {md}")

    return {
        "width": width,
        "height": height,
        "num_particles": num_particles,
        "time_steps": time_steps,
        "final_time": final_time,
    }


def load_energy_data(run_dir: str, run_index: int) -> Tuple[Dict[str, np.ndarray], Dict[str, float | int | None]]:
    """Load energy and temperature arrays from modern .npy or legacy .npz outputs."""
    npy_temp = os.path.join(run_dir, "temperatures.npy")
    npz_path = os.path.join(run_dir, f"results_{run_index}.npz")

    if os.path.exists(npy_temp):
        data = {
            "temperatures": np.load(os.path.join(run_dir, "temperatures.npy")),
            "potential": np.load(os.path.join(run_dir, "potentialEnergies.npy")),
            "kinetic": np.load(os.path.join(run_dir, "kineticEnergies.npy")),
            "total": np.load(os.path.join(run_dir, "totalEnergies.npy")),
            "metadata": np.load(os.path.join(run_dir, "metadata.npy")),
        }
    elif os.path.exists(npz_path):
        legacy = np.load(npz_path)
        data = {
            "temperatures": legacy["temperatures"],
            "potential": legacy["potentialEnergies"],
            "kinetic": legacy["kineticEnergies"],
            "total": legacy["totalEnergies"],
            "metadata": legacy["metadata"],
        }
    else:
        raise FileNotFoundError(
            f"Could not find modern .npy files or legacy .npz file in {run_dir}"
        )

    meta = _parse_metadata(data["metadata"])
    return data, meta


def moving_average(x: np.ndarray, window: int) -> np.ndarray:
    if window <= 1:
        return x.copy()
    if window > len(x):
        window = len(x)
    kernel = np.ones(window, dtype=float) / float(window)
    return np.convolve(x, kernel, mode="same")


def compute_stats(total: np.ndarray, temperature: np.ndarray) -> Dict[str, float]:
    drift = total - total[0]
    denom = abs(total[0]) if abs(total[0]) > 1e-15 else 1.0
    rel_drift = drift / denom

    return {
        "energy_initial": float(total[0]),
        "energy_final": float(total[-1]),
        "energy_min": float(np.min(total)),
        "energy_max": float(np.max(total)),
        "energy_mean": float(np.mean(total)),
        "energy_std": float(np.std(total)),
        "abs_drift_final": float(drift[-1]),
        "abs_drift_max": float(np.max(np.abs(drift))),
        "rel_drift_final": float(rel_drift[-1]),
        "rel_drift_max": float(np.max(np.abs(rel_drift))),
        "temperature_min": float(np.min(temperature)),
        "temperature_max": float(np.max(temperature)),
        "temperature_mean": float(np.mean(temperature)),
        "temperature_std": float(np.std(temperature)),
    }


def save_stats_csv(path: str, stats: Dict[str, float]) -> None:
    with open(path, "w", encoding="utf-8") as f:
        f.write("metric,value\n")
        for key, value in stats.items():
            f.write(f"{key},{value:.12g}\n")


def make_plots(
    run_dir: str,
    run_index: int,
    time_axis: np.ndarray,
    potential: np.ndarray,
    kinetic: np.ndarray,
    total: np.ndarray,
    temperature: np.ndarray,
    smooth_window: int,
) -> Tuple[str, str]:
    drift = total - total[0]
    denom = abs(total[0]) if abs(total[0]) > 1e-15 else 1.0
    rel_drift = drift / denom

    total_s = moving_average(total, smooth_window)
    temp_s = moving_average(temperature, smooth_window)

    fig, axes = plt.subplots(2, 2, figsize=(14, 10), constrained_layout=True)

    ax = axes[0, 0]
    ax.plot(time_axis, potential, color="royalblue", linewidth=1.1, label="Potential")
    ax.plot(time_axis, kinetic, color="tomato", linewidth=1.1, label="Kinetic")
    ax.plot(time_axis, total, color="forestgreen", linewidth=1.2, linestyle="--", label="Total")
    if smooth_window > 1:
        ax.plot(time_axis, total_s, color="black", linewidth=1.2, alpha=0.75, label=f"Total MA({smooth_window})")
    ax.set_title("Energy Components vs Time")
    ax.set_ylabel("Energy (reduced units)")
    ax.grid(True, alpha=0.3)
    ax.legend()

    ax = axes[0, 1]
    ax.plot(time_axis, drift, color="purple", linewidth=1.0, label="Absolute Drift")
    ax.axhline(0.0, color="black", linewidth=0.8, linestyle="--")
    ax.set_title("Total Energy Drift")
    ax.set_ylabel("E(t) - E(0)")
    ax.grid(True, alpha=0.3)

    ax2 = ax.twinx()
    ax2.plot(time_axis, rel_drift, color="gray", linewidth=0.9, alpha=0.75, label="Relative Drift")
    ax2.set_ylabel("Relative Drift")

    handles1, labels1 = ax.get_legend_handles_labels()
    handles2, labels2 = ax2.get_legend_handles_labels()
    ax2.legend(handles1 + handles2, labels1 + labels2, loc="upper right")

    ax = axes[1, 0]
    ax.plot(time_axis, temperature, color="darkorange", linewidth=1.1, label="Temperature")
    if smooth_window > 1:
        ax.plot(time_axis, temp_s, color="black", linewidth=1.2, alpha=0.75, label=f"Temp MA({smooth_window})")
    ax.set_title("Temperature vs Time")
    ax.set_xlabel("Time")
    ax.set_ylabel("Temperature (reduced)")
    ax.grid(True, alpha=0.3)
    ax.legend()

    ax = axes[1, 1]
    scatter = ax.scatter(total, temperature, c=time_axis, cmap="viridis", s=8, alpha=0.75)
    ax.set_title("Temperature vs Total Energy")
    ax.set_xlabel("Total Energy")
    ax.set_ylabel("Temperature")
    ax.grid(True, alpha=0.3)
    cbar = fig.colorbar(scatter, ax=ax)
    cbar.set_label("Time")

    fig.suptitle(f"Molecular Dynamics Comprehensive Energy Analysis (run {run_index})", fontsize=14)

    plot_path = os.path.join(run_dir, f"energy_analysis_{run_index}.png")
    fig.savefig(plot_path, dpi=180)
    plt.close(fig)

    fig2, ax = plt.subplots(figsize=(10, 6))
    num_bins = min(80, max(20, len(total) // 250))
    bins = np.linspace(np.min(total), np.max(total), num_bins)
    centers = 0.5 * (bins[:-1] + bins[1:])
    idx = np.digitize(total, bins)
    mean_temp = np.full_like(centers, np.nan, dtype=float)

    for i in range(1, len(bins)):
        mask = idx == i
        if np.any(mask):
            mean_temp[i - 1] = np.mean(temperature[mask])

    ax.scatter(total, temperature, s=6, alpha=0.2, color="steelblue", label="Samples")
    ax.plot(centers, mean_temp, color="crimson", linewidth=2.0, label="Binned Mean T(E)")
    ax.set_title("Temperature over Energy (Detailed)")
    ax.set_xlabel("Total Energy")
    ax.set_ylabel("Temperature")
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig2.tight_layout()

    te_path = os.path.join(run_dir, f"temperature_over_energy_{run_index}.png")
    fig2.savefig(te_path, dpi=180)
    plt.close(fig2)

    return plot_path, te_path


def build_time_axis(n: int, final_time: float | None) -> np.ndarray:
    if final_time is not None and final_time > 0:
        return np.linspace(0.0, final_time, n)
    return np.arange(n, dtype=float)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Comprehensive energy analysis and plotting for molecular dynamics output."
    )
    parser.add_argument(
        "--run",
        type=int,
        default=None,
        help="Run index N for output/out_N (default: latest).",
    )
    parser.add_argument(
        "--smooth-window",
        type=int,
        default=301,
        help="Moving-average window for smoothed overlays (default: 301).",
    )
    args = parser.parse_args()

    project_dir = os.path.dirname(__file__)
    run_dir, run_index = find_output_dir(project_dir, args.run)
    data, meta = load_energy_data(run_dir, run_index)

    potential = np.asarray(data["potential"], dtype=float).flatten()
    kinetic = np.asarray(data["kinetic"], dtype=float).flatten()
    total = np.asarray(data["total"], dtype=float).flatten()
    temperature = np.asarray(data["temperatures"], dtype=float).flatten()

    n = min(len(potential), len(kinetic), len(total), len(temperature))
    if n < 2:
        raise ValueError("Not enough samples for energy analysis.")

    potential = potential[:n]
    kinetic = kinetic[:n]
    total = total[:n]
    temperature = temperature[:n]

    final_time = meta["final_time"] if isinstance(meta["final_time"], float) else None
    time_axis = build_time_axis(n, final_time)

    stats = compute_stats(total, temperature)
    stats_path = os.path.join(run_dir, f"energy_summary_{run_index}.csv")
    save_stats_csv(stats_path, stats)

    analysis_path, te_path = make_plots(
        run_dir=run_dir,
        run_index=run_index,
        time_axis=time_axis,
        potential=potential,
        kinetic=kinetic,
        total=total,
        temperature=temperature,
        smooth_window=max(1, args.smooth_window),
    )

    print(f"Loaded run: out_{run_index}")
    print(f"Samples analyzed: {n}")
    print(f"Particles: {meta['num_particles']}")
    print(f"Energy summary saved to: {stats_path}")
    print(f"Comprehensive analysis plot saved to: {analysis_path}")
    print(f"Temperature-over-energy plot saved to: {te_path}")


if __name__ == "__main__":
    main()
