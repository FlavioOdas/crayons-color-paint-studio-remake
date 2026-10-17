"""List Delphi 1 published methods (event handlers) with their code addresses.

A published method table entry is a far pointer (an NE fixup to an internal segment)
followed by the method's name as a Pascal string. The table is preceded by the class
name and an entry count. Usage: python3 tools/delphi_methods.py EXE  -> class<TAB>method<TAB>seg:off
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from nelib import Ne, SRC_FARADDR, SRC_SEGMENT  # noqa: E402

IDENT = re.compile(rb"[A-Za-z_][A-Za-z0-9_]*\Z")


def main():
    ne = Ne(sys.argv[1])
    for seg in ne.segments:
        d = seg.data
        for loc in sorted(seg.fixups):
            src, target = seg.fixups[loc]
            if target[0] != "internal":
                continue
            if src == SRC_SEGMENT:  # some builds store <u16 offset><u16 segment fixup>
                offset = d[loc - 2] | d[loc - 1] << 8
                target = ("internal", target[1], offset)
                loc -= 2
            elif src != SRC_FARADDR:
                continue
            n = d[loc + 4] if loc + 4 < len(d) else 0
            name = bytes(d[loc + 5:loc + 5 + n])
            if not n or not IDENT.match(name):
                continue
            # walk back to the table head: <pstr class><u16 count>
            cls = "?"
            p = loc
            while p >= 0:
                if p - 2 >= 0 and (p - 2, ) and d[p - 2] < 200 and d[p - 1] == 0:
                    for k in range(2, 40):
                        q = p - 2 - k - 1
                        if q >= 0 and d[q] == k and IDENT.match(bytes(d[q + 1:q + 1 + k])) and d[q + 1] in b"T":
                            cls = bytes(d[q + 1:q + 1 + k]).decode()
                            break
                    if cls != "?":
                        break
                # previous entry: we cannot walk back reliably, so stop at a gap
                prev = [l for l in seg.fixups if l < p and p - l <= 4 + 64]
                if not prev:
                    break
                p = max(prev)
            print(f"{cls}\t{name.decode()}\t{target[1]}:{target[2]:04x}\t(table in seg {seg.number} @ {loc:04x})")


if __name__ == "__main__":
    main()
