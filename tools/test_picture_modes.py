"""Check that the colorful and monochrome clip-art buttons open matching pictures."""

import argparse
from pathlib import Path
import struct
import subprocess


def windows_path(path):
    return subprocess.check_output(["wslpath", "-w", str(path)], text=True).strip()


def colored_pixels(path):
    bmp = path.read_bytes()
    assert bmp[:2] == b"BM"
    offset = struct.unpack_from("<I", bmp, 10)[0]
    width, height = struct.unpack_from("<ii", bmp, 18)
    depth = struct.unpack_from("<H", bmp, 28)[0]
    assert (width, height, depth) == (640, -480, 32)
    count = 0
    for row in range(40, 190):
        for column in range(90, 285):
            pixel = offset + (row * width + column) * 4
            blue, green, red = bmp[pixel:pixel + 3]
            count += max(red, green, blue) - min(red, green, blue) > 30
    return count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data", type=Path, required=True, help="isolated imported test data")
    args = parser.parse_args()
    if not args.data.is_dir():
        parser.error("--data must be an existing directory")
    root = Path(__file__).resolve().parent.parent
    output = root / "work" / "test"
    output.mkdir(parents=True, exist_ok=True)
    failures = []
    for level, tab in ((1, 6), (2, 9)):
        for option, expected_color in ((0, True), (1, False)):
            path = output / f"picture-level{level}-option{option}.bmp"
            command = [str(root / "build" / "crayons.exe"), "--data", windows_path(args.data.resolve()),
                       "--snapshot", windows_path(path), "--level", str(level), "--time", "3200",
                       "--click", f"{107 + 41 * tab},75,2000", "--click", f"{120 + 40 * option},30,2500"]
            subprocess.run(command, check=True, capture_output=True, timeout=30)
            count = colored_pixels(path)
            print(f"level {level}, option {option}: {count} colored thumbnail pixels")
            if (count > 100) != expected_color:
                failures.append(f"level {level}, option {option}")
    assert not failures, f"wrong clip-art set: {', '.join(failures)}"


if __name__ == "__main__":
    main()
