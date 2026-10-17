#!/bin/bash
# Print one function from work/disasm<N>/seg<S>.asm, compacted: tools/fn.sh sub_2_6390 [N=1]
f="$1"; n="${2:-1}"; seg=$(echo "$f" | cut -d_ -f2)
file="$(dirname "$0")/../work/disasm$n/seg$seg.asm"
awk -v f="$f" '
  $0 ~ "^"f"( |:)" {on=1; print; next}
  on && /^sub_[0-9]+_[0-9a-f]+( <[^>]*>)?:$/ {exit}
  on {sub(/^[ \t]+/, ""); print}' "$file"
