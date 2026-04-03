#!/bin/bash
# /**
# @brief Primary SLURM workflow for Project 8.
# @details Validates build freshness, optionally auto-tunes thread and neighbor-skin settings, runs the simulation, then submits plotting and animation follow-up jobs.
# */
#SBATCH --job-name=molecular_dynamics  # Job name
#SBATCH --output=slurm_out/slurm_%j.out       # stdout log (%j = job ID)
#SBATCH --error=slurm_out/slurm_%j.err        # stderr log
#SBATCH --nodes=1                  # single node (OpenMP, not MPI)
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8          # number of OpenMP threads — adjust to node size
#SBATCH --mem=8G
#SBATCH --time=20:00:00             # wall time — increase if needed
##SBATCH --qos=normal
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
export OMP_PROC_BIND=spread
export OMP_PLACES=cores

# --- Build -------------------------------------------------------------
cd "$SLURM_SUBMIT_DIR"

# ---------------------------------------------------------------------------
# CPU signature: extract key SIMD instruction sets supported by this node.
# Flags are sorted so the comparison is stable across different lscpu output
# orderings.  Only flags that affect code-gen with -march=native are tracked.
# ---------------------------------------------------------------------------
# /**
# @brief Build a compact CPU feature signature used to detect architecture changes across nodes.
# @return Colon-delimited SIMD feature list via stdout.
# */
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
AUTO_TUNE=${AUTO_TUNE:-1}
RUN_CFG="simulation_2.cfg"

if [ "$AUTO_TUNE" = "1" ]; then
    echo "=== Auto-tuning thread/neighbor settings at $(date) ==="
    BENCH_CFG="simulation_2_autotune_${SLURM_JOB_ID}.cfg"
    BENCH_CSV="slurm_out/tune_${SLURM_JOB_ID}.csv"
    TUNED_CFG="simulation_2_tuned_${SLURM_JOB_ID}.cfg"

    cp simulation_2.cfg "$BENCH_CFG"
    perl -0pi -e 's/^timeSteps=\d+/timeSteps=1000/m; s/^finalTime=[0-9.]+/finalTime=2.0/m; s/^stepSkip=\d+/stepSkip=20/m' "$BENCH_CFG"

    if grep -q '^showProgress=' "$BENCH_CFG"; then
        perl -0pi -e 's/^showProgress=.*/showProgress=0/m' "$BENCH_CFG"
    else
        echo "showProgress=0" >> "$BENCH_CFG"
    fi

    if OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK ./bin/main "$BENCH_CFG" --benchmark > "$BENCH_CSV"; then
        BEST_ROW=$(awk -F, 'NR>2 && $1 ~ /^[0-9]+$/ {if(($6+0)>best){best=$6+0; row=$0}} END{print row}' "$BENCH_CSV")
        if [ -n "$BEST_ROW" ]; then
            BEST_THREADS=$(echo "$BEST_ROW" | awk -F, '{print $1}')
            BEST_SKIN=$(echo "$BEST_ROW" | awk -F, '{print $2}')
            echo "    Best row: $BEST_ROW"

            if [ "$BEST_THREADS" -gt "$SLURM_CPUS_PER_TASK" ]; then
                BEST_THREADS=$SLURM_CPUS_PER_TASK
            fi

            cp simulation_2.cfg "$TUNED_CFG"
            if grep -q '^neighborSkin=' "$TUNED_CFG"; then
                perl -0pi -e "s/^neighborSkin=.*/neighborSkin=$BEST_SKIN/m" "$TUNED_CFG"
            else
                echo "neighborSkin=$BEST_SKIN" >> "$TUNED_CFG"
            fi

            export OMP_NUM_THREADS=$BEST_THREADS
            RUN_CFG="$TUNED_CFG"
            echo "    Selected OMP_NUM_THREADS=$OMP_NUM_THREADS"
            echo "    Selected neighborSkin=$BEST_SKIN"
            echo "    Tuned config: $RUN_CFG"
        else
            echo "    Warning: No valid benchmark rows found; falling back to simulation_2.cfg"
        fi
    else
        echo "    Warning: Auto-tune benchmark failed; falling back to simulation_2.cfg"
    fi
fi

echo "=== Running with OMP_NUM_THREADS=$OMP_NUM_THREADS at $(date) ==="
time ./bin/main "$RUN_CFG"
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
