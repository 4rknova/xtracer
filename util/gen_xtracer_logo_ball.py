#!/usr/bin/env python3

import math
import subprocess
import tempfile
from pathlib import Path

from PIL import Image, ImageFilter, ImageOps


ROOT = Path(__file__).resolve().parents[1]
SVG_PATH = ROOT / "src/frontend/web-client/logo.svg"
OUT_DIR = ROOT / "scene/resources/generated"
OBJ_PATH = OUT_DIR / "xtracer_logo_ball.obj"
MASK_PATH = OUT_DIR / "xtracer_logo_ball_mask.png"


def normalize(v):
    x, y, z = v
    length = math.sqrt(x * x + y * y + z * z)
    if length <= 1e-12:
        return (0.0, 0.0, 1.0)
    inv = 1.0 / length
    return (x * inv, y * inv, z * inv)


def midpoint(a, b):
    return ((a[0] + b[0]) * 0.5, (a[1] + b[1]) * 0.5, (a[2] + b[2]) * 0.5)


def smoothstep(edge0, edge1, x):
    if x <= edge0:
        return 0.0
    if x >= edge1:
        return 1.0
    t = (x - edge0) / (edge1 - edge0)
    return t * t * (3.0 - 2.0 * t)


def generate_icosphere(subdivisions, radius):
    t = (1.0 + math.sqrt(5.0)) * 0.5
    vertices = [
        normalize((-1.0,  t,  0.0)),
        normalize(( 1.0,  t,  0.0)),
        normalize((-1.0, -t,  0.0)),
        normalize(( 1.0, -t,  0.0)),
        normalize(( 0.0, -1.0,  t)),
        normalize(( 0.0,  1.0,  t)),
        normalize(( 0.0, -1.0, -t)),
        normalize(( 0.0,  1.0, -t)),
        normalize(( t,  0.0, -1.0)),
        normalize(( t,  0.0,  1.0)),
        normalize((-t,  0.0, -1.0)),
        normalize((-t,  0.0,  1.0)),
    ]

    faces = [
        (0, 11, 5), (0, 5, 1), (0, 1, 7), (0, 7, 10), (0, 10, 11),
        (1, 5, 9), (5, 11, 4), (11, 10, 2), (10, 7, 6), (7, 1, 8),
        (3, 9, 4), (3, 4, 2), (3, 2, 6), (3, 6, 8), (3, 8, 9),
        (4, 9, 5), (2, 4, 11), (6, 2, 10), (8, 6, 7), (9, 8, 1),
    ]

    for _ in range(subdivisions):
        midpoint_cache = {}
        next_faces = []

        def midpoint_index(i, j):
            key = (i, j) if i < j else (j, i)
            cached = midpoint_cache.get(key)
            if cached is not None:
                return cached
            m = normalize(midpoint(vertices[i], vertices[j]))
            vertices.append(m)
            idx = len(vertices) - 1
            midpoint_cache[key] = idx
            return idx

        for a, b, c in faces:
            ab = midpoint_index(a, b)
            bc = midpoint_index(b, c)
            ca = midpoint_index(c, a)
            next_faces.extend([
                (a, ab, ca),
                (b, bc, ab),
                (c, ca, bc),
                (ab, bc, ca),
            ])
        faces = next_faces

    vertices = [(x * radius, y * radius, z * radius) for x, y, z in vertices]
    return vertices, faces


def rasterize_logo(svg_path, out_png_path):
    with tempfile.TemporaryDirectory(prefix="xtracer-logo-ball.") as tmp_dir:
        tmp_png = Path(tmp_dir) / "logo.png"
        subprocess.run(
            [
                "inkscape",
                str(svg_path),
                "--export-type=png",
                f"--export-filename={tmp_png}",
                "--export-width=2600",
                "--export-background-opacity=0",
            ],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

        rgba = Image.open(tmp_png).convert("RGBA")
        alpha = rgba.getchannel("A")
        bbox = alpha.getbbox()
        if bbox is None:
            raise RuntimeError("logo export has no visible alpha")

        alpha = alpha.crop(bbox)
        alpha = alpha.filter(ImageFilter.MaxFilter(5))
        alpha = alpha.filter(ImageFilter.GaussianBlur(1.8))
        alpha = ImageOps.autocontrast(alpha)

        resampling = getattr(Image, "Resampling", Image)
        canvas = Image.new("L", (2200, 760), 0)
        alpha.thumbnail((canvas.width - 160, canvas.height - 120), resampling.LANCZOS)
        offset = ((canvas.width - alpha.width) // 2, (canvas.height - alpha.height) // 2)
        canvas.paste(alpha, offset)
        canvas.save(out_png_path)
        return canvas


def sample_mask(mask, u, v):
    width, height = mask.size
    x = max(0.0, min(width - 1.0, u * (width - 1.0)))
    y = max(0.0, min(height - 1.0, v * (height - 1.0)))

    x0 = int(math.floor(x))
    y0 = int(math.floor(y))
    x1 = min(width - 1, x0 + 1)
    y1 = min(height - 1, y0 + 1)
    tx = x - x0
    ty = y - y0

    p00 = mask.getpixel((x0, y0)) / 255.0
    p10 = mask.getpixel((x1, y0)) / 255.0
    p01 = mask.getpixel((x0, y1)) / 255.0
    p11 = mask.getpixel((x1, y1)) / 255.0

    a = p00 * (1.0 - tx) + p10 * tx
    b = p01 * (1.0 - tx) + p11 * tx
    return a * (1.0 - ty) + b * ty


def sculpt_logo(vertices, mask, radius):
    half_width = math.radians(55.0)
    half_height = math.radians(18.0)
    max_depth = 0.12
    edge_softness = 0.12
    sculpted = []

    for vx, vy, vz in vertices:
        nx, ny, nz = normalize((vx, vy, vz))
        front = -nz
        if front <= 0.0:
            sculpted.append((vx, vy, vz))
            continue

        x_angle = math.atan2(nx, front)
        y_angle = math.atan2(ny, front)
        if abs(x_angle) > half_width or abs(y_angle) > half_height:
            sculpted.append((vx, vy, vz))
            continue

        u = 0.5 + (x_angle / (2.0 * half_width))
        v = 0.5 - (y_angle / (2.0 * half_height))
        strength = sample_mask(mask, u, v)
        if strength <= 0.01:
            sculpted.append((vx, vy, vz))
            continue

        edge_x = 1.0 - smoothstep(1.0 - edge_softness, 1.0, abs(x_angle) / half_width)
        edge_y = 1.0 - smoothstep(1.0 - edge_softness, 1.0, abs(y_angle) / half_height)
        depth = max_depth * (strength ** 1.35) * edge_x * edge_y
        new_radius = radius - depth
        sculpted.append((nx * new_radius, ny * new_radius, nz * new_radius))

    return sculpted


def write_obj(path, vertices, faces):
    normals = [normalize(v) for v in vertices]
    with path.open("w", encoding="ascii") as f:
        f.write("# Generated by util/gen_xtracer_logo_ball.py\n")
        f.write("o xtracer_logo_ball\n")
        for x, y, z in vertices:
            f.write(f"v {x:.8f} {y:.8f} {z:.8f}\n")
        for nx, ny, nz in normals:
            f.write(f"vn {nx:.8f} {ny:.8f} {nz:.8f}\n")
        for a, b, c in faces:
            ai = a + 1
            bi = b + 1
            ci = c + 1
            f.write(f"f {ai}//{ai} {bi}//{bi} {ci}//{ci}\n")


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    mask = rasterize_logo(SVG_PATH, MASK_PATH)
    radius = 1.0
    vertices, faces = generate_icosphere(subdivisions=6, radius=radius)
    vertices = sculpt_logo(vertices, mask, radius)
    write_obj(OBJ_PATH, vertices, faces)
    print(f"wrote {OBJ_PATH}")
    print(f"wrote {MASK_PATH}")


if __name__ == "__main__":
    main()
