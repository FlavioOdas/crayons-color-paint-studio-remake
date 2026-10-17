"""Map the DS globals of a second build to the first through functions with identical shape.
Usage: python3 -I tools/dsmap.py work/disasm1 work/disasm2 work/fnmatch12.txt > work/dsmap12.tsv
For every '=' pair (and '~' pairs of equal length with ratio 1.00) the raw instructions are zipped
and each ds:0xNNNN / [reg+0xNNNN] (>= 0x0f00) operand of build 2 is paired with build 1's."""
import collections
import os
import re
import sys


def raw(asm_dir):
    out = {}
    for f in os.listdir(asm_dir):
        if not f.startswith("seg"):
            continue
        name, body = None, []
        for line in open(os.path.join(asm_dir, f)):
            m = re.match(r"^(sub_\d+_[0-9a-f]+)( <[^>]*>)?:$", line.strip())
            if m:
                if name:
                    out[name] = body
                name, body = m.group(1), []
                continue
            m = re.match(r"^\s+[0-9a-f]+\s+(.*)$", line)
            if name and m:
                body.append(m.group(1).split(";")[0].strip())
        if name:
            out[name] = body
    return out


def ds_operands(ins):
    vals = re.findall(r"ds:0x([0-9a-f]+)", ins)
    vals += re.findall(r"\[(?:bx|di|si)\+0x([0-9a-f]+)\]", ins)
    return [int(v, 16) for v in vals if int(v, 16) >= 0xf00]


def main():
    a, b = raw(sys.argv[1]), raw(sys.argv[2])
    votes = collections.defaultdict(collections.Counter)
    for line in open(sys.argv[3]):
        parts = line.split()
        if parts[0] == "=":
            pairs = [(parts[1], n) for n in parts[2].split(",")[:1]]
        elif parts[0] == "~" and parts[3] == "1.00":
            pairs = [(parts[1], parts[2])]
        else:
            continue
        for fb, fa in pairs:
            if fa not in a or fb not in b or len(a[fa]) != len(b[fb]):
                continue
            for ia, ib in zip(a[fa], b[fb]):
                oa, ob = ds_operands(ia), ds_operands(ib)
                if len(oa) == len(ob):
                    for x, y in zip(oa, ob):
                        votes[y][x] += 1
    for y in sorted(votes):
        (x, n), = votes[y].most_common(1)
        others = sum(votes[y].values()) - n
        print(f"{y:04x}\t{x:04x}\t{n}" + (f"\t({others} other)" if others else ""))


if __name__ == "__main__":
    main()
