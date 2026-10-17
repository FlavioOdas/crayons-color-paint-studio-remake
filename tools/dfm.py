"""Turn the binary Delphi form resources (RCDATA "TPF0") of an NE executable into text.

Usage: python3 tools/dfm.py original/extracted/app/CRAYON1.EXE work/forms/crayon1 [--save-binary]
Writes one <FORM>.dfm.txt per form; with --save-binary, also each binary property (pictures,
glyphs) as <FORM>.<Component>.<Property>.bin so pictures can be looked at.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ne_resources import resources  # noqa: E402


class Reader:
    def __init__(self, data):
        self.d, self.p = data, 0

    def u8(self):
        self.p += 1
        return self.d[self.p - 1]

    def take(self, n):
        self.p += n
        return self.d[self.p - n:self.p]

    def sstr(self):
        return self.take(self.u8()).decode("latin-1")

    def value(self, binaries, key):
        t = self.u8()
        if t == 0:
            return "<null>"
        if t == 1:  # list
            items = []
            while self.d[self.p] != 0:
                items.append(self.value(binaries, key))
            self.p += 1
            return "(" + " ".join(items) + ")"
        if t == 2:
            return str(struct.unpack("<b", self.take(1))[0])
        if t == 3:
            return str(struct.unpack("<h", self.take(2))[0])
        if t == 4:
            return str(struct.unpack("<i", self.take(4))[0])
        if t == 5:  # 80-bit extended
            raw = self.take(10)
            mant, exp = struct.unpack("<QH", raw)
            sign = -1 if exp & 0x8000 else 1
            exp &= 0x7FFF
            v = 0.0 if exp == 0 and mant == 0 else sign * mant / (1 << 63) * 2.0 ** (exp - 16383)
            return repr(v)
        if t == 6:
            return repr(self.sstr())
        if t == 7:
            return self.sstr()
        if t == 8:
            return "False"
        if t == 9:
            return "True"
        if t == 10:
            n = struct.unpack("<I", self.take(4))[0]
            binaries.append((key, self.take(n)))
            return f"{{binary {n} bytes}}"
        if t == 11:
            items = []
            while True:
                s = self.sstr()
                if not s:
                    break
                items.append(s)
            return "[" + ", ".join(items) + "]"
        if t == 12:
            n = struct.unpack("<I", self.take(4))[0]
            return repr(self.take(n).decode("latin-1"))
        if t == 13:
            return "nil"
        if t == 14:  # collection
            items = []
            while self.d[self.p] != 0:
                if self.d[self.p] in (2, 3, 4):
                    self.value(binaries, key)
                self.p += 1  # vaList
                props = self.props(binaries, key)
                items.append("item " + "; ".join(f"{k} = {v}" for k, v in props) + " end")
            self.p += 1
            return "<" + " ".join(items) + ">"
        raise ValueError(f"unknown value type {t} at {self.p - 1:#x}")

    def props(self, binaries, owner):
        out = []
        while True:
            name = self.sstr()
            if not name:
                return out
            out.append((name, self.value(binaries, f"{owner}.{name}")))

    def component(self, depth, binaries, lines, events):
        if self.d[self.p] & 0xF0 == 0xF0:  # Delphi 2+ filer flags (not expected in Delphi 1)
            flags = self.u8()
            if flags & 2:
                self.value([], "")
        cls, name = self.sstr(), self.sstr()
        pad = "  " * depth
        lines.append(f"{pad}object {name}: {cls}")
        for k, v in self.props(binaries, name):
            lines.append(f"{pad}  {k} = {v}")
            if k.startswith("On"):
                events.append((name, cls, k, v))
        while self.d[self.p] != 0:
            self.component(depth + 1, binaries, lines, events)
        self.p += 1
        lines.append(f"{pad}end")


def main():
    exe, out = sys.argv[1], sys.argv[2]
    save = "--save-binary" in sys.argv
    os.makedirs(out, exist_ok=True)
    data = open(exe, "rb").read()
    for tid, rid, blob, name in named_rcdata(data):
        if blob[:4] != b"TPF0":
            continue
        r = Reader(blob)
        r.p = 4
        lines, binaries, events = [], [], []
        r.component(0, binaries, lines, events)
        with open(os.path.join(out, f"{name}.dfm.txt"), "w") as f:
            f.write("\n".join(lines) + "\n")
        if save:
            for key, b in binaries:
                with open(os.path.join(out, f"{name}.{key}.bin"), "wb") as f:
                    f.write(b)
        print(f"{name}: {sum(1 for l in lines if l.strip().startswith('object '))} components, "
              f"{len(events)} event bindings, {len(binaries)} binary blobs")


def named_rcdata(b):
    """Like ne_resources.resources but keeps string resource names (Delphi names forms by class)."""
    ne = struct.unpack_from("<I", b, 0x3C)[0]
    res_t = ne + struct.unpack_from("<H", b, ne + 0x24)[0]
    shift = struct.unpack_from("<H", b, res_t)[0]
    o = res_t + 2
    while True:
        tid, count = struct.unpack_from("<HH", b, o)
        if tid == 0:
            return
        for k in range(count):
            off, length, _flags, rid = struct.unpack_from("<HHHH", b, o + 8 + 12 * k)
            if rid & 0x8000:
                name = str(rid & 0x7FFF)
            else:
                n = b[res_t + rid]
                name = b[res_t + rid + 1:res_t + rid + 1 + n].decode("latin-1")
            if tid & 0x7FFF == 10:
                yield tid, rid, b[off << shift:(off << shift) + (length << shift)], name
        o += 8 + 12 * count


if __name__ == "__main__":
    main()
