# Geometry Reference

All geometry types supported by xtracer's scene format. Source: `src/xtcore/parseutil.cc`, `src/xtcore/proto.h`, `src/xtcore/math/`, `lib/nmesh/`.

---

## Analytic Primitives

Declared directly via `type = <name>`. Intersection is computed mathematically — no mesh, no BVH. All support `u_scale` / `v_scale` for UV scaling.

| Type | Fields | Notes |
|------|--------|-------|
| `sphere` | `position` vec3, `radius` float | Solves ray-sphere quadratic |
| `plane` | `normal` vec3, `distance` float | Infinite; ray-plane dot product |
| `triangle` | `vecdata = { v0, v1, v2 }` vec3 | Möller–Trumbore; UV per vertex |
| `point` | `position` vec3 | Near-zero sphere; use emissive spheres instead |

---

## Analytic Fractals

All use sphere-marching (SDF) internally. Defined via `type = <name>`. Ray cost is high — proportional to iteration depth.

### `menger_sponge`
Recursive AABB descent. At each level, a 3×3×3 grid of child boxes is tested; 7 center cells are culled. No geometry stored.

| Parameter | Type | Range | Default | Notes |
|-----------|------|-------|---------|-------|
| `position` | vec3 | — | `(0,0,0)` | Center |
| `orientation` | vec3 | — | `(0,0,0)` | Euler angles (radians) |
| `radius` | float | > 0 | `1.0` | Bounding scale |
| `resolution` | int | 1–5 | `2` | Recursion depth |

### `sierpinski_tetrahedron`
Recursive tetrahedral subdivision SDF.

| Parameter | Type | Range | Default |
|-----------|------|-------|---------|
| `position` | vec3 | — | `(0,0,0)` |
| `radius` | float | > 0 | `1.0` |
| `resolution` | int | 1–8 | `2` |

### `mandelbulb`
3D Mandelbrot set via iterated `z → z^power + c` with distance estimation.

| Parameter | Type | Range | Default |
|-----------|------|-------|---------|
| `position` | vec3 | — | `(0,0,0)` |
| `radius` | float | > 0 | `1.0` |
| `resolution` | int | 1–64 | `18` | March iteration cap |
| `power` | float | 2–16 | `8.0` |
| `bailout` | float | 2–64 | `4.0` |

### `julia`
3D Julia set — same iteration as `mandelbulb` but with a fixed constant.

| Parameter | Type | Range | Default |
|-----------|------|-------|---------|
| `position` | vec3 | — | `(0,0,0)` |
| `julia_c` | vec3 | — | `(-0.24, 0.74, 0.12)` |
| `radius` | float | > 0 | `1.0` |
| `resolution` | int | 1–64 | `18` |
| `power` | float | 2–16 | `8.0` |
| `bailout` | float | 2–64 | `4.0` |

---

## Mesh Generators

Declared as `type = mesh, source = gen(<name>)`. Geometry is built at parse time and accelerated by a BVH; ray cost is O(log N). All accept a `modifiers` block:

```
modifiers = {
    rotation    = vec3(rx, ry, rz)   # applied first
    scale       = vec3(sx, sy, sz)
    translation = vec3(tx, ty, tz)
    flip_normals = true
}
```

### Platonic Solids

No parameters. Fixed geometry.

| Generator | Faces | Description |
|-----------|-------|-------------|
| `tetrahedron` | 4 | Regular tetrahedron |
| `hexahedron` / `cube` | 12 | Unit cube |
| `octahedron` | 8 | Regular octahedron |
| `dodecahedron` | 36 tri | Regular dodecahedron |
| `icosahedron` | 20 | Regular icosahedron |

### Spherical / Dome

| Generator | Key Params | Notes |
|-----------|-----------|-------|
| `icosphere` | `resolution` (≥ 4) | Smooth sphere via icosahedron subdivision |
| `displaced_sphere` | `resolution` (≥ 4), `radius`, `displacement_scale` (0–1), optional `height_sampler` | Icosphere with radial vertex displacement from a texture or procedural sampler |
| `geodesic_dome` | `resolution` (≥ 4) | Upper hemisphere, geodesic faces |
| `hemisphere` | `resolution` (8–512) | Flat-bottomed half-sphere |

**`displaced_sphere` detail** — vertices are displaced radially by `displacement_scale × radius × grayscale(height_sampler(uv))`. UVs use spherical (longitude/latitude) projection. Smooth normals are recomputed after displacement.

| Parameter | Type | Range | Default | Notes |
|-----------|------|-------|---------|-------|
| `resolution` | int | ≥ 4 | `64` | Icosphere subdivision level (multiples of 16 give subdivision iterations 0–3) |
| `radius` | float | > 0 | `1.0` | Base sphere radius before displacement |
| `displacement_scale` | float | 0–1 | `0.05` | Max outward displacement as fraction of radius |
| `height_sampler` | group | — | none | Any sampler type; grayscale value drives displacement. If absent, no displacement is applied. |

```
moon_geo = {
    type               = mesh
    source             = gen(displaced_sphere)
    resolution         = 64
    radius             = 1.0
    displacement_scale = 0.04
    height_sampler = {
        type      = texture
        source    = https://svs.gsfc.nasa.gov/vis/a000000/a004700/a004720/ldem_3_8bit.jpg
        filtering = bilinear
    }
}
```

### Cylinders, Cones, Rings

All take `resolution` (min 12) for radial segment count.

| Generator | Extra Params | Notes |
|-----------|-------------|-------|
| `cylinder` | — | Open-ended circular cylinder |
| `capped_cylinder` | — | Cylinder with flat end caps |
| `cone` | — | Cone with base cap |
| `truncated_cone` | — | Frustum with both caps |
| `capsule` | — | Cylinder with hemispherical caps |
| `disc` | `inner_radius` (≥ 0), `outer_radius` (> 0) | Flat circle or annular ring |
| `ring` / `torus` | `radius`, `height`, `thickness`, `height_resolution` | Torus via profile revolution |
| `rounded_ring` | same + `profile_resolution` (min 8) | Torus with circular cross-section |

### Fractal Meshes

Three approaches for the same fractals — see [Menger Sponge variants](#menger-sponge-variants) below.

| Generator | `resolution` range | Notes |
|-----------|--------------------|-------|
| `menger_sponge` | 1–3 | Explicit cube decomposition; flat-shaded |
| `menger_sponge_implicit` | 1–4 | Voxel grid surface extraction; smooth silhouettes |
| `sierpinski_tetrahedron` | 1–5 | Explicit tetrahedron decomposition |
| `sierpinski_tetrahedron_implicit` | 1–5 | Voxel grid variant |

### Topological / Mathematical Surfaces

| Generator | Key Params | Notes |
|-----------|-----------|-------|
| `mobius_strip` | `radius`, `width`, `resolution` (min 24) | Non-orientable; one-sided |
| `klein_bottle` | `resolution` (min 24) | Non-orientable closed surface |
| `torus_knot` | `resolution` (min 24) | (2,3) knot; no extra params |
| `snowflake` | `resolution` | Koch snowflake fractal curve |
| `superellipsoid` | `e1`, `e2` (0.1–4.0), `resolution` (8–512) | `e=1` diamond, `e=2` sphere, `e>2` cube-like |
| `pyramid` | `base_size` (> 0), `height` (> 0) | Square base, triangular faces |
| `star` | `points` (3–32), `inner_radius`, `outer_radius`, `height` | Extruded 2D star polygon |

### Organic / Generative

| Generator | Key Params | Notes |
|-----------|-----------|-------|
| `rock` | `seed`, `radius`, `roughness` (0–2), `octaves` (1–8), `resolution` | Icosphere perturbed by fBm |
| `shell_spiral` | `turns` (0.5–24), `growth` (0.01–1), `tube_radius`, `resolution` (16–4096) | Nautilus-like logarithmic spiral |
| `hairball` | `seed`, `radius`, `resolution` (8–2048), `fibers` | Radial fiber strands from sphere |
| `tree` | `depth` (1–7), `branch_count` (1–6), `branch_angle`, `trunk_height`, `trunk_radius`, `seed` | Recursive tapered cylinders |
| `coral` | `depth` (1–7), `branch_count` (1–8), `branch_angle`, `height`, `branch_radius`, `seed` | Wider-branching variant of tree |
| `crystal` | `count` (1–32), `radius`, `height`, `tip_height`, `seed` | Cluster of tapered prismatic crystals |

### SVG Silhouette Mesh

```
type = mesh
source = gen(svg)
svg_source = assets/svg/badge.svg
resolution = 128     # raster cells along the longest SVG axis (8–256)
height = 0.12        # extrusion depth along Z
```

`gen(svg)` reads a filled SVG file, samples its filled regions into a 2D occupancy grid, and extrudes that silhouette into a watertight mesh on the `XY` plane with depth along `Z`.

| Parameter | Type | Range | Default | Notes |
|-----------|------|-------|---------|-------|
| `svg_source` | string | required | — | Scene-relative path to the SVG file |
| `resolution` | int | 8–256 | `128` | Sampling density along the longest SVG axis |
| `height` | float | > 0 | `0.12` | Extrusion depth |

Supported SVG subset:
- Filled `path`, `rect`, `circle`, `ellipse`, `polygon`, and filled `polyline` elements
- `viewBox`, `transform`, and `fill-rule="evenodd"`
- Path commands `M`, `L`, `H`, `V`, `C`, `S`, `Q`, `T`, `A`, and `Z`

Current limitations:
- The mesh is voxel-sampled from the SVG fill, not analytically tessellated; increase `resolution` for sharper edges
- Stroke-only artwork is ignored; the generator uses fill regions only
- `use`, clipping/masking, filters, and text layout are not supported

### Mechanical / Industrial

| Generator | Key Params | Notes |
|-----------|-----------|-------|
| `gear` | `tooth_count` (3–128), `tooth_depth`, `inner_radius`, `outer_radius`, `height` | Extruded involute gear |
| `spring` | `coils`, `wire_radius`, `spring_radius`, `height` | Circular profile swept along helix |
| `chain_link` | `count` (1–128), `major_radius`, `minor_radius`, `spacing`, optional `spline` group | Linked tori; spline curves the chain |
| `icosa_cage` | `resolution` (min 8) | Icosahedron wireframe as tubes |

### Surface of Revolution

```
type = mesh
source = gen(lathe)
resolution = 64         # rotational segments (min 12)
cap_ends = true
profile = {
    p0 = tex2(0.0, 0.0)   # (radius, height)
    p1 = tex2(0.5, 0.5)
    p2 = tex2(0.3, 1.0)
}
```

The `profile` group is a list of `tex2(radius, height)` points defining the 2D cross-section to revolve around Y.

### Environmental / Terrain

| Generator | Key Params | Notes |
|-----------|-----------|-------|
| `plane` | `resolution` (min 1) | Subdivided flat quad; `resolution = 1` gives a single quad |
| `terrain` | `resolution` (2–1024), `dimensions` vec3, optional `height_sampler` | fBm-displaced plane |
| `lowpoly_terrain` | `resolution` (2–256), `dimensions` vec3, optional `height_sampler` | Same but faceted normals |
| `draped_cloth_strip` | `dimensions` vec3, `folds`, `edge_lift`, `curl`, `taper`, `sway`, `asymmetry`, `pinned` | Physics-like draped fabric |
| `city` | `seed`, `blocks_x/z`, `block_size`, `road_width`, `building_height_min/max`, `floor_height`, `bay_width`, `window_*`, `pavement_*` | Procedural urban grid |

**`height_sampler` block** (used by terrain generators):
```
height_sampler = {
    type             = scenery_heightfield
    seed             = 1337
    scale            = 0.28
    octaves          = 4
    lacunarity       = 2.0
    gain             = 0.52
    ridge_strength   = 0.82
    mountain_strength = 0.90
    valley_strength  = 0.40
}
```

---

## Mesh — External Import

```
type = mesh
source = ext(relative/path/to/file.obj)   # OBJ, GLTF, FBX
```

---

## Mesh Group

Loads multiple files into a single BVH.

```
type = meshgroup
sources = {
    a = "path/a.obj"
    b = "path/b.obj"
}
# or:
glob = "assets/parts/*.obj"
```

---

## CSG (Constructive Solid Geometry)

Recursively combines signed distance fields. Leaf nodes must be `sphere`, `plane`, `point`, or a fractal type.

```
type = csg
op   = union | soft_union | intersection | difference
smoothness = 0.1        # soft_union only (0.01–1.0)
left  = { type = sphere, position = vec3(0,0,0), radius = 1.2 }
right = { type = sphere, position = vec3(0.8,0,0), radius = 0.9 }
```

CSG trees can be nested up to 32 levels deep.

---

## Menger Sponge Variants

Three independent implementations of the same fractal. All accept `resolution` but differ fundamentally in how they represent the surface.

| | Analytic (`type = menger_sponge`) | Mesh (`gen(menger_sponge)`) | Implicit Mesh (`gen(menger_sponge_implicit)`) |
|---|---|---|---|
| **How it works** | Recursive AABB ray descent; no stored geometry | Explicit cube decomposition; face-culled triangle mesh | Uniform 3D voxel grid (6×3^(n−1) per axis); surface from occupied-neighbor test |
| **Intersection** | Recursive — up to O(20^depth) AABB tests per ray | BVH O(log N) | BVH O(log N) |
| **Memory** | ~200 bytes | ~120 KB (depth 3) | ~5 MB (depth 3) |
| **Build time** | None | Fast (~100 ms) | Slow (O(grid³) evaluations) |
| **Surface quality** | Exact — perfect normals | Faceted (flat shading) | Smooth silhouettes at high grid density |
| **Max resolution** | 5 | 3 | 4 |
| **Best for** | Dynamic/CSG, zero memory cost | Static scenes, render performance | High-quality still renders |

Source files:
- Analytic: [src/xtcore/math/fractal.cc](../src/xtcore/math/fractal.cc)
- Mesh generators: [lib/nmesh/extras.cc](../lib/nmesh/extras.cc)
