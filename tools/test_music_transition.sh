#!/bin/bash
# Usage: tools/test_music_transition.sh <isolated imported test data directory>
set -euo pipefail
if [[ $# -ne 1 || ! -d "$1" ]]; then
    echo "usage: tools/test_music_transition.sh <isolated imported test data directory>" >&2
    exit 2
fi
data="$(realpath "$1")"
cd "$(dirname "$0")/.."
source ~/tools/env.sh
mkdir -p work/test
x86_64-w64-mingw32-clang++ -std=c++20 -static -municode tools/test_music_transition.cpp \
  -o work/test/test_music_transition.exe -luser32 -lwinmm
./work/test/test_music_transition.exe "$(wslpath -w build/crayons.exe)" "$(wslpath -w "$data")"
