#!/bin/bash
# /**
# @brief SLURM batch script for rendering molecular-dynamics animations from the latest output run.
# @details Allocates compute resources, loads Python and ffmpeg modules, and executes plotting.py with worker count tied to allocated CPUs.
# */
#SBATCH --job-name=molecular_dynamics_anamation  # Job name
#SBATCH --output=slurm_out/slurm_%j.out       # stdout log (%j = job ID)
#SBATCH --error=slurm_out/slurm_%j.err        # stderr log
#SBATCH --nodes=1                  # single node (OpenMP, not MPI)
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=20          # number of OpenMP threads — adjust to node size
#SBATCH --mem=32G
#SBATCH --time=02:00:00             # wall time — increase if needed
##SBATCH --qos=standby
##SBATCH --partition=compute        # uncomment and set your partition name if required
##SBATCH --account=your_account     # uncomment and set your allocation if required

# --- Environment -------------------------------------------------------
# Load modules appropriate for your cluster. Common examples:
#   module load gcc/12 zlib
# If your cluster uses environment modules, uncomment/adjust the lines below:
# module purge
# module load gcc zlib

module load python
module load ffmpeg

# Tell OpenMP to use all allocated CPUs
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

# --- Build -------------------------------------------------------------
cd "$SLURM_SUBMIT_DIR"

# ---------------------------------------------------------------------------
# CPU signature: extract key SIMD instruction sets supported by this node.
# Flags are sorted so the comparison is stable across different lscpu output
# orderings.  Only flags that affect code-gen with -march=native are tracked.
# ---------------------------------------------------------------------------




# --- Run ---------------------------------------------------------------
echo "=== Running with OMP_NUM_THREADS=$OMP_NUM_THREADS at $(date) ==="

echo "== Creating Animation results at $(date) ==="
time python plotting.py -w $OMP_NUM_THREADS

echo "=== Done at $(date) ==="
