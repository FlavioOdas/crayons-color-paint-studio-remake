"""Crop and enlarge part of a PNG (8-bit RGB/RGBA/gray/palette, non-interlaced) for close inspection.
Usage: python3 tools/crop.py in.png out.png x y w h [scale=4]
       python3 tools/crop.py in.png --pixel x y      (print one pixel)"""
import os
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from png import write_rgb  # noqa: E402


def read_png(path):
    b = open(path, "rb").read()
    assert b[:8] == b"\x89PNG\r\n\x1a\n"
    p, idat, plte = 8, b"", None
    while p < len(b):
        n, tag = struct.unpack(">I4s", b[p:p + 8])
        body = b[p + 8:p + 8 + n]
        if tag == b"IHDR":
            w, h, depth, ctype = struct.unpack(">IIBB", body[:10])
        elif tag == b"PLTE":
            plte = body
        elif tag == b"IDAT":
            idat += body
        p += 12 + n
    assert depth == 8, "only 8-bit PNGs"
    ch = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype]
    raw = zlib.decompress(idat)
    stride = w * ch
    out, prev = bytearray(), bytearray(stride)
    for y in range(h):
        f = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - ch] if i >= ch else 0
            c = prev[i - ch] if i >= ch else 0
            up = prev[i]
            if f == 1:
                line[i] = (line[i] + a) & 255
            elif f == 2:
                line[i] = (line[i] + up) & 255
            elif f == 3:
                line[i] = (line[i] + (a + up) // 2) & 255
            elif f == 4:
                pa, pb, pc = abs(up - c), abs(a - c), abs(a + up - 2 * c)
                pr = a if pa <= pb and pa <= pc else up if pb <= pc else c
                line[i] = (line[i] + pr) & 255
        prev = line
        for x in range(w):
            px = line[x * ch:(x + 1) * ch]
            if ctype == 3:
                out += plte[3 * px[0]:3 * px[0] + 3]
            elif ctype in (0, 4):
                out += bytes((px[0],) * 3)
            else:
                out += px[:3]
    return w, h, bytes(out)


def main():
    w, h, rgb = read_png(sys.argv[1])
    if sys.argv[2] == "--pixel":
        x, y = int(sys.argv[3]), int(sys.argv[4])
        print(tuple(rgb[(y * w + x) * 3:(y * w + x) * 3 + 3]))
        return
    x0, y0, cw, chh = map(int, sys.argv[3:7])
    s = int(sys.argv[7]) if len(sys.argv) > 7 else 4
    out = bytearray()
    for y in range(chh * s):
        for x in range(cw * s):
            sx, sy = x0 + x // s, y0 + y // s
            out += rgb[(sy * w + sx) * 3:(sy * w + sx) * 3 + 3] if 0 <= sx < w and 0 <= sy < h else b"\0\0\0"
    write_rgb(sys.argv[2], cw * s, chh * s, bytes(out))


if __name__ == "__main__":
    main()
