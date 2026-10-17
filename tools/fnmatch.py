"""Pair the functions of two builds (CRAYON1 / CRAYON2 are one code base compiled twice).
Usage: python3 -I tools/fnmatch.py work/disasm1 work/disasm2 [--segs 1,2,3,4,5,6] [--min 0.6]
Normalises each function (addresses, constants, call targets and DS offsets -> placeholders) and
prints for every function of the second build: '=' identical code in the first, '~' the most
similar one with its ratio, '+' nothing similar (new)."""
import difflib
import os
import re
import sys


def functions(asm_dir, segs):
    out = {}
    for seg in segs:
        path = os.path.join(asm_dir, f"seg{seg}.asm")
        if not os.path.exists(path):
            continue
        name, body = None, []
        for line in open(path):
            m = re.match(r"^(sub_\d+_[0-9a-f]+)( <[^>]*>)?:$", line.strip())
            if m:
                if name:
                    out[name] = body
                name, body = m.group(1), []
                continue
            m = re.match(r"^\s+[0-9a-f]+\s+(.*)$", line)
            if name and m:
                ins = m.group(1).split(";")[0].strip()
                note = line.split(";", 1)[1] if ";" in line else ""
                api = re.search(r"far ([A-Z]+\.\w+)", note)
                ins = re.sub(r"sub_\d+_[0-9a-f]+", "S", ins)
                ins = re.sub(r"0x[0-9a-f]+", "N", ins)
                if api:
                    ins += " " + api.group(1)
                body.append(ins)
        if name:
            out[name] = body
    return out


def main():
    a_dir, b_dir = sys.argv[1], sys.argv[2]
    segs = [1, 2, 3, 4, 5, 6]
    if "--segs" in sys.argv:
        segs = [int(s) for s in sys.argv[sys.argv.index("--segs") + 1].split(",")]
    least = float(sys.argv[sys.argv.index("--min") + 1]) if "--min" in sys.argv else 0.6
    a, b = functions(a_dir, range(1, 17)), functions(b_dir, segs)
    index = {}
    for name, body in a.items():
        index.setdefault("\n".join(body), []).append(name)
    for name, body in sorted(b.items(), key=lambda kv: (int(kv[0].split("_")[1]), int(kv[0].split("_")[2], 16))):
        key = "\n".join(body)
        if len(body) < 3:
            continue
        if key in index:
            print(f"= {name:12} {','.join(index[key][:3])}  ({len(body)})")
            continue
        best, ratio = None, 0.0
        for other, obody in a.items():
            if abs(len(obody) - len(body)) > max(20, len(body) // 3):
                continue
            sm = difflib.SequenceMatcher(None, body, obody, autojunk=False)
            if sm.real_quick_ratio() <= ratio or sm.quick_ratio() <= ratio:
                continue
            r = sm.ratio()
            if r > ratio:
                best, ratio = other, r
        if best and ratio >= least:
            print(f"~ {name:12} {best:12} {ratio:.2f}  ({len(body)} vs {len(a[best])})")
        else:
            print(f"+ {name:12} {'':12}       ({len(body)})")


if __name__ == "__main__":
    main()
