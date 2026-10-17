"""Print the cases of a `case X of` jump table in a function, as pcalls-style listings.
Usage: python3 tools/switch_cases.py sub_2_0d0a [disasm_dir] [--exe EXE]
Finds `sub ax,K / cmp ax,N / ... jmp cs:[bx+T]` and prints each case value's block until the
next jump to the shared end."""
import os
import re
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from nelib import Ne  # noqa: E402

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")


def main():
    func = sys.argv[1]
    asm_dir = sys.argv[2] if len(sys.argv) > 2 and not sys.argv[2].startswith("--") else os.path.join(ROOT, "work", "disasm1")
    exe = sys.argv[sys.argv.index("--exe") + 1] if "--exe" in sys.argv else os.path.join(ROOT, "original", "extracted", "app", "CRAYON1.EXE")
    seg = int(func.split("_")[1])
    code = Ne(exe).segments[seg - 1].data
    listing = subprocess.check_output([sys.executable, "-I", os.path.join(ROOT, "tools", "pcalls.py"), func, asm_dir], text=True).split("\n")
    raw_all = open(os.path.join(asm_dir, f"seg{seg}.asm")).read().split("\n")
    start = next(i for i, l in enumerate(raw_all) if re.match(rf"^{func}( <[^>]*>)?:", l))
    end = next((i for i in range(start + 1, len(raw_all)) if re.match(r"^sub_\d+_[0-9a-f]+( <[^>]*>)?:$", raw_all[i])), len(raw_all))
    raw = raw_all[start:end]
    # locate the dispatch
    sub = cmp = table = None
    for l in raw:
        m = re.search(r"\s([0-9a-f]+)\s+sub\s+ax,0x([0-9a-f]+)$", l)
        if m:
            sub = int(m.group(2), 16)
        m = re.search(r"cmp\s+ax,0x([0-9a-f]+)$", l)
        if m:
            cmp = int(m.group(1), 16)
        m = re.search(r"jmp\s+WORD PTR cs:\[bx([+-])0x([0-9a-f]+)\]", l)
        if m and table is None and cmp is not None:
            table = (int(m.group(2), 16) * (1 if m.group(1) == "+" else -1)) & 0xFFFF
            break
    if table is None:
        print("no jump table found")
        return
    sub = sub or 0
    targets = [struct.unpack_from("<H", code, table + 2 * k)[0] for k in range(cmp + 1)]
    lines = [l for l in listing if re.match(r"^\s+[0-9a-f]{4}\s", l)]
    addrs = [int(l.split()[0], 16) for l in lines]
    counts = {}
    for t in targets:
        counts[t] = counts.get(t, 0) + 1
    default = max(counts, key=counts.get)
    print(f"{func}: case base {sub}, {cmp + 1} entries, table cs:{table:04x}, default/end {default:04x}")
    seen = {}
    for k, t in enumerate(targets):
        value = k + sub
        if t == default:
            continue
        if t in seen:
            print(f"case {value} (0x{value:x}): same as {seen[t]}")
            continue
        seen[t] = value
        print(f"case {value} (0x{value:x}) @{t:04x}:")
        i = next((j for j, a in enumerate(addrs) if a >= t), len(lines))
        n = 0
        while i < len(lines) and n < 40:
            print("   ", lines[i].strip())
            if re.search(rf"jmp 0x{default:x}\b", lines[i]) or re.search(r"retf|\sret\b", lines[i]):
                break
            nxt = addrs[i + 1] if i + 1 < len(addrs) else None
            if nxt is not None and nxt in counts and nxt != t:
                break
            i += 1
            n += 1


if __name__ == "__main__":
    main()
