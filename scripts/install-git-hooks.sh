#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$ROOT_DIR"

git config core.hooksPath .githooks
chmod +x .githooks/pre-commit scripts/markdown_math_pipe_guard.sh

echo "Installed repo hooks."
echo "Pre-commit will now auto-fix markdown math bars and block commit if issues remain."
