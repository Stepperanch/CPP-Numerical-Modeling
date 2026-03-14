import numpy as np
import matplotlib
matplotlib.use("Agg")  # non-interactive backend, required for multiprocessing
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import os
import subprocess
import tempfile
import multiprocessing as mp
import argparse

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
FPS         = 60    # frames per second
STEP_SKIP   = 1     # data timesteps skipped between frames
FINAL_INDEX = None  # last data index to animate (None = use all)

# ── Parse command-line arguments ───────────────────────────────────────────────
parser = argparse.ArgumentParser(description="Generate molecular dynamics animation and energy plots")
parser.add_argument(
    "--workers", "-w",
    type=int,
    default=min(8, max(1, mp.cpu_count() - 1)),
    help=f"Number of worker processes (default: {min(8, max(1, mp.cpu_count() - 1))})"
)
args = parser.parse_args()
N_WORKERS = args.workers

# ── Find most recent output directory ────────────────────────────────────────
output_root = os.path.join(os.path.dirname(__file__), "output")
i = 0
run_dirs = sorted(
    [d for d in os.listdir(output_root) if d.startswith("out_")],
    key=lambda d: int(d.split("_")[1])
)

if not run_dirs:
    raise FileNotFoundError(f"No output directories found in {output_root}")

i = int(run_dirs[-1].split("_")[1])

latest = os.path.join(output_root, run_dirs[-1])
print(f"Loading from: {latest}")
print(f"Using {N_WORKERS} worker processes")

# ── Load data ─────────────────────────────────────────────────────────────────
# Support both new individual .npy files and legacy .npz format
npz_path = os.path.join(latest, f"results_{i}.npz")
npy_pos  = os.path.join(latest, "positions.npy")

if os.path.exists(npy_pos):
    # New format: individual .npy files (avoids 4 GB ZIP limit)
    print("Loading individual .npy files...")
    positions         = np.load(os.path.join(latest, "positions.npy"))           # (timeSteps/5, numParticles, 2)
    temperatures      = np.load(os.path.join(latest, "temperatures.npy"))        # (timeSteps,)
    potentialEnergies = np.load(os.path.join(latest, "potentialEnergies.npy"))   # (timeSteps,)
    kineticEnergies   = np.load(os.path.join(latest, "kineticEnergies.npy"))     # (timeSteps,)
    totalEnergies     = np.load(os.path.join(latest, "totalEnergies.npy"))       # (timeSteps,)
    metadata          = np.load(os.path.join(latest, "metadata.npy"))            # [L, numParticles, timeSteps, finalTime]
elif os.path.exists(npz_path):
    # Legacy format: single .npz file
    print("Loading legacy .npz file...")
    data              = np.load(npz_path)
    positions         = data["positions"]
    temperatures      = data["temperatures"]
    potentialEnergies = data["potentialEnergies"]
    kineticEnergies   = data["kineticEnergies"]
    totalEnergies     = data["totalEnergies"]
    metadata          = data["metadata"]
else:
    raise FileNotFoundError(f"No results found in {latest}")

width        = float(metadata[0])
height       = float(metadata[1])  # square box
numParticles = int(metadata[2])
timeSteps    = int(metadata[3])
finalTime    = float(metadata[4])

numPosFrames = positions.shape[0]  # may be timeSteps / skip from the C++ side
end = FINAL_INDEX if FINAL_INDEX is not None else numPosFrames - 1
frame_indices = np.arange(0, end + 1, STEP_SKIP)

flat_positions = positions.reshape(numPosFrames, -1)
all_zero_frame_mask = np.all(flat_positions == 0.0, axis=1)
if np.any(all_zero_frame_mask):
    first_zero_frame = int(np.argmax(all_zero_frame_mask))
    if all_zero_frame_mask[first_zero_frame: ].all():
        print(f"WARNING: Position data is all zeros from frame {first_zero_frame} onward. This indicates truncated/incorrect simulation output.")
    else:
        zero_count = int(np.sum(all_zero_frame_mask))
        print(f"WARNING: Found {zero_count} all-zero position frames in loaded data.")

print(f"Loaded data: {numPosFrames} position frames, {timeSteps} timesteps, {numParticles} particles.")

# ── Parallel segment renderer ─────────────────────────────────────────────────
def render_segment(args):
    """Render a contiguous slice of frames to a temp .mp4 file.
    Receives only pos_slice (the frames this worker needs) to avoid OOM.
    """
    seg_frames, pos_slice, width, height, FPS, out_path = args
    # pos_slice shape: (len(seg_frames), numParticles, 2)

    try:
        fig, ax = plt.subplots(figsize=(8, 8), dpi = 200)
        ax.set_xlim(0, width)
        ax.set_ylim(0, height)
        ax.set_aspect("equal")
        ax.set_xlabel("x (σ)")
        ax.set_ylabel("y (σ)")

        scat = ax.scatter(
            pos_slice[0, :, 0], pos_slice[0, :, 1],
            s=3, c="steelblue", edgecolors="k", linewidths=0.4
        )

        def update(local_idx):
            scat.set_offsets(pos_slice[local_idx])
            ax.set_title(f"Molecular Dynamics — step {seg_frames[local_idx]}")
            return (scat,)

        ani = animation.FuncAnimation(
            fig, update, frames=range(len(seg_frames)), interval=1000 // FPS, blit=True
        )
        writer = animation.FFMpegWriter(
            fps=FPS, bitrate=15000, codec="mpeg4",
            extra_args=["-pix_fmt", "yuv420p"]
        )
        ani.save(out_path, writer=writer)
        plt.close(fig)

        # Verify output file was created and has content
        if os.path.exists(out_path):
            file_size = os.path.getsize(out_path)
            print(f"✓ Segment frames {seg_frames[0]:05d}-{seg_frames[-1]:05d} ({len(seg_frames):4d} frames) → {out_path} ({file_size/1024/1024:6.1f} MB)")
        else:
            raise FileNotFoundError(f"Output file was not created: {out_path}")

        return out_path
    except Exception as e:
        print(f"✗ Segment frames {seg_frames[0]:05d}-{seg_frames[-1]:05d} FAILED: {e}")
        raise

# ── Split frames into segments and render in parallel ────────────────────────
print(f"Rendering {len(frame_indices)} frames across {N_WORKERS} workers...")
print(f"Frame range: {frame_indices[0]} to {frame_indices[-1]}")

# Split frame_indices into N_WORKERS roughly equal chunks
segments = np.array_split(frame_indices, N_WORKERS)
segments = [s for s in segments if len(s) > 0]

total_frames_to_render = sum(len(seg) for seg in segments)
print(f"Total frames in all segments: {total_frames_to_render}")

tmp_dir = tempfile.mkdtemp()
seg_paths = [os.path.join(tmp_dir, f"seg_{seg_idx:04d}.mp4") for seg_idx in range(len(segments))]

print(f"Segment breakdown:")
for seg_idx, seg in enumerate(segments):
    print(f"  Segment {seg_idx}: frames {seg[0]}-{seg[-1]} ({len(seg)} frames)")

worker_args = [
    (seg, positions[seg], width, height, FPS, path)   # pass only this segment's frames
    for seg, path in zip(segments, seg_paths)
]

print(f"Processing {len(worker_args)} segments with {N_WORKERS} workers...")

with mp.Pool(processes=min(N_WORKERS, len(segments))) as pool:
    try:
        results = pool.map(render_segment, worker_args)
        print(f"Successfully rendered {len(results)} segments")
    except Exception as e:
        print(f"Worker pool error: {e}")
        pool.terminate()
        pool.join()
        raise

# ── Stitch segments with ffmpeg concat ───────────────────────────────────────
concat_list = os.path.join(tmp_dir, "concat.txt")
with open(concat_list, "w") as f:
    for p in seg_paths:
        f.write(f"file '{p}'\n")

# Verify all segment files exist before concatenating
print("\nVerifying segment files before concatenation...")
missing_files = []
total_segment_size = 0
for seg_idx, p in enumerate(seg_paths):
    if os.path.exists(p):
        size = os.path.getsize(p)
        total_segment_size += size
        print(f"  Segment {seg_idx}: {size/1024/1024:6.1f} MB ✓")
    else:
        missing_files.append(p)
        print(f"  Segment {seg_idx}: MISSING ✗")

if missing_files:
    print(f"\nERROR: Missing {len(missing_files)} segment files:")
    for p in missing_files:
        print(f"  {p}")
    raise FileNotFoundError(f"{len(missing_files)} segment files were not created")

print(f"Total segment data: {total_segment_size/1024/1024:.1f} MB")
print(f"\nConcatenating {len(seg_paths)} segments with ffmpeg...")

# Debug: print the concat list content
with open(concat_list, "r") as f:
    content = f.read()
    print(f"Concat file contains:\n{content}")

anim_path = os.path.join(latest, f"md_animation_{i}.mp4")
result = subprocess.run([
    FFMPEG, "-y", "-f", "concat", "-safe", "0",
    "-i", concat_list,
    "-c", "copy",
    anim_path
], capture_output=True, text=True)

if result.returncode != 0:
    print(f"FFmpeg STDERR:\n{result.stderr}")
    print(f"FFmpeg STDOUT:\n{result.stdout}")
    raise RuntimeError(f"FFmpeg concat failed with return code {result.returncode}")

print(f"Concatenation complete. Output: {anim_path}")

# Check final output file size
if os.path.exists(anim_path):
    final_size = os.path.getsize(anim_path)
    print(f"Final animation file: {final_size/1024/1024:.1f} MB")
else:
    raise FileNotFoundError(f"Final animation file was not created: {anim_path}")

# Clean up temp files
for p in seg_paths:
    os.remove(p)
os.remove(concat_list)
os.rmdir(tmp_dir)

print(f"Animation saved to: {anim_path}")

