"""Generate unique phosphor-style icons for Tech upgrades."""

import math
import os
import struct
import zlib

OUT_DIR = os.path.join(
    os.path.dirname(__file__), "..", "src", "assets", "icons", "upgrades"
)

# Console palette
G = (90, 240, 140)
G_DIM = (40, 160, 80)
G_HOT = (180, 255, 200)
G_CORE = (220, 255, 230)
ORANGE = (255, 170, 70)
BLUE = (120, 200, 255)
PURPLE = (180, 140, 255)
YELLOW = (255, 230, 120)


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
    cx = float(cx)
    cy = float(cy)
    rad = float(rad)
    r0 = int(rad + soft + 1)
    ix = int(round(cx))
    iy = int(round(cy))
    for y in range(iy - r0, iy + r0 + 1):
        for x in range(ix - r0, ix + r0 + 1):
            d = math.hypot(x - cx, y - cy)
            if d <= rad:
                px(buf, w, x, y, r, g, b, a)
            elif d <= rad + soft:
                t = 1.0 - (d - rad) / soft
                px(buf, w, x, y, r, g, b, int(a * t))


def ring(buf, w, cx, cy, r_in, r_out, r, g, b, a):
    cx = float(cx)
    cy = float(cy)
    r0 = int(float(r_out) + 2)
    ix = int(round(cx))
    iy = int(round(cy))
    for y in range(iy - r0, iy + r0 + 1):
        for x in range(ix - r0, ix + r0 + 1):
            d = math.hypot(x - cx, y - cy)
            if r_in <= d <= r_out:
                edge = min(d - r_in, r_out - d)
                t = min(1.0, edge / 1.2)
                px(buf, w, x, y, r, g, b, int(a * t))


def line(buf, w, x0, y0, x1, y1, r, g, b, a, thick=1.5):
    steps = int(max(abs(x1 - x0), abs(y1 - y0)) * 2) + 1
    for i in range(steps + 1):
        t = i / max(1, steps)
        x = x0 + (x1 - x0) * t
        y = y0 + (y1 - y0) * t
        disk(buf, w, int(round(x)), int(round(y)), thick, r, g, b, a, soft=1.0)


def rect(buf, w, x0, y0, x1, y1, r, g, b, a):
    for y in range(int(y0), int(y1) + 1):
        for x in range(int(x0), int(x1) + 1):
            px(buf, w, x, y, r, g, b, a)


def canvas(n=64):
    return bytearray(n * n * 4)


def save(buf, name, n=64):
    path = os.path.join(OUT_DIR, name)
    write_png(path, n, n, bytes(buf))
    print(name, os.path.getsize(path))


def icon_click_p(n=64):
    """Proton Injector — nozzle firing a proton."""
    b = canvas(n)
    # body
    rect(b, n, 14, 26, 40, 38, *G_DIM, 230)
    rect(b, n, 18, 22, 36, 26, *G, 220)
    # tip
    for i in range(8):
        line(b, n, 40, 32, 40 + i * 1.2, 32 - 3 + i * 0.75, *G_HOT, 200, 1.1)
    # proton
    disk(b, n, 50, 32, 5, 80, 255, 140, 255, soft=2)
    disk(b, n, 50, 32, 2.2, *G_CORE, 255)
    # glow trail
    for i in range(5):
        disk(b, n, 42 + i * 1.5, 32, 1.5 - i * 0.15, *G, 140)
    return b


def icon_click_n(n=64):
    """Neutron Channel — tunnel with neutron."""
    b = canvas(n)
    # channel walls
    for y in (20, 44):
        rect(b, n, 10, y, 54, y + 4, *G_DIM, 230)
    # side rails
    line(b, n, 12, 24, 12, 44, *G, 200, 1.4)
    line(b, n, 52, 24, 52, 44, *G, 200, 1.4)
    # flowing dashed path
    for x in range(16, 50, 6):
        disk(b, n, x, 32, 1.6, *G_HOT, 180)
    # neutron
    disk(b, n, 32, 32, 7, 100, 220, 255, 240, soft=2)
    disk(b, n, 32, 32, 3, 200, 240, 255, 255)
    return b


def icon_click_e(n=64):
    """Electron Lens — convex lens focusing electrons."""
    b = canvas(n)
    # lens body (ellipse-ish)
    for y in range(n):
        for x in range(n):
            dx = (x - 32) / 10.0
            dy = (y - 32) / 18.0
            if dx * dx + dy * dy <= 1.0:
                t = 1.0 - (dx * dx + dy * dy)
                px(b, n, x, y, 60, 200, 255, int(80 + 140 * t))
    ring(b, n, 32, 32, 9.5, 11.5, *BLUE, 220)
    # focus rays
    line(b, n, 8, 20, 24, 30, *G_HOT, 200, 1.2)
    line(b, n, 8, 44, 24, 34, *G_HOT, 200, 1.2)
    line(b, n, 40, 30, 56, 24, *YELLOW, 210, 1.2)
    line(b, n, 40, 34, 56, 40, *YELLOW, 210, 1.2)
    disk(b, n, 32, 32, 2.5, *G_CORE, 255)
    return b


def icon_crit_he(n=64):
    """Helium Core Focus — crosshair over helium core."""
    b = canvas(n)
    disk(b, n, 32, 32, 14, 255, 200, 80, 90, soft=4)
    disk(b, n, 32, 32, 8, *ORANGE, 230, soft=2)
    disk(b, n, 32, 32, 3.5, *YELLOW, 255)
    # crosshair
    ring(b, n, 32, 32, 16, 18, *G, 230)
    line(b, n, 32, 8, 32, 18, *G_HOT, 240, 1.3)
    line(b, n, 32, 46, 32, 56, *G_HOT, 240, 1.3)
    line(b, n, 8, 32, 18, 32, *G_HOT, 240, 1.3)
    line(b, n, 46, 32, 56, 32, *G_HOT, 240, 1.3)
    return b


def icon_crit_c(n=64):
    """Carbon Lattice — hexagonal lattice."""
    b = canvas(n)
    pts = []
    for i in range(6):
        a = math.radians(30 + i * 60)
        pts.append((32 + math.cos(a) * 16, 32 + math.sin(a) * 16))
    for i in range(6):
        x0, y0 = pts[i]
        x1, y1 = pts[(i + 1) % 6]
        line(b, n, x0, y0, x1, y1, *G, 230, 1.5)
        line(b, n, 32, 32, x0, y0, *G_DIM, 180, 1.2)
        disk(b, n, int(x0), int(y0), 2.4, *G_HOT, 255)
    disk(b, n, 32, 32, 3.2, *G_CORE, 255)
    return b


def icon_auto_h(n=64):
    """Hydrogen Farm — sprouting nodes."""
    b = canvas(n)
    # ground
    rect(b, n, 10, 46, 54, 52, *G_DIM, 220)
    # stems
    for x, h in ((20, 18), (32, 26), (44, 16)):
        line(b, n, x, 48, x, 48 - h, *G, 220, 1.4)
        disk(b, n, x, 48 - h, 4.5, 120, 255, 180, 240, soft=1.5)
        disk(b, n, x, 48 - h, 1.8, *G_CORE, 255)
    # small H dots
    disk(b, n, 26, 28, 1.5, *YELLOW, 200)
    disk(b, n, 38, 22, 1.5, *YELLOW, 200)
    return b


def icon_auto_he(n=64):
    """Helium Turbine — spinning blades."""
    b = canvas(n)
    disk(b, n, 32, 32, 20, *G_DIM, 80, soft=3)
    for i in range(6):
        a0 = math.radians(i * 60 + 10)
        a1 = math.radians(i * 60 + 40)
        for t in range(0, 12):
            u = t / 11
            a = a0 + (a1 - a0) * u
            r = 6 + u * 12
            x = 32 + math.cos(a) * r
            y = 32 + math.sin(a) * r
            disk(b, n, int(x), int(y), 2.0 - u * 0.5, *G if i % 2 == 0 else G_HOT, 220)
    disk(b, n, 32, 32, 5, *ORANGE, 240)
    disk(b, n, 32, 32, 2.2, *YELLOW, 255)
    return b


def icon_auto_o(n=64):
    """Oxygen Reactor — containment ring + core."""
    b = canvas(n)
    ring(b, n, 32, 32, 18, 21, *G, 230)
    ring(b, n, 32, 32, 13, 15, *G_DIM, 200)
    for ang in range(0, 360, 60):
        rad = math.radians(ang)
        x = int(32 + math.cos(rad) * 19.5)
        y = int(32 + math.sin(rad) * 19.5)
        disk(b, n, x, y, 2.2, *G_HOT, 240)
    disk(b, n, 32, 32, 8, 80, 180, 255, 220, soft=2)
    disk(b, n, 32, 32, 4, *G_CORE, 255)
    # O mark as twin lobes
    disk(b, n, 28, 32, 2.0, *BLUE, 230)
    disk(b, n, 36, 32, 2.0, *BLUE, 230)
    return b


def icon_crit_amplifier(n=64):
    """Critical Cascade — impact burst with multiplier mark."""
    b = canvas(n)
    # outer shock rings
    ring(b, n, 32, 32, 22, 24, *ORANGE, 180)
    ring(b, n, 32, 32, 16, 18, *G_HOT, 220)
    # radial spikes
    for ang in range(0, 360, 30):
        rad = math.radians(ang)
        col = YELLOW if ang % 60 == 0 else G
        alpha = 210 if ang % 60 == 0 else 160
        width = 1.4 if ang % 60 == 0 else 1.1
        line(
            b,
            n,
            32 + math.cos(rad) * 6,
            32 + math.sin(rad) * 6,
            32 + math.cos(rad) * 26,
            32 + math.sin(rad) * 26,
            *col,
            alpha,
            width,
        )
    # core
    disk(b, n, 32, 32, 8, *ORANGE, 240, soft=2)
    disk(b, n, 32, 32, 4, *YELLOW, 255)
    disk(b, n, 32, 32, 1.8, 255, 255, 240, 255)
    # small "x" hint for multiplier
    line(b, n, 44, 12, 52, 20, *G_CORE, 230, 1.6)
    line(b, n, 52, 12, 44, 20, *G_CORE, 230, 1.6)
    return b


def icon_quantum_cpu(n=64):
    """Quantum Processor — chip with qubits."""
    b = canvas(n)
    rect(b, n, 16, 16, 48, 48, 20, 70, 40, 240)
    rect(b, n, 20, 20, 44, 44, 35, 120, 70, 240)
    # pins
    for i in range(4):
        y = 20 + i * 8
        line(b, n, 8, y, 16, y, *G, 220, 1.3)
        line(b, n, 48, y, 56, y, *G, 220, 1.3)
        x = 20 + i * 8
        line(b, n, x, 8, x, 16, *G, 220, 1.3)
        line(b, n, x, 48, x, 56, *G, 220, 1.3)
    # qubit dots
    for dx, dy in ((-6, -6), (6, -6), (-6, 6), (6, 6), (0, 0)):
        disk(b, n, 32 + dx, 32 + dy, 2.4, *PURPLE, 240)
    disk(b, n, 32, 32, 1.4, *G_CORE, 255)
    return b


def icon_chaotic_accelerator(n=64):
    """Chaotic Accelerator — unstable swirling core with lightning."""
    b = canvas(n)
    magenta = (255, 100, 200)
    for i, (col, a0, r_base, twist) in enumerate(
        (
            (PURPLE, 200, 22, 1.2),
            (magenta, 180, 18, -1.7),
            (ORANGE, 190, 14, 2.3),
            (G_HOT, 170, 10, -2.8),
        )
    ):
        for t in range(0, 270, 2):
            ang = math.radians(t + i * 40) + twist * (t / 270.0)
            rad = r_base + 3.0 * math.sin(t * 0.08 + i)
            x = 32 + math.cos(ang) * rad
            y = 32 + math.sin(ang) * rad
            disk(b, n, x, y, 1.5, *col, a0, soft=1.2)
    for x0, y0, x1, y1 in ((12, 18, 50, 40), (48, 14, 18, 46), (20, 50, 46, 20)):
        line(b, n, x0, y0, x1, y1, *YELLOW, 210, 1.3)
        mx = (x0 + x1) * 0.5 + 4
        my = (y0 + y1) * 0.5 - 3
        line(b, n, x0, y0, mx, my, *G_CORE, 180, 1.0)
        line(b, n, mx, my, x1, y1, *G_CORE, 180, 1.0)
    disk(b, n, 32, 32, 9, *PURPLE, 220, soft=2.5)
    disk(b, n, 32, 32, 5.5, *magenta, 240, soft=1.5)
    disk(b, n, 32, 32, 3.0, *YELLOW, 255)
    disk(b, n, 32, 32, 1.4, 255, 255, 245, 255)
    for x, y, col in (
        (14, 28, G),
        (50, 30, ORANGE),
        (30, 12, G_HOT),
        (36, 52, PURPLE),
        (22, 44, YELLOW),
        (46, 48, magenta),
    ):
        disk(b, n, x, y, 1.8, *col, 230, soft=1.0)
    return b


def icon_annihilation(n=64):
    """Annihilation Coil — spiral coil with flash."""
    b = canvas(n)
    # spiral
    for i in range(90):
        t = i / 90
        a = t * math.pi * 4.2
        r = 4 + t * 18
        x = 32 + math.cos(a) * r
        y = 32 + math.sin(a) * r
        col = G_HOT if i % 2 == 0 else G
        disk(b, n, int(x), int(y), 1.6, *col, 210)
    # flash
    for ang in range(0, 360, 45):
        rad = math.radians(ang)
        line(
            b,
            n,
            32 + math.cos(rad) * 4,
            32 + math.sin(rad) * 4,
            32 + math.cos(rad) * 14,
            32 + math.sin(rad) * 14,
            *ORANGE,
            200,
            1.2,
        )
    disk(b, n, 32, 32, 5, *YELLOW, 240, soft=2)
    disk(b, n, 32, 32, 2.2, 255, 255, 240, 255)
    return b


# Accent colors per element (phosphor-friendly).
ELEMENT_COLORS = {
    "Hydrogen": (140, 255, 220),
    "Helium": (255, 210, 90),
    "Carbon": (90, 240, 140),
    "Oxygen": (100, 210, 255),
    "Silicon": (180, 210, 230),
    "Iron": (255, 150, 90),
    "Nickel": (200, 210, 220),
    "Silver": (220, 230, 240),
    "Xenon": (160, 140, 255),
    "Gold": (255, 215, 90),
}

# Approximate nucleon cluster size (visual only).
ELEMENT_NUCLEONS = {
    "Hydrogen": 1,
    "Helium": 4,
    "Carbon": 8,
    "Oxygen": 10,
    "Silicon": 12,
    "Iron": 14,
    "Nickel": 14,
    "Silver": 15,
    "Xenon": 15,
    "Gold": 16,
}


def icon_nucleus(element, n=64):
    """Dense nucleon cluster for nucleus click upgrades."""
    b = canvas(n)
    accent = ELEMENT_COLORS[element]
    count = ELEMENT_NUCLEONS[element]
    # Soft glow halo
    disk(b, n, 32, 32, 18, *accent, 50, soft=5)
    disk(b, n, 32, 32, 12, *G_DIM, 90, soft=3)
    # Nucleon positions on a small spiral / ring
    for i in range(count):
        if count == 1:
            x, y = 32.0, 32.0
        else:
            a = (i / count) * math.pi * 2 + 0.3
            r = 3.5 + (i % 3) * 2.2
            x = 32 + math.cos(a) * r
            y = 32 + math.sin(a) * r
        # Alternate proton (hot green) / neutron (cool blue)
        if i % 2 == 0:
            disk(b, n, x, y, 3.2, 255, 90, 90, 240, soft=1.2)
            disk(b, n, x, y, 1.3, 255, 200, 180, 255)
        else:
            disk(b, n, x, y, 3.2, 100, 180, 255, 240, soft=1.2)
            disk(b, n, x, y, 1.3, 200, 230, 255, 255)
    # Accent rim
    ring(b, n, 32, 32, 17, 19, *accent, 200)
    return b


def icon_atom(element, n=64):
    """Bohr-style atom for atom EPS upgrades."""
    b = canvas(n)
    accent = ELEMENT_COLORS[element]
    # Orbit ellipses
    for scale, rot in ((1.0, 0.0), (0.85, 50.0), (0.7, -40.0)):
        for t in range(0, 360, 3):
            a = math.radians(t + rot)
            x = 32 + math.cos(a) * 20 * scale
            y = 32 + math.sin(a) * 12 * scale
            # rotate second axes slightly
            if rot != 0:
                ca, sa = math.cos(math.radians(rot * 0.4)), math.sin(
                    math.radians(rot * 0.4)
                )
                dx, dy = x - 32, y - 32
                x = 32 + dx * ca - dy * sa
                y = 32 + dx * sa + dy * ca
            px(b, n, int(round(x)), int(round(y)), *accent, 160)
    # Nucleus
    disk(b, n, 32, 32, 6, *accent, 200, soft=2)
    disk(b, n, 32, 32, 3.2, *G_CORE, 255)
    # Electrons on orbits
    for i, (ang, sc) in enumerate(((20, 1.0), (140, 0.85), (260, 0.7))):
        a = math.radians(ang)
        x = 32 + math.cos(a) * 20 * sc
        y = 32 + math.sin(a) * 12 * sc
        disk(b, n, x, y, 2.6, *YELLOW, 240, soft=1.2)
        disk(b, n, x, y, 1.1, 255, 255, 220, 255)
    return b


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    icons = {
        "click_p.png": icon_click_p,
        "click_n.png": icon_click_n,
        "click_e.png": icon_click_e,
        "crit_he.png": icon_crit_he,
        "crit_c.png": icon_crit_c,
        "crit_amplifier.png": icon_crit_amplifier,
        "auto_h.png": icon_auto_h,
        "auto_he.png": icon_auto_he,
        "auto_o.png": icon_auto_o,
        "quantum_cpu.png": icon_quantum_cpu,
        "chaotic_accelerator.png": icon_chaotic_accelerator,
        "annihilation.png": icon_annihilation,
    }
    for name, fn in icons.items():
        save(fn(), name)

    for el in ELEMENT_COLORS:
        save(icon_nucleus(el), f"nuc_{el}.png")
        save(icon_atom(el), f"atom_{el}.png")


if __name__ == "__main__":
    main()
