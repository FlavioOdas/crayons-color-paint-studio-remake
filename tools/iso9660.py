"""Extract an ISO 9660 image (plain ISO names, ;1 stripped). Usage: iso9660.py image.iso outdir"""
import os
import struct
import sys

SECTOR = 2048


def walk(f, extent, size, path, out):
    f.seek(extent * SECTOR)
    data = f.read(size)
    pos = 0
    while pos < len(data):
        length = data[pos]
        if length == 0:  # records never cross sectors: skip to the next one
            pos = (pos // SECTOR + 1) * SECTOR
            continue
        rec = data[pos:pos + length]
        ext, = struct.unpack_from("<I", rec, 2)
        sz, = struct.unpack_from("<I", rec, 10)
        y, mo, d, h, mi, s = rec[18:24]
        flags = rec[25]
        name = rec[33:33 + rec[32]]
        pos += length
        if name in (b"\0", b"\1"):
            continue
        name = name.decode("latin-1").split(";")[0].rstrip(".")
        full = os.path.join(path, name)
        if flags & 2:
            os.makedirs(os.path.join(out, full), exist_ok=True)
            walk(f, ext, sz, full, out)
        else:
            f.seek(ext * SECTOR)
            with open(os.path.join(out, full), "wb") as o:
                o.write(f.read(sz))
            print(f"{1900 + y:04d}-{mo:02d}-{d:02d} {h:02d}:{mi:02d} {sz:10d} {full}")
            f.seek(0)


def main():
    image, out = sys.argv[1], sys.argv[2]
    with open(image, "rb") as f:
        f.seek(16 * SECTOR)
        pvd = f.read(SECTOR)
        assert pvd[1:6] == b"CD001", "not ISO 9660"
        print("volume:", pvd[40:72].decode().strip(), "created", pvd[813:829].decode())
        root = pvd[156:190]
        ext, = struct.unpack_from("<I", root, 2)
        sz, = struct.unpack_from("<I", root, 10)
        os.makedirs(out, exist_ok=True)
        walk(f, ext, sz, "", out)


if __name__ == "__main__":
    main()
