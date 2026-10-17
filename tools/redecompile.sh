#!/bin/bash
# Re-apply names (auto-generated starts + docs/re/names.tsv) and regenerate work/ghidra/crayon<N>.c.
# Usage: tools/redecompile.sh [1|2]
set -euo pipefail
cd "$(dirname "$0")/.."
source ~/tools/env.sh
n="${1:-1}"
cat "work/names_auto$n.tsv" docs/re/names.tsv 2>/dev/null > "work/names_all$n.tsv"
"$GHIDRA_HOME/support/analyzeHeadless" work/ghidra Crayons -process "CRAYON$n.EXE" -noanalysis \
  -scriptPath tools/ghidra -preScript ApplyNames.java "work/names_all$n.tsv" \
  -postScript DecompileAll.java "work/ghidra/crayon$n.c" "work/ghidra/functions$n.tsv" > "work/ghidra/redecompile$n.log" 2>&1
grep -E "applied|decompiled|ERROR" "work/ghidra/redecompile$n.log" | head
