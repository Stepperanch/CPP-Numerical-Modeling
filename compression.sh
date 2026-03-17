#!/bin/bash

# --- Slurm Header (Edit these for your specific needs) ---
#SBATCH --cpus-per-task=8
#SBATCH --mem=12G
#SBATCH --time=01:00:00
#SBATCH --job-name=video_compress

# 1. Capture arguments
INPUT_FILE=$1
OUTPUT_FILE=$2

# 2. Basic Error Checking
if [ -z "$INPUT_FILE" ] || [ -z "$OUTPUT_FILE" ]; then
    echo "Usage: sbatch job.sh <input_path> <output_path>"
    exit 1
fi

# 3. Load Environment
# Initialize conda from user's home directory
source ~/.bashrc
conda activate ffmpeg_full

# 4. Handle Threading
# We tell FFmpeg to use exactly what Slurm provided
if [ -z "$SLURM_CPUS_PER_TASK" ]; then
    THREADS=4  # Fallback if run locally
else
    THREADS=$SLURM_CPUS_PER_TASK
fi

export OMP_NUM_THREADS=$THREADS

echo "Compressing $INPUT_FILE using $THREADS threads..."

# 5. The Optimized Command
ffmpeg -y -i "$INPUT_FILE" \
    -threads "$THREADS" \
    -vf "setpts=0.5*PTS,scale=800*800" \
    -r 30 \
    -c:v libx265 \
    -crf 20 \
    -preset medium \
    -tune animation \
    -pix_fmt yuv420p \
    -an \
    -tag:v hvc1 \
    "$OUTPUT_FILE"

echo "Done. Saved to $OUTPUT_FILE"