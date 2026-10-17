"""Crude PDF text extractor (stdlib): inflates content streams and prints Tj/TJ strings per page.
Usage: python3 tools/pdftext.py file.pdf > out.txt"""
import re
import sys
import zlib


def strings_in(content):
    out = []
    for m in re.finditer(rb"\((?:\\.|[^\\)])*\)\s*Tj|\[(.*?)\]\s*TJ|T\*|Td|TD|'|\"", content, re.S):
        tok = m.group(0)
        if tok.endswith(b"Tj"):
            out.append(unescape(tok[1:tok.rindex(b")")]))
        elif tok.endswith(b"TJ"):
            parts = re.findall(rb"\((?:\\.|[^\\)])*\)|-?\d+\.?\d*", m.group(1))
            s = ""
            for p in parts:
                if p.startswith(b"("):
                    s += unescape(p[1:-1])
                elif float(p) < -200:
                    s += " "
            out.append(s)
        else:
            out.append("\n")
    return "".join(out)


def unescape(s):
    s = re.sub(rb"\\([0-7]{1,3})", lambda m: bytes([int(m.group(1), 8)]), s)
    s = s.replace(rb"\(", b"(").replace(rb"\)", b")").replace(rb"\\", b"\\")
    return s.decode("latin-1")


def lzw(data):
    """PDF LZWDecode (MSB-first codes, 9..12 bits, EarlyChange 1)."""
    out, table, width, prev = bytearray(), [bytes([i]) for i in range(256)] + [b"", b""], 9, None
    bits = nbits = 0
    for byte in data:
        bits, nbits = (bits << 8) | byte, nbits + 8
        while nbits >= width:
            nbits -= width
            code = (bits >> nbits) & ((1 << width) - 1)
            if code == 256:
                table, width, prev = table[:258], 9, None
                continue
            if code == 257:
                return bytes(out)
            if prev is None:
                entry = table[code]
            else:
                entry = table[code] if code < len(table) else prev + prev[:1]
                table.append(prev + entry[:1])
            out += entry
            prev = entry
            if len(table) + 1 >= (1 << width) and width < 12:
                width += 1
    return bytes(out)


def a85(data):
    data = re.sub(rb"\s", b"", data)
    if data.endswith(b"~>"):
        data = data[:-2]
    out, group = bytearray(), []
    for c in data:
        if c == ord("z") and not group:
            out += b"\0\0\0\0"
            continue
        group.append(c - 33)
        if len(group) == 5:
            v = 0
            for g in group:
                v = v * 85 + g
            out += v.to_bytes(4, "big")
            group = []
    if group:
        n = len(group)
        group += [84] * (5 - n)
        v = 0
        for g in group:
            v = v * 85 + g
        out += v.to_bytes(4, "big")[:n - 1]
    return bytes(out)


def main():
    data = open(sys.argv[1], "rb").read()
    for m in re.finditer(rb"<<((?:(?!>>).)*?)>>\s*stream\r?\n(.*?)\r?\nendstream", data, re.S):
        dic, raw = m.group(1), m.group(2)
        if b"/Subtype /Image" in dic or b"DCTDecode" in dic:
            continue
        try:
            if b"ASCII85Decode" in dic:
                raw = a85(raw)
            if b"LZWDecode" in dic:
                raw = lzw(raw)
            elif b"FlateDecode" in dic:
                raw = zlib.decompress(raw)
        except Exception:
            continue
        if b"BT" in raw:
            t = strings_in(raw)
            t = re.sub(r"\n{2,}", "\n", t)
            if t.strip():
                print(t)


if __name__ == "__main__":
    main()
