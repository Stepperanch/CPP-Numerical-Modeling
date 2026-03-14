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
print(positions.shape)

for i in range(18260, 18278, 1):
    print(f"Position sample {i}: {positions[i, 0]}  {potentialEnergies[i]}")