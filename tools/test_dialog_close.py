"""Check that modal title-bar close buttons dismiss their dialogs."""

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
    assert (width, height, depth) == (640, -480, 32)
    start = offset + (y * width + x) * 4
    return bmp[start:start + 3]


def snapshot(root, data, level, name, actions):
    path = root / "work" / "test" / f"dialog-close-{name}.bmp"
    command = [str(root / "build" / "crayons.exe"), "--data", data,
               "--snapshot", windows_path(path), "--level", str(level), "--time", "3200", *actions]
    subprocess.run(command, check=True, capture_output=True, timeout=30)
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data", type=Path, required=True, help="isolated imported test data")
    args = parser.parse_args()
    if not args.data.is_dir():
        parser.error("--data must be an existing directory")
    root = Path(__file__).resolve().parent.parent
    (root / "work" / "test").mkdir(parents=True, exist_ok=True)
    data = windows_path(args.data.resolve())
    for level, pc_tab, pictures_tab in ((1, 435, 353), (2, 517, 476)):
        cases = (
            ("jukebox", pc_tab, "320,30,2500", "487,111,2700", (300, 150)),
            ("orders", pc_tab, "360,30,2500", "515,82,2700", (300, 80)),
            ("thumbs", pictures_tab, "120,30,2500", "603,14,2700", (400, 100)),
        )
        for name, tab_x, option, close, probe in cases:
            tab = f"{tab_x},75,2000"
            case = f"level{level}-{name}"
            base = pixel(snapshot(root, data, level, f"{case}-base", ["--click", tab]), *probe)
            opened = pixel(snapshot(root, data, level, f"{case}-open", ["--click", tab, "--click", option]), *probe)
            closed = pixel(snapshot(root, data, level, f"{case}-closed", ["--click", tab, "--click", option,
                                                              "--click", close]), *probe)
            assert opened != base, f"{case} did not open"
            assert closed == base, f"{case} title-bar X did not close"
            print(f"{case}: title-bar X closes")


if __name__ == "__main__":
    main()
