"""Diff one function across the two builds in pcalls form, with build 2's DS globals shifted.
Usage: python3 -I tools/fndiff.py sub_2_6390 sub_2_7aad [--shift 14]"""
import difflib
import os
import re
import subprocess
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")


def listing(func, asm_dir, shift):
    out = subprocess.check_output([sys.executable, "-I", os.path.join(ROOT, "tools", "pcalls.py"), func, asm_dir], text=True)
    lines = []
    for l in out.split("\n")[1:]:
        m = re.match(r"^\s+([0-9a-f]{4})\s+(.*)$", l)
        if not m:
            continue
        t = re.sub(r"\s*\[ax=.*\]$", "", m.group(2))

        def ds(mm):
            v = int(mm.group(2), 16)
            return f"{mm.group(1)}{v - shift:x}" if v >= 0xf00 else mm.group(0)
        t = re.sub(r"(ds:0x|\bg)([0-9a-f]{3,4})\b", ds, t)
        t = re.sub(r"\b(j\w+) 0x[0-9a-f]+", r"\1 L", t)
        t = re.sub(r"ret_[0-9a-f]+", "ret", t)
        lines.append((m.group(1), t))
    return lines


def main():
    shift = int(sys.argv[sys.argv.index("--shift") + 1]) if "--shift" in sys.argv else 14
    a = listing(sys.argv[1], os.path.join(ROOT, "work", "disasm1"), 0)
    b = listing(sys.argv[2], os.path.join(ROOT, "work", "disasm2"), shift)
    sm = difflib.SequenceMatcher(None, [t for _, t in a], [t for _, t in b], autojunk=False)
    for op, i1, i2, j1, j2 in sm.get_opcodes():
        if op == "equal":
            continue
        print(f"--- {op} C1 {a[i1][0] if i1 < len(a) else 'end'}..  C2 {b[j1][0] if j1 < len(b) else 'end'}..")
        for k in range(i1, i2):
            print(f"  - {a[k][0]} {a[k][1]}")
        for k in range(j1, j2):
            print(f"  + {b[k][0]} {b[k][1]}")


if __name__ == "__main__":
    main()
