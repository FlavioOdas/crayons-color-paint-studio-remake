"""Regression check for the About form's original close box and click hotspot.

Usage: python3 -I tools/test_about_close.py --data /mnt/c/.../Temp/Crayons-test/data
"""
import argparse
from pathlib import Path
import struct
import subprocess


def windows_path(path):
    return subprocess.check_output(["wslpath", "-w", str(path)], text=True).strip()


def pixel(path, x, y):
    bmp = path.read_bytes()
    assert bmp[:2] == b"BM"
    offset = struct.unpack_from("<I", bmp, 10)[0]
    width, height = struct.unpack_from("<ii", bmp, 18)
    depth = struct.unpack_from("<H", bmp, 28)[0]
    assert width == 640 and height == -480 and depth == 32
    return bmp[offset + (y * width + x) * 4:offset + (y * width + x) * 4 + 3]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data", type=Path, required=True, help="isolated imported test data")
    parser.add_argument("--level", type=int, choices=(1, 2), default=2)
    args = parser.parse_args()
    if not args.data.is_dir():
        parser.error("--data must be an existing directory")
    root = Path(__file__).resolve().parent.parent
    output = root / "work" / "test"
    output.mkdir(parents=True, exist_ok=True)
    opening = ["--click", f"{435 if args.level == 1 else 517},75,2000", "--click", "280,30,2500"]
    scenarios = {
        "base": [],
        "open": opening,
        "close-x": [*opening, "--click", "530,131,2700"],
        "close-hotspot": [*opening, "--click", "400,435,2700"],
    }
    colors = {}
    for name, actions in scenarios.items():
        path = output / f"about-level{args.level}-{name}.bmp"
        command = [str(root / "build" / "crayons.exe"), "--data", windows_path(args.data.resolve()),
                   "--snapshot", windows_path(path), "--level", str(args.level), "--time", "3200", *actions]
        subprocess.run(command, check=True, capture_output=True, timeout=30)
        colors[name] = pixel(path, 400, 200)
    assert colors["open"] != colors["base"], "About dialog did not open"
    assert colors["close-hotspot"] == colors["base"], "original About hotspot did not close"
    assert colors["close-x"] == colors["base"], "About title-bar X did not close"
    print(f"Level {args.level}: About opens; original hotspot and title-bar X both close it")


if __name__ == "__main__":
    main()
