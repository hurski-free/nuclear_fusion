"""Generate lab element / isotope PNGs for mid-ladder elements."""
import math
import os
import struct
import zlib

OUT_DIR = os.path.join(
    os.path.dirname(__file__), "..", "src", "assets", "images", "elements"
)

COLORS = {
    "nickel": (200, 210, 220),
    "silver": (220, 230, 240),
    "xenon": (160, 140, 255),
}


def write_png(path, w, h, rgba):
    def chunk(tag, data):
        return (
            struct.pack(">I", len(data))
            + tag
            + data
            + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
        )

    raw = b"".join(b"\x00" + rgba[y * w * 4 : (y + 1) * w * 4] for y in range(h))
    data = b"\x89PNG\r\n\x1a\n"
    data += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    data += chunk(b"IDAT", zlib.compress(raw, 9))
    data += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(data)


def px(buf, w, x, y, r, g, b, a):
    if not (0 <= x < w and 0 <= y < w):
        return
    i = (y * w + x) * 4
    oa = buf[i + 3] / 255.0
    na = a / 255.0
    out_a = na + oa * (1 - na)
    if out_a <= 1e-6:
        return
    buf[i] = int((r * na + buf[i] * oa * (1 - na)) / out_a)
    buf[i + 1] = int((g * na + buf[i + 1] * oa * (1 - na)) / out_a)
    buf[i + 2] = int((b * na + buf[i + 2] * oa * (1 - na)) / out_a)
    buf[i + 3] = int(out_a * 255)


def disk(buf, w, cx, cy, rad, r, g, b, a, soft=2.0):
    r0 = int(rad + soft + 1)
    for y in range(cy - r0, cy + r0 + 1):
        for x in range(cx - r0, cx + r0 + 1):
            d = math.hypot(x - cx, y - cy)
            if d <= rad:
                px(buf, w, x, y, r, g, b, a)
            elif d <= rad + soft:
                t = 1.0 - (d - rad) / soft
                px(buf, w, x, y, r, g, b, int(a * t))


def ring(buf, w, cx, cy, r_in, r_out, r, g, b, a):
    r0 = int(r_out + 2)
    for y in range(cy - r0, cy + r0 + 1):
        for x in range(cx - r0, cx + r0 + 1):
            d = math.hypot(x - cx, y - cy)
            if r_in <= d <= r_out:
                edge = min(d - r_in, r_out - d)
                t = min(1.0, edge / 1.4)
                px(buf, w, x, y, r, g, b, int(a * t))


def make_element(name, accent, isotope=False, n=96):
    b = bytearray(n * n * 4)
    cx = cy = n // 2
    # Glow
    disk(b, n, cx, cy, 28, *accent, 40 if not isotope else 70, soft=8)
    # Nucleus
    disk(b, n, cx, cy, 14, *accent, 230, soft=3)
    disk(b, n, cx, cy, 7, 240, 255, 240, 255)
    # Orbits
    ring(b, n, cx, cy, 22, 24, *accent, 200)
    ring(b, n, cx, cy, 30, 32, 80, 220, 120, 160)
    for ang, sc in ((25, 1.0), (150, 0.9), (270, 0.75)):
        a = math.radians(ang)
        x = cx + math.cos(a) * 26 * sc
        y = cy + math.sin(a) * 16 * sc
        disk(b, n, int(x), int(y), 3.2, 255, 230, 120, 240, soft=1.2)
    if isotope:
        # Mutation mark
        disk(b, n, cx + 18, cy - 18, 6, 255, 120, 60, 230, soft=2)
        disk(b, n, cx + 18, cy - 18, 2.5, 255, 230, 180, 255)
    return b


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    for name, col in COLORS.items():
        path = os.path.join(OUT_DIR, f"{name}.png")
        write_png(path, 96, 96, bytes(make_element(name, col, False)))
        print(name, os.path.getsize(path))
        ipath = os.path.join(OUT_DIR, f"iso_{name}.png")
        write_png(ipath, 96, 96, bytes(make_element(name, col, True)))
        print("iso_" + name, os.path.getsize(ipath))


if __name__ == "__main__":
    main()
