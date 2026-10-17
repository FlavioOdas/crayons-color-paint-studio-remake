"""Annotate ne_disasm output with names from docs/re/names.tsv (`far sub_13_177b` gets
`<TControl_SetLeft>`) and with the Pascal string constants Delphi code pushes as
`mov di,OFF / push cs` (constants live in the code segment) or `mov di,OFF / push ds`
(typed constants/globals in DGROUP). Rewrites work/disasmN/segK.asm in place (idempotent).

Usage: python3 tools/pstr_annotate.py original/extracted/app/CRAYON1.EXE work/disasm1
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from nelib import Ne  # noqa: E402

MOV = re.compile(r"^(\s+[0-9a-f]+\s+mov\s+(?:di|ax|si),0x([0-9a-f]+))(\s*;.*)?$")


def pstr(data, off):
    if off >= len(data):
        return None
    n = data[off]
    s = bytes(data[off + 1:off + 1 + n])
    if n == 0 or len(s) != n or not all(32 <= c < 127 for c in s):
        return None
    return s.decode("latin-1")


def load_names():
    names = {}
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "docs", "re", "names.tsv")
    for line in open(path):
        if line.startswith("#") or not line.strip():
            continue
        addr, name = line.split("\t")[:2]
        sel, off = addr.split(":")
        names[f"sub_{(int(sel, 16) - 0x1000) // 8 + 1}_{off.lower()}"] = name.strip()
    return names


def main():
    ne = Ne(sys.argv[1])
    out = sys.argv[2]
    names = load_names()
    label = re.compile(r"\b(sub_\d+_[0-9a-f]+)\b(?: <[^>]*>)?")
    dgroup = ne.segments[ne.autodata - 1].data
    for seg in ne.segments:
        path = os.path.join(out, f"seg{seg.number}.asm")
        if not os.path.exists(path):
            continue
        lines = open(path).read().split("\n")
        lines = [label.sub(lambda m: m.group(1) + (f" <{names[m.group(1)]}>" if m.group(1) in names else ""), l)
                 for l in lines]
        for i, line in enumerate(lines[:-1]):
            m = MOV.match(line)
            if not m or "pstr" in (m.group(3) or ""):
                continue
            nxt = lines[i + 1]
            source = seg.data if re.search(r"push\s+cs$", nxt) else dgroup if re.search(r"push\s+ds$", nxt) else None
            if source is None:
                continue
            s = pstr(source, int(m.group(2), 16))
            if s:
                lines[i] = f"{m.group(1)}    ; pstr {'cs' if source is seg.data else 'ds'} {s!r}"
        open(path, "w").write("\n".join(lines))


if __name__ == "__main__":
    main()
