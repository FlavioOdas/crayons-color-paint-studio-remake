"""Deterministic headless input soak against an explicitly selected test-data directory.

Usage: python3 -I tools/soak.py --data /mnt/c/.../Temp/Crayons-test/data
Outputs stay in ignored work/test/. Never point --data at the player's AppData directory.
"""
import argparse
from pathlib import Path
import random
import subprocess


def windows_path(path):
    return subprocess.check_output(["wslpath", "-w", str(path)], text=True).strip()


def events(level, count):
    rng = random.Random(1700 + level)
    result = []
    for index in range(count):
        at = 1000 + index * 160
        kind = rng.randrange(10)
        if kind == 0:
            tab = rng.randrange(9 if level == 1 else 11)
            result.extend(("--click", f"{107 + 41 * tab},75,{at}"))
        elif kind == 1:
            result.extend(("--click", f"{120 + 40 * rng.randrange(10)},30,{at}"))
        elif kind == 2:
            result.extend(("--click", f"{rng.randrange(10, 70)},{rng.randrange(230, 425)},{at}"))
        elif kind == 3:
            result.extend(("--rclick", f"{rng.randrange(85, 625)},{rng.randrange(100, 470)},{at}"))
        elif kind == 4:
            result.extend(("--type", f"Hi\\r,{at}"))
        elif kind <= 6:
            result.extend(("--drag", f"{rng.randrange(90, 590)},{rng.randrange(100, 430)},"
                                     f"{rng.randrange(90, 590)},{rng.randrange(100, 430)},{at}"))
        else:
            result.extend(("--click", f"{rng.randrange(90, 630)},{rng.randrange(100, 470)},{at}"))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data", required=True, type=Path, help="isolated imported test data, never player data")
    parser.add_argument("--events", type=int, default=200)
    args = parser.parse_args()
    if not args.data.is_dir() or args.events < 1:
        parser.error("--data must be an existing directory and --events must be positive")
    root = Path(__file__).resolve().parent.parent
    output_dir = root / "work" / "test"
    output_dir.mkdir(parents=True, exist_ok=True)
    for level in (1, 2):
        output = output_dir / f"soak-level{level}.bmp"
        output.unlink(missing_ok=True)
        command = [str(root / "build" / "crayons.exe"), "--data", windows_path(args.data.resolve()),
                   "--snapshot", windows_path(output), "--level", str(level),
                   "--time", str(2000 + args.events * 160), *events(level, args.events)]
        run = subprocess.run(command, capture_output=True, text=True, timeout=120)
        diagnostics = run.stdout + run.stderr
        print(f"level {level}: exit={run.returncode}, snapshot={output.stat().st_size if output.exists() else 0} bytes")
        if diagnostics:
            print(diagnostics.rstrip())
        if run.returncode or not output.exists() or "not ported yet" in diagnostics:
            raise SystemExit(1)


if __name__ == "__main__":
    main()
