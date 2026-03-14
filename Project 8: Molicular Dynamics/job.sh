#!/bin/bash
#SBATCH --job-name=molecular_dynamics  # Job name
#SBATCH --output=slurm_out/slurm_%j.out       # stdout log (%j = job ID)
#SBATCH --error=slurm_out/slurm_%j.err        # stderr log
#SBATCH --nodes=1                  # single node (OpenMP, not MPI)
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8          # number of OpenMP threads — adjust to node size
#SBATCH --mem=16G
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

# ---------------------------------------------------------------------------
# CPU signature: extract key SIMD instruction sets supported by this node.
# Flags are sorted so the comparison is stable across different lscpu output
# orderings.  Only flags that affect code-gen with -march=native are tracked.
# ---------------------------------------------------------------------------
get_cpu_signature() {
    grep -m1 "^flags" /proc/cpuinfo \
        | tr ' ' '\n' \
        | grep -E '^(avx|avx2|avx512f|avx512bw|avx512cd|avx512dq|avx512vl|sse4_1|sse4_2|fma|bmi|bmi2)$' \
        | sort \
        | tr '\n' ':' \
        | sed 's/:$//'
}

BINARY="bin/main"
ARCH_FILE="bin/main.arch"
CURRENT_SIG=$(get_cpu_signature)

echo "=== Build check on $(hostname) at $(date) ==="
echo "    CPU signature : $CURRENT_SIG"

needs_rebuild=false
rebuild_reason=""

# --- Check 1: binary must exist -----------------------------------------
if [ ! -f "$BINARY" ]; then
    needs_rebuild=true
    rebuild_reason="Binary not found."
fi

# --- Check 2: binary must be newer than all source / header files -------
if [ "$needs_rebuild" = false ]; then
    newer=$(find . \( -name "*.cpp" -o -name "*.h" \) -newer "$BINARY" 2>/dev/null | head -1)
    if [ -n "$newer" ]; then
        needs_rebuild=true
        rebuild_reason="Source/header newer than binary: $newer"
    fi
fi

# --- Check 3: binary must match current hardware ------------------------
if [ "$needs_rebuild" = false ]; then
    if [ ! -f "$ARCH_FILE" ]; then
        needs_rebuild=true
        rebuild_reason="No architecture record found (bin/main.arch missing)."
    else
        STORED_SIG=$(cat "$ARCH_FILE")
        if [ "$STORED_SIG" != "$CURRENT_SIG" ]; then
            needs_rebuild=true
            rebuild_reason="CPU mismatch — stored: [$STORED_SIG]  current: [$CURRENT_SIG]"
        fi
    fi
fi

# --- Build if required --------------------------------------------------
if [ "$needs_rebuild" = true ]; then
    echo "    Reason       : $rebuild_reason"
    echo "=== Building (release) on $(hostname) at $(date) ==="
    time make release
    if [ $? -ne 0 ]; then
        echo "Build failed — aborting." >&2
        exit 1
    fi
    # Record the CPU signature so future jobs can detect hardware changes
    echo "$CURRENT_SIG" > "$ARCH_FILE"
    echo "    Architecture record saved: $CURRENT_SIG"
else
    echo "    Binary is up to date and matches current hardware — skipping build."
fi



# --- Run ---------------------------------------------------------------
echo "=== Running with OMP_NUM_THREADS=$OMP_NUM_THREADS at $(date) ==="
time ./bin/main simulation_2.cfg
if [ $? -ne 0 ]; then
    echo "Execution failed — aborting." >&2
    exit 1
fi

echo "== Creating Animation results at $(date) ==="
sbatch anamation.sh
if [ $? -ne 0 ]; then
    echo "Failed to submit animation job — aborting." >&2
    exit 1
fi

echo "== Plotting Energy data at $(date) ==="
sbatch plotting.sh
if [ $? -ne 0 ]; then
    echo "Failed to submit plotting job — aborting." >&2
    exit 1
fi

echo "=== Done at $(date) ==="
