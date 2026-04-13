#!/usr/bin/env bash
set -euo pipefail

MODE="${1:-check}"

if [[ "$MODE" != "check" && "$MODE" != "fix" ]]; then
  echo "Usage: scripts/markdown_math_pipe_guard.sh [check|fix]"
  exit 2
fi

ROOT_DIR="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$ROOT_DIR"

# Markdown files in source docs only (skip generated output and heavy asset folders).
FILES=()
while IFS= read -r -d '' file; do
  FILES+=("$file")
done < <(
  find . -type f -name '*.md' \
    ! -path '*/.git/*' \
    ! -path '*/.venv/*' \
    ! -path '*/output/*' \
    ! -path '*/Output/*' \
    ! -path '*/assets/html-assets/*' \
    ! -path '*/assets/videos/*' \
    ! -path '*/assets/images/*' \
    ! -path '*/assets/plots/*' \
    -print0
)

if [[ ${#FILES[@]} -eq 0 ]]; then
  echo "No markdown files found in scope."
  exit 0
fi

TRANSFORM_PERL='
  our $in_code_fence = 0;

  sub fix_expr {
    my ($expr) = @_;

    # Convert norm-style bars first so they do not get consumed by abs conversion.
    $expr =~ s/(?<!\\)\|\|\s*([^|]+?)\s*(?<!\\)\|\|/\\lVert $1 \\rVert/g;

    # Convert absolute-value bars.
    $expr =~ s/(?<!\\)\|\s*([^|]+?)\s*(?<!\\)\|/\\lvert $1 \\rvert/g;

    # Keep output clean when bars wrap simple symbols.
    $expr =~ s/\\lvert\s+([A-Za-z0-9_\\]+)\s+\\rvert/\\lvert$1\\rvert/g;
    $expr =~ s/\\lVert\s+([A-Za-z0-9_\\]+)\s+\\rVert/\\lVert$1\\rVert/g;

    return $expr;
  }

  if (/^```/) {
    $in_code_fence = !$in_code_fence;
    print;
    next;
  }

  if ($in_code_fence) {
    print;
    next;
  }

  s/\$([^$\n]+)\$/"\$" . fix_expr($1) . "\$"/ge;
  print;
'

if [[ "$MODE" == "fix" ]]; then
  perl -i -ne "$TRANSFORM_PERL" "${FILES[@]}"

  echo "Fixed inline math bars in markdown files."
fi

FOUND=0
for file in "${FILES[@]}"; do
  tmp_file="$(mktemp)"
  perl -ne "$TRANSFORM_PERL" "$file" > "$tmp_file"
  if ! cmp -s "$file" "$tmp_file"; then
    echo "$file"
    FOUND=1
  fi
  rm -f "$tmp_file"
done

if [[ $FOUND -eq 1 ]]; then
  echo
  echo "Found markdown math that needs bar normalization."
  echo "Run: scripts/markdown_math_pipe_guard.sh fix"
  exit 1
fi

echo "Markdown math-pipe check passed."
