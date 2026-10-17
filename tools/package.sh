#!/bin/bash
# Build a data-free Windows package. Never include original/ or work/.
set -euo pipefail
cd "$(dirname "$0")/.."
source ~/tools/env.sh
cmake --build build -j16
python3 -I - <<'PY'
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

output = Path("build/crayons-win11.zip")
with ZipFile(output, "w", compression=ZIP_DEFLATED) as archive:
    archive.write("build/crayons.exe", "crayons.exe")
    archive.write("README.md", "README.md")
print(output)
PY
