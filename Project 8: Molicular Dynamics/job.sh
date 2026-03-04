#!/bin/bash
#SBATCH --job-name=molecular_dynamics  # Job name
#SBATCH --output=slurm_out/slurm_%j.out       # stdout log (%j = job ID)
#SBATCH --error=slurm_out/slurm_%j.err        # stderr log
#SBATCH --nodes=1                   # single node (OpenMP, not MPI)
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16          # number of OpenMP threads — adjust to node size
#SBATCH --mem=20G
#SBATCH --time=02:00:00             # wall time — increase if needed
#SBATCH --qos=standby
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

echo "=== Building (release) on $(hostname) at $(date) ==="
# Get data about processer and store it in a log for reference
# lscpu > lscpu.log

make release
if [ $? -ne 0 ]; then
    echo "Build failed — aborting." >&2
    exit 1
fi



# --- Run ---------------------------------------------------------------
echo "=== Running with OMP_NUM_THREADS=$OMP_NUM_THREADS at $(date) ==="
time ./bin/main

echo "== Ploting results at $(date) ==="
time python plotting.py

echo "=== Done at $(date) ==="
