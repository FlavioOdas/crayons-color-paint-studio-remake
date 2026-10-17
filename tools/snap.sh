#!/bin/bash
# Build, run a headless snapshot and convert it to PNG.
# Usage: tools/snap.sh <name> [crayons.exe options...]   -> work/test/<name>.png
# Uses the test data folder %TEMP%\Crayons-test\data (import it once with --import ... --out).
set -euo pipefail
cd "$(dirname "$0")/.."
source ~/tools/env.sh
cmake --build build -j16
name="$1"; shift
mkdir -p work/test
out="$(wslpath -w work/test)\\$name.bmp"
if [[ -n ${CRAYONS_TEST_DATA:-} ]]; then
    data="$CRAYONS_TEST_DATA"
    if [[ $data == /* ]]; then data="$(wslpath -w "$data")"; fi
else
    data="$(cmd.exe /C 'echo %TEMP%' 2>/dev/null | tr -d '\r' | tail -n 1)\\Crayons-test\\data"
fi
./build/crayons.exe --data "$data" --snapshot "$out" "$@"
python3 -I tools/bmp2png.py work/test "work/test/$name.bmp"
mv "work/test/$name.bmp.png" "work/test/$name.png"
echo "work/test/$name.png"
