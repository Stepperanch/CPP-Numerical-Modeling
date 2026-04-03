import numpy as np
import os

"""
/**
@brief Quick smoke-test loader for latest molecular-dynamics output arrays.
@details Validates that trajectory and energy arrays can be loaded and prints early frame samples for manual inspection.
*/
"""

FINAL_INDEX = None  # last data index to animate (None = use all)
STEP_SKIP   = 4     # data timesteps skipped between frames

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

L            = float(metadata[0])
numParticles = int(metadata[1])
timeSteps    = int(metadata[2])
finalTime    = float(metadata[3])

numPosFrames = positions.shape[0]  # may be timeSteps / skip from the C++ side
end = FINAL_INDEX if FINAL_INDEX is not None else numPosFrames - 1
frame_indices = np.arange(0, end + 1, STEP_SKIP)


for i in range(min(4, len(positions))):
    print(f"Frame {i}:")
    print(positions[i])