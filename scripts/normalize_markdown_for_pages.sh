#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$ROOT_DIR"

# Markdown docs only; skip generated artifacts and large asset folders.
FILES=()
while IFS= read -r -d '' file; do
  FILES+=("$file")
done < <(
  find . -type f -name '*.md' \
    ! -path '*/.git/*' \
    ! -path '*/.venv/*' \
    ! -path '*/_site/*' \
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

for file in "${FILES[@]}"; do
  perl - "$file" <<'PERL'
use strict;
use warnings;

my $path = shift @ARGV;

open my $rfh, '<', $path or die "Unable to read $path: $!";
my @lines = <$rfh>;
close $rfh;

my $original = join('', @lines);

# Pass 1: convert GitHub callout markers into Markdown-safe blockquote labels.
my $in_code_fence = 0;
for my $line (@lines) {
  if ($line =~ /^```/) {
    $in_code_fence = !$in_code_fence;
    next;
  }

  next if $in_code_fence;

  if ($line =~ /^>\s*\[!(TIP|NOTE|IMPORTANT|WARNING|CAUTION)\]\s*$/i) {
    my $label = ucfirst(lc($1));
    $line = "> **$label.**\n";
  }
}

sub is_blank {
  my ($line) = @_;
  return $line =~ /^\s*$/;
}

sub is_pipe_row {
  my ($line) = @_;
  return $line =~ /^\s*\|.*\|\s*$/;
}

sub is_table_delimiter {
  my ($line) = @_;
  return $line =~ /^\s*\|\s*:?-{3,}:?(?:\s*\|\s*:?-{3,}:?)+\s*\|?\s*$/;
}

# Pass 2: enforce a blank line before and after each pipe table.
my @output;
$in_code_fence = 0;

for (my $i = 0; $i <= $#lines; $i++) {
  my $line = $lines[$i];

  if ($line =~ /^```/) {
    push @output, $line;
    $in_code_fence = !$in_code_fence;
    next;
  }

  if (
    !$in_code_fence &&
    is_pipe_row($line) &&
    $i + 1 <= $#lines &&
    is_table_delimiter($lines[$i + 1])
  ) {
    if (@output && !is_blank($output[-1])) {
      push @output, "\n";
    }

    while ($i <= $#lines && is_pipe_row($lines[$i])) {
      push @output, $lines[$i];
      $i++;
    }
    $i--;

    if ($i < $#lines && !is_blank($lines[$i + 1])) {
      push @output, "\n";
    }

    next;
  }

  push @output, $line;
}

my $normalized = join('', @output);

if ($normalized ne $original) {
  open my $wfh, '>', $path or die "Unable to write $path: $!";
  print {$wfh} $normalized;
  close $wfh;
}
PERL
done

echo "Normalized markdown for GitHub Pages build in ${#FILES[@]} files."
