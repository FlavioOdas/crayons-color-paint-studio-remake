"""Print every `case` jump table of a function (nested ones too) with each case's code.
Usage: python3 -I tools/all_cases.py sub_2_868c [disasm_dir] [--exe EXE] [--max 40]
Each table is found from `[sub ax,K] / cmp ax,N / ja / ... jmp WORD PTR cs:[bx+T]`; a case's block
runs (pcalls listing) until a jump to the table's end, a return, or the next case label."""
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
    limit = int(sys.argv[sys.argv.index("--max") + 1]) if "--max" in sys.argv else 40
    seg = int(func.split("_")[1])
    code = Ne(exe).segments[seg - 1].data
    listing = subprocess.check_output([sys.executable, "-I", os.path.join(ROOT, "tools", "pcalls.py"), func, asm_dir], text=True).split("\n")
    raw_all = open(os.path.join(asm_dir, f"seg{seg}.asm")).read().split("\n")
    start = next(i for i, l in enumerate(raw_all) if re.match(rf"^{func}( <[^>]*>)?:", l))
    end = next((i for i in range(start + 1, len(raw_all)) if re.match(r"^sub_\d+_[0-9a-f]+( <[^>]*>)?:$", raw_all[i])), len(raw_all))
    raw = raw_all[start:end]
    lines = [l for l in listing if re.match(r"^\s+[0-9a-f]{4}\s", l)]
    addrs = [int(l.split()[0], 16) for l in lines]
    tables = []
    sub = cmp = None
    jaTarget = None
    after_jbe = False
    for l in raw:
        m = re.match(r"^\s+([0-9a-f]+)\s+(.*)$", l)
        if not m:
            continue
        ins = m.group(2).split(";")[0].strip()
        if re.match(r"sub\s+ax,0x[0-9a-f]+$", ins):
            sub = int(ins.split("0x")[1], 16)
        elif re.match(r"cmp\s+ax,0x[0-9a-f]+$", ins):
            cmp = int(ins.split("0x")[1], 16)
            jaTarget, after_jbe = None, False
        elif re.match(r"ja\s+0x", ins):
            jaTarget = int(ins.split("0x")[1], 16)
        elif re.match(r"jbe\s+0x", ins):
            after_jbe = True
        elif after_jbe and re.match(r"jmp\s+0x[0-9a-f]+$", ins):
            jaTarget, after_jbe = int(ins.split("0x")[1], 16), False
        mm = re.search(r"jmp\s+WORD PTR cs:\[bx([+-])0x([0-9a-f]+)\]", ins)
        if mm and cmp is not None:
            t = (int(mm.group(2), 16) * (1 if mm.group(1) == "+" else -1)) & 0xFFFF
            tables.append((int(m.group(1), 16), t, sub or 0, cmp, jaTarget))
            sub = cmp = jaTarget = None
        elif re.match(r"(mov|xor|call|push)\b", ins) and not ins.startswith("mov    bx") and not ins.startswith("mov bx"):
            pass
    labels = set()
    for at, t, base, n, ja in tables:
        for k in range(n + 1):
            labels.add(struct.unpack_from("<H", code, t + 2 * k)[0])
    for at, t, base, n, ja in tables:
        targets = [struct.unpack_from("<H", code, t + 2 * k)[0] for k in range(n + 1)]
        counts = {}
        for x in targets:
            counts[x] = counts.get(x, 0) + 1
        default = ja if ja is not None else max(counts, key=counts.get)
        print(f"==== table at {at:04x}: cs:{t:04x}, base {base}, {n + 1} entries, end {default:04x}")
        seen = {}
        for k, x in enumerate(targets):
            v = k + base
            if x == default:
                continue
            if x in seen:
                print(f"  case {v}: same as {seen[x]}")
                continue
            seen[x] = v
            print(f"  case {v} (0x{v:x}) @{x:04x}:")
            i = next((j for j, a in enumerate(addrs) if a >= x), len(lines))
            c = 0
            while i < len(lines) and c < limit:
                print("     ", lines[i].strip())
                if re.search(rf"jmp 0x{default:x}\b", lines[i]) or re.search(r"retf|\sret\b", lines[i]):
                    break
                nxt = addrs[i + 1] if i + 1 < len(addrs) else None
                if nxt is not None and nxt in labels and nxt != x:
                    break
                i += 1
                c += 1


if __name__ == "__main__":
    main()
