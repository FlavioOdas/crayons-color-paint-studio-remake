"""Convert BMP files (1/4/8-bit, plain or RLE) to PNG, including Delphi TPicture blobs.

Usage: python3 tools/bmp2png.py out_dir file...   (writes out_dir/<name>.png)
Data files with other extensions (.ASL) are converted if they are BMPs and skipped otherwise.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from png import write_indexed, write_rgb  # noqa: E402


def unwrap(b):
    """A Delphi TPicture.Data blob is <shortstring class name><u32 size><file>."""
    if b[:2] != b"BM" and b and b[0] < 32 and b[1:1 + b[0]].isalpha():
        n = b[0]
        b = b[1 + n + (4 if b[1:1 + n] == b"TBitmap" else 0):]
    return b


def decode(b):
    b = unwrap(b)
    if b[:2] != b"BM":
        return None
    bits, = struct.unpack_from("<I", b, 10)
    hsize, w, h, _planes, bpp, comp, _size, _xr, _yr, used = struct.unpack_from("<IiiHHIIiiI", b, 14)
    if hsize == 12:  # OS/2 core header
        w, h, _p, bpp = struct.unpack_from("<HHHH", b, 18)
        comp, used, entry = 0, 0, 3
    else:
        entry = 4
    ncol = used or (1 << bpp if bpp <= 8 else 0)
    pal_at = 14 + hsize
    palette = [(b[pal_at + i * entry + 2], b[pal_at + i * entry + 1], b[pal_at + i * entry]) for i in range(ncol)]
    palette += [(0, 0, 0)] * (256 - len(palette))
    flip = h > 0
    h = abs(h)
    px = bytearray(w * h)
    data = b[bits:]
    if comp == 0:
        stride = (w * bpp + 31) // 32 * 4
        for y in range(h):
            row = data[y * stride:(y + 1) * stride]
            for x in range(w):
                if bpp == 8:
                    v = row[x]
                elif bpp == 4:
                    v = (row[x >> 1] >> (4 if x % 2 == 0 else 0)) & 15
                elif bpp == 1:
                    v = (row[x >> 3] >> (7 - x % 8)) & 1
                else:
                    raise ValueError(f"{bpp} bpp")
                px[y * w + x] = v
    elif comp in (1, 2):  # RLE8 / RLE4
        x = y = i = 0
        while i + 1 < len(data):
            n, v = data[i], data[i + 1]
            i += 2
            if n:
                for k in range(n):
                    c = v if comp == 1 else (v >> 4 if k % 2 == 0 else v & 15)
                    if x < w and y < h:
                        px[y * w + x] = c
                    x += 1
            elif v == 0:
                x, y = 0, y + 1
            elif v == 1:
                break
            elif v == 2:
                x, y = x + data[i], y + data[i + 1]
                i += 2
            else:
                for k in range(v):
                    c = data[i + k] if comp == 1 else (data[i + k // 2] >> 4 if k % 2 == 0 else data[i + k // 2] & 15)
                    if x < w and y < h:
                        px[y * w + x] = c
                    x += 1
                used_bytes = v if comp == 1 else (v + 1) // 2
                i += used_bytes + (used_bytes & 1)
    else:
        raise ValueError(f"compression {comp}")
    if flip:
        px = b"".join(px[(h - 1 - y) * w:(h - y) * w] for y in range(h))
    return w, h, bytes(px), palette


def decode_rgb(b):
    """24/32-bit BMP -> (w, h, RGB bytes); None for other depths."""
    b = unwrap(b)
    if b[:2] != b"BM":
        return None
    bits, = struct.unpack_from("<I", b, 10)
    _hs, w, h, _p, bpp, comp = struct.unpack_from("<IiiHHI", b, 14)
    if bpp not in (24, 32) or comp != 0:
        return None
    stride = (w * bpp // 8 + 3) // 4 * 4
    rows = range(abs(h) - 1, -1, -1) if h > 0 else range(abs(h))
    out = bytearray()
    for y in rows:
        row = b[bits + y * stride:bits + (y + 1) * stride]
        step = bpp // 8
        for x in range(w):
            px = row[x * step:x * step + 3]
            out += bytes((px[2], px[1], px[0]))
    return w, abs(h), bytes(out)


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    for path in sys.argv[2:]:
        raw = open(path, "rb").read()
        rgb = decode_rgb(raw)
        if rgb:
            write_rgb(os.path.join(out, os.path.basename(path) + ".png"), *rgb)
            continue
        try:
            r = decode(raw)
        except Exception as e:  # report and keep going
            print(f"{path}: {e}")
            continue
        if r:
            w, h, px, pal = r
            write_indexed(os.path.join(out, os.path.basename(path) + ".png"), w, h, px, pal)


if __name__ == "__main__":
    main()
