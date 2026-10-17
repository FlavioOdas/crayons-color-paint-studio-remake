"""Contact sheet of BMP files: python3 tools/contact.py out.png max_cell_w file...  (labels omitted; order = args)"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bmp2png import decode  # noqa: E402
from png import write_rgb  # noqa: E402


def main():
    out, cell = sys.argv[1], int(sys.argv[2])
    imgs = []
    for p in sys.argv[3:]:
        try:
            r = decode(open(p, "rb").read())
        except Exception:
            r = None
        if r:
            imgs.append((os.path.basename(p), r))
    sheet_w = 1400
    x = y = row_h = 0
    placed = []
    for name, (w, h, px, pal) in imgs:
        s = max(1, -(-w // cell))  # integer downscale so the cell fits
        cw, ch = w // s, h // s
        if x + cw > sheet_w:
            x, y, row_h = 0, y + row_h + 4, 0
        placed.append((x, y, s, w, h, px, pal, cw, ch, name))
        x += cw + 4
        row_h = max(row_h, ch)
    H = y + row_h
    canvas = bytearray([128]) * (sheet_w * H * 3)
    for x0, y0, s, w, h, px, pal, cw, ch, name in placed:
        for yy in range(ch):
            for xx in range(cw):
                c = pal[px[(yy * s) * w + xx * s]]
                o = ((y0 + yy) * sheet_w + x0 + xx) * 3
                canvas[o:o + 3] = bytes(c)
        print(f"{name}: at {x0},{y0} scale 1/{s}")
    write_rgb(out, sheet_w, H, bytes(canvas))


if __name__ == "__main__":
    main()
