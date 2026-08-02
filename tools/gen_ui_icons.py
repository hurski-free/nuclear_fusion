import math
import os
import struct
import zlib

OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "src", "assets", "icons")


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


def disk(buf, w, cx, cy, rad, r, g, b, a, soft=1.5):
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
                t = min(1.0, edge / 1.2)
                px(buf, w, x, y, r, g, b, int(a * t))


def line(buf, w, x0, y0, x1, y1, r, g, b, a, thick=1.5):
    steps = int(max(abs(x1 - x0), abs(y1 - y0)) * 2) + 1
    for i in range(steps + 1):
        t = i / steps
        x = x0 + (x1 - x0) * t
        y = y0 + (y1 - y0) * t
        disk(buf, w, int(round(x)), int(round(y)), thick, r, g, b, a, soft=1.0)


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    n = 64

    lab = bytearray(n * n * 4)
    for y in range(n):
        for x in range(n):
            if 18 <= y <= 28 and 28 <= x <= 35:
                px(lab, n, x, y, 70, 220, 110, 230)
            if 28 <= y <= 52:
                half = 6 + (y - 28) * 0.55
                if abs(x - 32) <= half:
                    if y >= 40:
                        px(lab, n, x, y, 40, 180, 90, 210)
                    else:
                        px(lab, n, x, y, 90, 240, 140, 180)
    ring(lab, n, 32, 44, 7, 9, 160, 255, 190, 220)
    disk(lab, n, 32, 44, 2.5, 220, 255, 230, 255)
    disk(lab, n, 27, 36, 1.2, 180, 255, 200, 200)
    disk(lab, n, 36, 38, 1.0, 180, 255, 200, 180)
    write_png(os.path.join(OUT_DIR, "lab.png"), n, n, bytes(lab))

    tech = bytearray(n * n * 4)
    for y in range(18, 46):
        for x in range(18, 46):
            px(tech, n, x, y, 20, 90, 40, 230)
    for y in range(24, 40):
        for x in range(24, 40):
            px(tech, n, x, y, 40, 160, 80, 240)
    disk(tech, n, 32, 32, 5, 120, 255, 160, 255)
    disk(tech, n, 32, 32, 2, 220, 255, 220, 255)
    for i in range(4):
        yy = 22 + i * 6
        line(tech, n, 10, yy, 18, yy, 80, 230, 120, 230, 1.2)
        line(tech, n, 46, yy, 54, yy, 80, 230, 120, 230, 1.2)
        xx = 22 + i * 6
        line(tech, n, xx, 10, xx, 18, 80, 230, 120, 230, 1.2)
        line(tech, n, xx, 46, xx, 54, 80, 230, 120, 230, 1.2)
    for ang in range(0, 360, 45):
        rad = math.radians(ang)
        x = int(32 + math.cos(rad) * 9)
        y = int(32 + math.sin(rad) * 9)
        disk(tech, n, x, y, 1.4, 160, 255, 180, 220)
    write_png(os.path.join(OUT_DIR, "tech.png"), n, n, bytes(tech))

    sn = bytearray(n * n * 4)
    cx = cy = 32
    disk(sn, n, cx, cy, 26, 120, 20, 10, 60, soft=6)
    for ang in range(0, 360, 15):
        rad = math.radians(ang)
        x1 = cx + math.cos(rad) * 8
        y1 = cy + math.sin(rad) * 8
        long_ray = ang % 30 == 0
        x2 = cx + math.cos(rad) * (22 if long_ray else 17)
        y2 = cy + math.sin(rad) * (22 if long_ray else 17)
        col = (255, 180, 60) if long_ray else (255, 90, 40)
        line(sn, n, x1, y1, x2, y2, *col, 210, 1.3 if long_ray else 1.0)
    ring(sn, n, cx, cy, 12, 14, 255, 120, 50, 200)
    ring(sn, n, cx, cy, 16, 18, 255, 70, 40, 140)
    disk(sn, n, cx, cy, 8, 255, 220, 120, 240, soft=2)
    disk(sn, n, cx, cy, 4, 255, 255, 230, 255, soft=1.5)
    write_png(os.path.join(OUT_DIR, "supernova.png"), n, n, bytes(sn))

    for name in ("lab.png", "tech.png", "supernova.png"):
        path = os.path.join(OUT_DIR, name)
        print(name, os.path.getsize(path))


if __name__ == "__main__":
    main()
