"""Compare two PNGs pixel by pixel in a region; print mismatch count and samples, write a diff image.
Usage: python3 tools/pixdiff.py a.png b.png [x y w h] [--out diff.png]"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from crop import read_png  # noqa: E402
from png import write_rgb  # noqa: E402


def main():
    args = [a for a in sys.argv[1:]]
    out = None
    if "--out" in args:
        i = args.index("--out")
        out = args[i + 1]
        del args[i:i + 2]
    wa, ha, a = read_png(args[0])
    wb, hb, b = read_png(args[1])
    x0, y0, w, h = (int(v) for v in args[2:6]) if len(args) >= 6 else (0, 0, min(wa, wb), min(ha, hb))
    bad, samples, diff = 0, [], bytearray()
    for y in range(y0, y0 + h):
        for x in range(x0, x0 + w):
            pa = a[(y * wa + x) * 3:(y * wa + x) * 3 + 3]
            pb = b[(y * wb + x) * 3:(y * wb + x) * 3 + 3]
            same = pa == pb
            if not same:
                bad += 1
                if len(samples) < 12:
                    samples.append((x, y, tuple(pa), tuple(pb)))
            diff += pa if same else b"\xff\x00\xff"
    print(f"{bad} of {w * h} pixels differ")
    for s in samples:
        print("  ", s)
    if out:
        write_rgb(out, w, h, bytes(diff))


if __name__ == "__main__":
    main()
