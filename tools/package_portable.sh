#!/bin/bash
# Plug-and-play release asset. Requires redistribution rights for the original CD data.
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ $# -ne 1 || ! -f "$1" ]]; then
    echo "usage: tools/package_portable.sh <original CD image or CRAYONS.Z>" >&2
    exit 2
fi
source ~/tools/env.sh
cmake --build build -j16
source_path="$(realpath "$1")"
stage="$(mktemp -d build/crayons-portable.XXXXXX)"
trap 'rm -rf -- "$stage"' EXIT
mkdir -p "$stage/data"
./build/crayons.exe --import "$(wslpath -w "$source_path")" --out "$(wslpath -w "$stage/data")"
cp build/crayons.exe README.md "$stage/"
python3 -I - "$stage" <<'PY'
from pathlib import Path
import sys
from zipfile import ZIP_DEFLATED, ZipFile

stage = Path(sys.argv[1])
output = Path("build/Crayons-Windows-11.zip")
files = sorted(path for path in stage.rglob("*") if path.is_file())
with ZipFile(output, "w", compression=ZIP_DEFLATED) as archive:
    for path in files:
        archive.write(path, path.relative_to(stage))
print(f"{output}: {len(files)} files")
PY
