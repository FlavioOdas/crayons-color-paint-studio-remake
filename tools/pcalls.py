"""Readable call listing for Delphi 1 (Pascal convention) code from ne_disasm output.

Tracks pushes and simple register loads, and at every call prints `name(arg1, arg2, ...)`
with arguments in Pascal order (first pushed = first argument; Self is pushed last for
methods). Far pointers pushed as `push es; push di` after `les di,X` show as `&X`/`X`.
Stores, compares and jumps are kept so control flow stays readable.

Usage: python3 tools/pcalls.py sub_2_6390 [disasm_dir=work/disasm1]
"""
import os
import re
import sys

# Bytes of arguments (incl. the far Self pointer) for virtual methods seen so far:
# TGraphic/TBitmap 18=GetHeight 1c=GetWidth 24=SetHeight 28=SetWidth 30=LoadFromFile,
# TControl 4c=SetBounds, TStrings c=Get(Index).
VMT_ARG_BYTES = {0x18: 4, 0x1c: 4, 0x24: 6, 0x28: 6, 0x30: 8, 0x4c: 12, 0x0c: 10}
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
INSN = re.compile(r"^\s*([0-9a-f]+)\s+(\S+)\s*(.*?)\s*(?:;\s*(.*))?$")


def retf_sizes(asm_dir):
    sizes = {}
    for name in os.listdir(asm_dir):
        if not name.endswith(".asm"):
            continue
        cur = None
        for line in open(os.path.join(asm_dir, name)):
            m = re.match(r"^(sub_\d+_[0-9a-f]+)", line)
            if m:
                cur = m.group(1)
                continue
            m = re.search(r"\sretf\s+0x([0-9a-f]+)", line)
            if cur and m and cur not in sizes:
                sizes[cur] = int(m.group(1), 16)
            elif cur and re.search(r"\sretf\s*$", line) and cur not in sizes:
                sizes[cur] = 0
    return sizes


def function_lines(asm_dir, func):
    seg = func.split("_")[1]
    out, on = [], False
    for line in open(os.path.join(asm_dir, f"seg{seg}.asm")):
        line = line.rstrip("\n")
        if re.match(rf"^{func}( <[^>]*>)?:", line):
            on = True
            out.append(line)
            continue
        if on and re.match(r"^sub_\d+_[0-9a-f]+( <[^>]*>)?:$", line):
            break
        if on:
            out.append(line)
    return out


def main():
    func = sys.argv[1]
    asm_dir = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, "work", "disasm1")
    sizes = retf_sizes(asm_dir)
    lines = function_lines(asm_dir, func)
    print(lines[0])
    regs = {}
    pushes = []

    def val(op):
        op = op.strip()
        if op in regs:
            return regs[op]
        m = re.fullmatch(r"WORD PTR \[bp([+-])0x([0-9a-f]+)\]", op)
        if m:
            return ("arg" if m.group(1) == "+" else "loc") + m.group(2)
        m = re.fullmatch(r"(?:WORD PTR )?ds:0x([0-9a-f]+)", op)
        if m:
            return "g" + m.group(1)
        m = re.fullmatch(r"0x([0-9a-f]+)", op)
        if m:
            v = int(m.group(1), 16)
            return str(v - 0x10000 if v >= 0xff80 and v != 0xffff else v)
        return op

    for line in lines[1:]:
        m = INSN.match(line)
        if not m:
            continue
        addr, op, args, comment = m.group(1), m.group(2), m.group(3), m.group(4) or ""
        if op == "push":
            if args == "es" and pushes and False:
                pass
            if args in ("es", "ss", "ds", "cs"):
                pushes.append(("seg", args))
            elif args == "di" and pushes and pushes[-1][0] == "seg":
                seg = pushes.pop()[1]
                base = regs.get("es:di" if seg == "es" else "di", "di")
                if seg == "ss":
                    base = regs.get("di", "di")
                    base = base if base.startswith("&") else "&" + base
                elif seg in ("ds", "cs"):
                    s = re.search(r"pstr \w+ ('.*')", regs.get("di_comment", ""))
                    base = s.group(1) if s else (seg + ":" + regs.get("di", "di"))
                pushes.append(("ptr", base))
            else:
                pushes.append(("w", val(args)))
            continue
        if op == "pop":
            continue
        if op == "les":
            dst, src = [a.strip() for a in args.split(",", 1)]
            m2 = re.fullmatch(r"DWORD PTR (?:es:)?\[(di)\+0x([0-9a-f]+)\]", src)
            if m2:
                regs["es:" + dst] = f"{regs.get('es:di', '?')}.f{m2.group(2)}"
            elif re.fullmatch(r"DWORD PTR es:\[di\]", src):
                regs["es:" + dst] = f"{regs.get('es:di', '?')}.vmt"
            else:
                regs["es:" + dst] = val(src.replace("DWORD PTR ", "WORD PTR "))
            continue
        if op == "lea":
            dst, src = [a.strip() for a in args.split(",", 1)]
            m2 = re.fullmatch(r"\[bp([+-])0x([0-9a-f]+)\]", src)
            regs[dst] = ("&arg" if m2.group(1) == "+" else "&loc") + m2.group(2) if m2 else src
            continue
        if op == "mov":
            dst, src = [a.strip() for a in args.split(",", 1)]
            if dst in ("ax", "bx", "cx", "dx", "si", "di", "al", "es"):
                regs[dst] = val(src)
                if dst == "di":
                    regs["di_comment"] = comment
                if dst == "di":
                    regs.pop("es:di", None)
                continue
        if op == "call":
            target = re.search(r"(sub_\d+_[0-9a-f]+)(?: <([^>]*)>)?|([A-Z0-9]+\.\w+)", comment)
            vm = re.search(r"DWORD PTR es:\[di\+0x([0-9a-f]+)\]", args)
            if vm:
                obj = regs.get("es:di", "?").removesuffix(".vmt")
                name = f"{obj}.vmt[{vm.group(1)}]"
                n = VMT_ARG_BYTES.get(int(vm.group(1), 16))
            elif target:
                name = target.group(2) or target.group(1) or target.group(3)
                n = sizes.get(target.group(1)) if target.group(1) else None
            else:
                near = re.fullmatch(r"0x([0-9a-f]+)", args)
                name = f"sub_{func.split('_')[1]}_{near.group(1)}" if near else args
                n = None
            if n is not None:
                words = n // 2
                taken = []
                while words > 0 and pushes:
                    k, v = pushes.pop()
                    taken.insert(0, v)
                    words -= 2 if k == "ptr" else 1
            else:
                taken = [v for _, v in pushes]
                pushes = []
            print(f"  {addr}  {name}({', '.join(taken)})")
            regs["ax"] = f"ret_{addr}"
            regs["dx"] = f"ret_{addr}.hi"
            regs["al"] = f"ret_{addr}"
            continue
        if op in ("enter", "leave", "nop") or op.startswith("push"):
            continue
        # simple register arithmetic, so pushed expressions stay exact
        arith = {"add": "+", "sub": "-", "imul": "*", "shl": "<<", "sar": ">>", "shr": ">>", "and": "&", "or": "|"}
        if op in ("inc", "dec", "neg") and args in ("ax", "bx", "cx", "dx", "si", "di"):
            cur = regs.get(args, args)
            regs[args] = f"({cur}{'+1' if op == 'inc' else '-1'})" if op != "neg" else f"-({cur})"
        elif op in arith and "," in args:
            dst, src = [a.strip() for a in args.split(",", 1)]
            if dst in ("ax", "bx", "cx", "dx", "si", "di"):
                regs[dst] = f"({regs.get(dst, dst)}{arith[op]}{val(src)})"
        elif op == "imul" and args.count(",") == 2:
            dst, a, b = [x.strip() for x in args.split(",")]
            if dst in ("ax", "bx", "cx", "dx", "si", "di"):
                regs[dst] = f"({val(a)}*{val(b)})"
        elif op == "xor" and args in ("ax,ax", "dx,dx", "cx,cx", "bx,bx"):
            regs[args.split(",")[0]] = "0"
        text = f"{op} {args}".strip()
        if comment and "pstr" in comment:
            text += f"    ; {comment}"
        # show register values used by stores/compares
        print(f"  {addr}    {text}" + (f"    [ax={regs.get('ax')}]" if "ax" in args or "al" in args else ""))


if __name__ == "__main__":
    main()
