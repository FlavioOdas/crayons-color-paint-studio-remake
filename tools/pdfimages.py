"""Extract image XObjects from the CD's PDF manual (DCT -> .jpg, LZW/raw RGB/gray/indexed -> .png).
Usage: python3 tools/pdfimages.py file.pdf out_dir"""
import os
import re
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pdftext import a85, lzw  # noqa: E402
from png import write_rgb  # noqa: E402


def main():
    data = open(sys.argv[1], "rb").read()
    out = sys.argv[2]
    os.makedirs(out, exist_ok=True)
    objs = {int(m.group(1)): m.group(2) for m in re.finditer(rb"(\d+) 0 obj(.*?)endobj", data, re.S)}
    n = 0
    for num, body in objs.items():
        m = re.match(rb"\s*<<(.*?)>>\s*stream\r?\n(.*?)\r?\nendstream", body, re.S)
        if not m or b"/Subtype /Image" not in m.group(1):
            continue
        dic, raw = m.group(1), m.group(2)
        w = int(re.search(rb"/Width (\d+)", dic).group(1))
        h = int(re.search(rb"/Height (\d+)", dic).group(1))
        if b"ASCII85Decode" in dic:
            raw = a85(raw)
        if b"DCTDecode" in dic:
            open(os.path.join(out, f"img{num}.jpg"), "wb").write(raw)
            n += 1
            continue
        if b"LZWDecode" in dic:
            raw = lzw(raw)
        elif b"FlateDecode" in dic:
            raw = zlib.decompress(raw)
        bpc = int(re.search(rb"/BitsPerComponent (\d+)", dic).group(1))
        cs = re.search(rb"/ColorSpace\s*(/\w+|\[.*?\]|\d+ 0 R)", dic, re.S).group(1)
        rgb = bytearray()
        if cs.startswith(b"[") and b"/Indexed" in cs:
            hival = int(re.search(rb"/Indexed\s*/\w+\s*(\d+)", cs).group(1))
            lut_m = re.search(rb"<([0-9a-fA-F\s]+)>", cs)
            if lut_m:
                lut = bytes.fromhex(re.sub(rb"\s", b"", lut_m.group(1)).decode())
            else:
                ref = re.search(rb"(\d+) 0 R\s*\]", cs)
                lb = objs[int(ref.group(1))]
                lm = re.match(rb"\s*<<(.*?)>>\s*stream\r?\n(.*?)\r?\nendstream", lb, re.S)
                lut = lm.group(2)
                if b"ASCII85Decode" in lm.group(1):
                    lut = a85(lut)
                if b"LZWDecode" in lm.group(1):
                    lut = lzw(lut)
            stride = (w * bpc + 7) // 8
            for y in range(h):
                row = raw[y * stride:(y + 1) * stride]
                for x in range(w):
                    if bpc == 8:
                        i = row[x] if x < len(row) else 0
                    else:
                        bit = x * bpc
                        i = (row[bit // 8] >> (8 - bpc - bit % 8)) & ((1 << bpc) - 1) if bit // 8 < len(row) else 0
                    rgb += lut[3 * i:3 * i + 3] if 3 * i + 3 <= len(lut) else b"\0\0\0"
        elif b"DeviceRGB" in cs or b"CalRGB" in cs or re.match(rb"\d+ 0 R", cs):
            rgb = bytearray(raw[:w * h * 3].ljust(w * h * 3, b"\0"))
        elif b"DeviceGray" in cs or b"CalGray" in cs:
            if bpc == 1:
                stride = (w + 7) // 8
                for y in range(h):
                    for x in range(w):
                        v = 255 if raw[y * stride + x // 8] >> (7 - x % 8) & 1 else 0
                        rgb += bytes((v, v, v))
            else:
                for v in raw[:w * h]:
                    rgb += bytes((v, v, v))
        else:
            print("skip", num, cs[:40])
            continue
        write_rgb(os.path.join(out, f"img{num}.png"), w, h, bytes(rgb[:w * h * 3]))
        n += 1
    print(n, "images")


if __name__ == "__main__":
    main()
