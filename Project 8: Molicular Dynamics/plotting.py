import numpy as np
import matplotlib
matplotlib.use("Agg")  # non-interactive backend, required for multiprocessing
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import os
import subprocess
import tempfile
import multiprocessing as mp

"""
File Header:

Author: Nels Buhrley
Date: 2026-03-02

Description:
    Animation and energy plots for Project 8: Molecular Dynamics.
    Loads the most recent output directory (./output/out_N/results.npz).
    Positions array shape: (timeSteps, numParticles, 2)
    Metadata array: [L, numParticles, timeSteps, finalTime]
    Animation is rendered in parallel segments and stitched with ffmpeg.
"""

FFMPEG = "/apps/spack/root/opt/spack/linux-rhel9-haswell/gcc-13.2.0/ffmpeg-7.0.1-pzg5pllmqfjzz2ubrlm3jcxyyh7gtpyr/bin/ffmpeg"
plt.rcParams["animation.ffmpeg_path"] = FFMPEG

# ── Animation controls ────────────────────────────────────────────────────────
FPS         = 30    # frames per second
STEP_SKIP   = 5     # data timesteps skipped between frames
FINAL_INDEX = None  # last data index to animate (None = use all)
N_WORKERS   = min(32, max(1, mp.cpu_count() - 1))  # cap to avoid OOM

# ── Find most recent output directory ────────────────────────────────────────
output_root = os.path.join(os.path.dirname(__file__), "output")
run_dirs = sorted(
    [d for d in os.listdir(output_root) if d.startswith("out_")],
    key=lambda d: int(d.split("_")[1])
)
if not run_dirs:
    raise FileNotFoundError(f"No output directories found in {output_root}")

latest = os.path.join(output_root, run_dirs[-1])
print(f"Loading from: {latest}")

# ── Load data ─────────────────────────────────────────────────────────────────
data              = np.load(os.path.join(latest, "results.npz"))
positions         = data["positions"]           # (timeSteps, numParticles, 2)
temperatures      = data["temperatures"]        # (timeSteps,)
potentialEnergies = data["potentialEnergies"]   # (timeSteps,)
kineticEnergies   = data["kineticEnergies"]     # (timeSteps,)
totalEnergies     = data["totalEnergies"]       # (timeSteps,)
metadata          = data["metadata"]            # [L, numParticles, timeSteps, finalTime]

L            = float(metadata[0])
numParticles = int(metadata[1])
timeSteps    = int(metadata[2])
finalTime    = float(metadata[3])

end = FINAL_INDEX if FINAL_INDEX is not None else timeSteps - 1
frame_indices = np.arange(0, end + 1, STEP_SKIP)

# ── Parallel segment renderer ─────────────────────────────────────────────────
def render_segment(args):
    """Render a contiguous slice of frames to a temp .mp4 file.
    Receives only pos_slice (the frames this worker needs) to avoid OOM.
    """
    seg_frames, pos_slice, L, FPS, out_path = args
    # pos_slice shape: (len(seg_frames), numParticles, 2)

    fig, ax = plt.subplots(figsize=(6, 6))
    ax.set_xlim(0, L)
    ax.set_ylim(0, L)
    ax.set_aspect("equal")
    ax.set_xlabel("x (σ)")
    ax.set_ylabel("y (σ)")

    scat = ax.scatter(
        pos_slice[0, :, 0], pos_slice[0, :, 1],
        s=10, c="steelblue", edgecolors="k", linewidths=0.4
    )

    def update(local_idx):
        scat.set_offsets(pos_slice[local_idx])
        ax.set_title(f"Molecular Dynamics — step {seg_frames[local_idx]}")
        return (scat,)

    ani = animation.FuncAnimation(
        fig, update, frames=range(len(seg_frames)), interval=1000 // FPS, blit=True
    )
    writer = animation.FFMpegWriter(
        fps=FPS, bitrate=1800, codec="mpeg4",
        extra_args=["-pix_fmt", "yuv420p"]
    )
    ani.save(out_path, writer=writer)
    plt.close(fig)
    return out_path

# ── Split frames into segments and render in parallel ────────────────────────
print(f"Rendering {len(frame_indices)} frames across {N_WORKERS} workers...")

# Split frame_indices into N_WORKERS roughly equal chunks
segments = np.array_split(frame_indices, N_WORKERS)
segments = [s for s in segments if len(s) > 0]

tmp_dir = tempfile.mkdtemp()
seg_paths = [os.path.join(tmp_dir, f"seg_{i:04d}.mp4") for i in range(len(segments))]

worker_args = [
    (seg, positions[seg], L, FPS, path)   # pass only this segment's frames
    for seg, path in zip(segments, seg_paths)
]

with mp.Pool(processes=len(segments)) as pool:
    pool.map(render_segment, worker_args)

# ── Stitch segments with ffmpeg concat ───────────────────────────────────────
concat_list = os.path.join(tmp_dir, "concat.txt")
with open(concat_list, "w") as f:
    for p in seg_paths:
        f.write(f"file '{p}'\n")

anim_path = os.path.join(latest, "md_animation.mp4")
subprocess.run([
    FFMPEG, "-y", "-f", "concat", "-safe", "0",
    "-i", concat_list,
    "-c", "copy",
    anim_path
], check=True, capture_output=True)

# Clean up temp files
for p in seg_paths:
    os.remove(p)
os.remove(concat_list)
os.rmdir(tmp_dir)

print(f"Animation saved to: {anim_path}")

# ── Energy plots ──────────────────────────────────────────────────────────────
fig2, axes = plt.subplots(3, 1, figsize=(10, 8), sharex=True)
t = np.arange(timeSteps)

axes[0].plot(t, potentialEnergies, color="royalblue",   label="Potential Energy")
axes[0].plot(t, kineticEnergies,   color="tomato",      label="Kinetic Energy")
axes[0].plot(t, totalEnergies,     color="forestgreen", label="Total Energy", linestyle="--")
axes[0].set_ylabel("Energy (ε)")
axes[0].legend()
axes[0].grid(True)

axes[1].plot(t, temperatures, color="darkorange")
axes[1].set_ylabel("Temperature (reduced)")
axes[1].grid(True)

axes[2].plot(t, totalEnergies - totalEnergies[0], color="purple")
axes[2].axhline(0, color="k", linewidth=0.5, linestyle="--")
axes[2].set_ylabel("ΔE (energy drift)")
axes[2].set_xlabel("Timestep")
axes[2].grid(True)

fig2.suptitle("Molecular Dynamics — Energy Analysis")
fig2.tight_layout()

energy_plot_path = os.path.join(latest, "energy_plot.png")
fig2.savefig(energy_plot_path, dpi=150)
print(f"Energy plot saved to: {energy_plot_path}")

# __END__
