"""WinHelp 3.1 (.HLP) reader: lists internal files and dumps topics as text.
Usage: python3 -I tools/hlp.py FILE.HLP [--list | --topics | --extract DIR]
Format notes: docs/formats/hlp.md (header, B+ tree directory, |SYSTEM, |TOPIC LZ77 blocks)."""
import os
import struct
import sys


class Hlp:
    def __init__(self, path):
        self.b = open(path, "rb").read()
        magic, self.dir_start, self.free, self.size = struct.unpack_from("<IIiI", self.b, 0)
        assert magic == 0x00035F3F, hex(magic)
        self.files = self._btree(self.dir_start)

    def _file(self, offset):
        reserved, used, flags = struct.unpack_from("<IIB", self.b, offset)
        return self.b[offset + 9:offset + 9 + used]

    def _btree(self, offset):
        data = self._file(offset)
        (magic, flags, page_size, structure, must_be_zero, page_splits, root, must_be_neg1,
         total_pages, levels, entries) = struct.unpack_from("<HHH16sHHHhHHI", data, 0)
        assert magic == 0x293B
        pages = data[38:]
        out = {}

        def leaf_chain(page):
            while page != -1:
                p = pages[page * page_size:(page + 1) * page_size]
                unused, n, prev, nxt = struct.unpack_from("<HHhh", p, 0)
                pos = 8
                for _ in range(n):
                    end = p.index(b"\0", pos)
                    name = p[pos:end].decode("latin-1")
                    (off,) = struct.unpack_from("<I", p, end + 1)
                    out[name] = off
                    pos = end + 5
                page = nxt

        # descend to the first leaf
        page = root
        for _ in range(levels - 1):
            p = pages[page * page_size:(page + 1) * page_size]
            unused, n, prev = struct.unpack_from("<HHh", p, 0)
            page = prev
        leaf_chain(page)
        return out

    def file(self, name):
        return self._file(self.files[name])


def main():
    h = Hlp(sys.argv[1])
    mode = sys.argv[2] if len(sys.argv) > 2 else "--list"
    if mode == "--list":
        for name, off in sorted(h.files.items(), key=lambda kv: kv[1]):
            print(f"{off:8x} {len(h.file(name)):8d} {name}")
    elif mode == "--extract":
        out = sys.argv[3]
        os.makedirs(out, exist_ok=True)
        for name in h.files:
            open(os.path.join(out, name.replace("|", "_")), "wb").write(h.file(name))


if __name__ == "__main__":
    main()


# ---- topics -------------------------------------------------------------------------------

class Reader:
    def __init__(self, b, pos=0):
        self.b, self.p = b, pos

    def byte(self):
        v = self.b[self.p]
        self.p += 1
        return v

    def word(self):
        v = struct.unpack_from("<H", self.b, self.p)[0]
        self.p += 2
        return v

    def short(self):
        v = struct.unpack_from("<h", self.b, self.p)[0]
        self.p += 2
        return v

    def long(self):
        v = struct.unpack_from("<i", self.b, self.p)[0]
        self.p += 4
        return v

    def cushort(self):  # compressed unsigned short
        if self.b[self.p] & 1:
            return self.word() >> 1
        return self.byte() >> 1

    def csshort(self):  # compressed signed short
        if self.b[self.p] & 1:
            return (self.word() >> 1) - 0x4000
        return (self.byte() >> 1) - 0x40

    def culong(self):
        if self.b[self.p] & 1:
            return struct.unpack_from("<I", self.b, self.p - 0)[0] >> 1 if not self._adv(4) else 0
        return self.word() >> 1

    def _adv(self, n):
        self.p += n
        return False

    def cslong(self):
        if self.b[self.p] & 1:
            v = struct.unpack_from("<I", self.b, self.p)[0]
            self.p += 4
            return (v >> 1) - 0x40000000
        return (self.word() >> 1) - 0x4000

    def cstr(self):
        e = self.b.index(b"\0", self.p)
        s = self.b[self.p:e].decode("cp1252")
        self.p = e + 1
        return s


def ulong_c(r):
    if r.b[r.p] & 1:
        v = struct.unpack_from("<I", r.b, r.p)[0]
        r.p += 4
        return v >> 1
    return r.word() >> 1


def topic_stream(h):
    """The |TOPIC data without block headers, and a TOPICPOS -> stream offset mapping."""
    t = h.file("|TOPIC")
    data = b"".join(t[i:i + 4096][12:] for i in range(0, len(t), 4096))
    return data, lambda pos: (pos >> 14) * 4084 + (pos & 0x3FFF) - 12


def links(h):
    data, at = topic_stream(h)
    pos = 12
    while pos != -1 and pos != 0xFFFFFFFF:
        o = at(pos)
        if o < 0 or o + 21 > len(data):
            break
        bs, dl2, prev, nxt, dl1, rt = struct.unpack_from("<IIiiIB", data, o)
        d1 = data[o + 21:o + dl1]
        d2 = data[o + dl1:o + bs]
        yield pos, rt, d1, d2
        pos = nxt
