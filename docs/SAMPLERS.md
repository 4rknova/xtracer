# Sampler Reference

All procedural and image-based sampler types supported by xtracer's scene format. Source: `src/xtcore/parseutil.cc`, `src/xtcore/sampler/`.

Samplers are used in two contexts:

- **Material slots** — assigned inside `material.<name>.properties.samplers.<slot>` (e.g. `base_color`, `normal`, `emissive`, `roughness`)
- **Environment** — assigned as the top-level `environment` block
- **Height sampler** — assigned as `geometry.<name>.height_sampler` for terrain-type meshes (`terrain`, `lowpoly_terrain`, `draped_cloth_strip`)

---

## Solid Color

### `color`
Uniform color across the entire surface.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `value` | col3 | `col3(0,0,0)` | |

```
diffuse = { type = color, value = col3(0.8, 0.3, 0.1) }
```

---

## Image-Based

### `texture`
Loads an image file and samples it at UV coordinates.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `source` | string | — | File path to image |
| `filtering` | string | `"bilinear"` | `"nearest"` or `"bilinear"` |
| `flip_x` | bool | `false` | |
| `flip_y` | bool | `false` | |

```
diffuse = { type = texture, source = "assets/wood.png", filtering = bilinear }
```

### `erp`
Equirectangular projection image, typically used as an environment map.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `source` | string | — | File path to HDR or LDR image |
| `filtering` | string | `"bilinear"` | `"nearest"` or `"bilinear"` |

```
environment = {
    type = erp
    config = { source = "hdri/sunlit_field.hdr" }
}
```

### `cubemap`
Six-face cubemap, typically used as an environment map.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `posx` | string | — | Right (+X) face |
| `negx` | string | — | Left (−X) face |
| `posy` | string | — | Top (+Y) face |
| `negy` | string | — | Bottom (−Y) face |
| `posz` | string | — | Front (+Z) face |
| `negz` | string | — | Back (−Z) face |

```
environment = {
    type = cubemap
    config = {
        posx = "sky_right.hdr"  negx = "sky_left.hdr"
        posy = "sky_top.hdr"    negy = "sky_bottom.hdr"
        posz = "sky_front.hdr"  negz = "sky_back.hdr"
    }
}
```

---

## Procedural — Pattern

### `checker`
Classic two-color checkerboard.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `a` | col3 | `col3(0.92, 0.92, 0.92)` | First color |
| `b` | col3 | `col3(0.08, 0.08, 0.08)` | Second color |
| `scale_u` | float | `2.0` | Tiles per unit in U |
| `scale_v` | float | `2.0` | Tiles per unit in V |
| `offset_u` | float | `0.0` | Grid offset in U |
| `offset_v` | float | `0.0` | Grid offset in V |

```
diffuse = { type = checker, a = col3(1,1,1), b = col3(0.1,0.1,0.1), scale_u = 4, scale_v = 4 }
```

### `graphpaper`
Graph paper with minor and major grid lines.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `base` | col3 | `col3(0.93, 0.93, 0.93)` | Background color |
| `minor` | col3 | `col3(0.71, 0.83, 0.89)` | Minor grid line color |
| `major` | col3 | `col3(0.30, 0.63, 0.77)` | Major grid line color |
| `scale` | float | `16.0` | Cells per unit |
| `minor_width` | float | `0.020` | Minor line width as fraction of cell |
| `major_width` | float | `0.050` | Major line width as fraction of cell |
| `major_every` | int | `5` | Major line every N cells |

```
diffuse = { type = graphpaper, scale = 8, major_every = 4 }
```

### `weave`
Woven textile with visible warp and weft threads.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `base` | col3 | `col3(0.14, 0.13, 0.12)` | Background color |
| `warp` | col3 | `col3(0.86, 0.80, 0.72)` | Warp thread color |
| `weft` | col3 | `col3(0.28, 0.62, 0.68)` | Weft thread color |
| `scale` | float | `12.0` | Thread density |
| `band_width` | float | `0.66` | Thread band width; range 0.02–0.98 |

```
diffuse = { type = weave, scale = 10, warp = col3(0.9,0.9,0.9), weft = col3(0.2,0.2,0.2) }
```

---

## Procedural — Noise

### `fbm_marble`
Procedural marble using sinusoidal veins distorted by fBm.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `a` | col3 | `col3(0.93, 0.92, 0.90)` | Light marble color |
| `b` | col3 | `col3(0.62, 0.60, 0.58)` | Dark marble color |
| `vein` | col3 | `col3(0.18, 0.17, 0.16)` | Vein color |
| `scale` | float | `6.0` | Noise frequency |
| `vein_frequency` | float | `9.0` | Vein stripe frequency |
| `turbulence` | float | `3.5` | Vein distortion amount |
| `octaves` | int | `5` | fBm octave count |
| `lacunarity` | float | `2.0` | fBm frequency multiplier per octave |
| `gain` | float | `0.5` | fBm amplitude multiplier per octave |
| `vein_strength` | float | `0.85` | Vein blend weight; range 0–1 |
| `vein_sharpness` | float | `4.0` | Vein sharpness exponent |

```
diffuse = {
    type = fbm_marble
    a = col3(0.9, 0.85, 0.8)
    b = col3(0.6, 0.55, 0.5)
    vein = col3(0.2, 0.18, 0.16)
    scale = 2
    turbulence = 0.5
}
```

### `scenery_heightfield`
Grayscale fBm heightfield, typically used as a `height_sampler` on terrain geometry.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `seed` | int | `1337` | |
| `scale` | float | `0.25` | Noise frequency |
| `octaves` | int | `5` | fBm octave count |
| `lacunarity` | float | `2.0` | fBm frequency multiplier per octave |
| `gain` | float | `0.5` | fBm amplitude multiplier per octave; range 0.05–0.95 |
| `ridge_strength` | float | `0.65` | Ridge prominence |
| `mountain_strength` | float | `0.75` | Mountain sharpness |
| `valley_strength` | float | `0.55` | Valley depth |

```
height_sampler = {
    type = scenery_heightfield
    seed = 42
    scale = 0.3
    octaves = 6
    ridge_strength = 0.7
}
```

---

## Procedural — Normal Map

### `voronoi_normal`
Generates a tangent-space normal map by assigning a random normal within a cone to each Voronoi cell. Output is packed RGB: `(n.x+1, n.y+1, n.z+1) / 2`.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `cells` | int | `64` | Number of Voronoi cells |
| `max_deviation` | float | `12.0` | Max normal deviation from (0,0,1) in degrees; range 0–89 |
| `seed` | int | `1337` | |

```
normal = { type = voronoi_normal, cells = 200, max_deviation = 15, seed = 42 }
```

---

## Environment — Sky

### `gradient`
Simple two-color vertical sky gradient. Color interpolates based on the Y component of the direction vector.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `a` | col3 | `col3(1, 1, 1)` | Bottom/horizon color |
| `b` | col3 | `col3(0.5, 0.7, 1)` | Top/zenith color |

```
environment = {
    type = gradient
    config = { a = col3(0.96,0.98,1.00), b = col3(0.45,0.62,0.92) }
}
```

### `rayleigh_sky`
Physically-based sky with a Rayleigh scattering model, sun disk, and glow.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `sun_direction` | vec3 | `vec3(0.35, 0.8, 0.2)` | Normalized sun direction |
| `sun_intensity` | col3 | `col3(24, 22, 18)` | Sun radiance |
| `beta_rayleigh` | col3 | `col3(0.18, 0.35, 0.80)` | Per-channel scattering coefficients |
| `ground_color` | col3 | `col3(0.02, 0.02, 0.03)` | Below-horizon fill |
| `density` | float | `1.0` | Atmospheric density scale |
| `horizon_falloff` | float | `1.5` | Horizon falloff exponent |
| `sun_disk_radius` | float | `1.0` | Sun disk angular radius (degrees) |
| `sun_disk_intensity` | float | `1.0` | Disk brightness multiplier |
| `sun_glow_radius` | float | `8.0` | Glow angular extent (degrees) |
| `sun_glow_intensity` | float | `0.35` | Glow brightness multiplier |
| `sun_glow_falloff` | float | `4.0` | Glow falloff exponent |

```
environment = {
    type = rayleigh_sky
    config = {
        sun_direction   = vec3(0.4, 0.9, 0.2)
        sun_intensity   = col3(6, 5, 4)
        beta_rayleigh   = col3(0.014, 0.035, 0.085)
        ground_color    = col3(0.12, 0.10, 0.08)
    }
}
```

### `preetham_sky`
Analytical sky model based on Preetham, Shirley & Smits 1999. Computes CIE xyY sky luminance and chromaticity via the Perez function, converted to linear sRGB. Accurate across a turbidity range of 1.7 (very clear) to 10 (very hazy).

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `sun_direction` | vec3 | `vec3(0.35, 0.8, 0.2)` | Normalized sun direction |
| `turbidity` | float | `3.0` | Atmospheric turbidity; range 1.7–10 |
| `exposure` | float | `0.04` | Radiance-to-display scale |
| `ground_color` | col3 | `col3(0.02, 0.02, 0.02)` | Below-horizon fill |

```
environment = {
    type = preetham_sky
    config = {
        sun_direction = vec3(0.4, 0.75, 0.2)
        turbidity     = 2.5
        exposure      = 0.05
    }
}
```

### `hosek_wilkie_sky`
Extended analytical sky model based on Hosek & Wilkie 2012. Improves upon Preetham near the horizon and at high turbidities. Uses a 9-parameter radiance function evaluated per RGB channel, with a ground-albedo correction term.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `sun_direction` | vec3 | `vec3(0.35, 0.8, 0.2)` | Normalized sun direction |
| `turbidity` | float | `3.0` | Atmospheric turbidity; range 1–10 |
| `ground_albedo` | float | `0.3` | Ground reflectance (affects horizon brightness) |
| `exposure` | float | `1.0` | Radiance-to-display scale |
| `ground_color` | col3 | `col3(0.02, 0.02, 0.02)` | Below-horizon fill |

```
environment = {
    type = hosek_wilkie_sky
    config = {
        sun_direction = vec3(0.3, 0.6, 0.2)
        turbidity     = 4.0
        ground_albedo = 0.2
        exposure      = 1.2
    }
}
```

### `stars`
Procedural starfield for night or space environments. Stars are Gaussian splats with randomised brightness and slight colour-temperature variation (hot = blue-white, cool = orange-white). Designed for use as a spherical environment.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `background` | col3 | `col3(0, 0, 0.02)` | Sky background colour |
| `density` | float | `0.015` | Fraction of grid cells that contain a star; range 0–1 |
| `min_brightness` | float | `0.4` | Dimmest star brightness |
| `max_brightness` | float | `1.0` | Brightest star brightness |
| `star_size` | float | `0.015` | Gaussian sigma in grid-cell units |
| `seed` | int | `42` | |

```
environment = {
    type = stars
    config = {
        background    = col3(0, 0, 0)
        density       = 0.02
        min_brightness = 0.3
        max_brightness = 1.0
        star_size     = 0.012
    }
}
```

---

## Procedural — Noise (continued)

### `fbm_wood`
Concentric ring wood grain distorted by fBm turbulence.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `a` | col3 | `col3(0.72, 0.48, 0.22)` | Light grain colour |
| `b` | col3 | `col3(0.36, 0.20, 0.08)` | Dark grain colour |
| `scale` | float | `4.0` | Overall noise frequency |
| `ring_frequency` | float | `8.0` | Rings per unit |
| `turbulence` | float | `2.0` | fBm distortion amount |
| `octaves` | int | `4` | fBm octave count |
| `lacunarity` | float | `2.0` | fBm frequency multiplier per octave |
| `gain` | float | `0.5` | fBm amplitude multiplier per octave |

```
diffuse = { type = fbm_wood, a = col3(0.8, 0.55, 0.25), b = col3(0.4, 0.22, 0.08), ring_frequency = 10 }
```

### `curl_noise`
Swirling, divergence-free flow pattern computed from the numerical curl of an fBm field. Produces fluid-like streaks and vortices.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `a` | col3 | `col3(0.10, 0.18, 0.42)` | Low-value colour |
| `b` | col3 | `col3(0.82, 0.90, 0.98)` | High-value colour |
| `scale` | float | `3.0` | Noise frequency |
| `strength` | float | `1.0` | Curl displacement magnitude |
| `octaves` | int | `4` | fBm octave count |
| `lacunarity` | float | `2.0` | fBm frequency multiplier per octave |
| `gain` | float | `0.5` | fBm amplitude multiplier per octave |

```
diffuse = { type = curl_noise, scale = 4, strength = 1.5 }
```

---

## Procedural — Surface Detail

### `scratches`
Anisotropic hair-line scratches for metal wear and brushed surfaces. Randomly oriented line segments are distributed across tiled cells; each scratch is sampled as a capped line-segment SDF.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `base` | col3 | `col3(0.60, 0.60, 0.62)` | Base metal colour |
| `scratch` | col3 | `col3(0.90, 0.90, 0.92)` | Scratch highlight colour |
| `scale` | float | `4.0` | Tile density |
| `density` | int | `24` | Scratch segments per tile |
| `width` | float | `0.008` | Scratch width in UV space |
| `angle` | float | `0.0` | Mean scratch angle (radians) |
| `angle_jitter` | float | `0.3` | Random angle deviation (fraction of π) |
| `seed` | int | `42` | |

```
roughness = { type = scratches, density = 32, width = 0.005, angle_jitter = 0.15 }
```

### `edge_wear`
Voronoi-based edge wear: Worley F2−F1 distance highlights cell boundaries, simulating worn or chipped edges on tiled surfaces.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `base` | col3 | `col3(0.50, 0.48, 0.46)` | Unworn surface colour |
| `worn` | col3 | `col3(0.92, 0.90, 0.88)` | Worn edge colour |
| `scale` | float | `8.0` | Voronoi cell density |
| `sharpness` | float | `6.0` | Power curve exponent; higher = crisper edge |
| `coverage` | float | `0.5` | Edge wear width; range 0–1 |
| `seed` | int | `1337` | |

```
roughness = { type = edge_wear, scale = 6, sharpness = 8, coverage = 0.3 }
```


## Procedural — Pattern (continued)

### `brick`
Staggered brick pattern with mortar joints. Odd rows are offset by half a brick width. Per-brick brightness variation is driven by a hash.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `brick` | col3 | `col3(0.72, 0.32, 0.22)` | Brick face colour |
| `mortar` | col3 | `col3(0.72, 0.70, 0.66)` | Mortar colour |
| `scale_u` | float | `8.0` | Bricks per unit in U |
| `scale_v` | float | `4.0` | Bricks per unit in V |
| `mortar_u` | float | `0.06` | Mortar joint width in U; range 0–1 |
| `mortar_v` | float | `0.10` | Mortar joint width in V; range 0–1 |
| `color_variation` | float | `0.08` | Per-brick brightness variation |
| `seed` | int | `42` | |

```
diffuse = { type = brick, brick = col3(0.65, 0.28, 0.18), mortar = col3(0.75, 0.73, 0.70), scale_u = 6 }
```

### `dots`
Regular polka-dot / halftone grid. Dot centres sit at integer lattice points; distance is thresholded with soft edge.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `bg` | col3 | `col3(0.92, 0.92, 0.92)` | Background colour |
| `dot` | col3 | `col3(0.10, 0.10, 0.10)` | Dot colour |
| `scale` | float | `8.0` | Dots per unit |
| `radius` | float | `0.35` | Dot radius as fraction of cell; range 0–0.5 |
| `softness` | float | `0.05` | Edge softness |

```
diffuse = { type = dots, bg = col3(1,1,1), dot = col3(0,0,0), scale = 10, radius = 0.3 }
```

---

## Compositing

### `blend`
Linearly interpolates between two child samplers by a constant factor.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `a` | group | — | Child sampler at t=0 |
| `b` | group | — | Child sampler at t=1 |
| `t` | float | `0.5` | Blend factor; range 0–1 |

```
diffuse = {
    type = blend
    t    = 0.4
    a    = { type = color, value = col3(0.8, 0.2, 0.1) }
    b    = { type = checker, a = col3(1,1,1), b = col3(0,0,0) }
}
```

### `mix_masked`
Composites `overlay` over `base` using an optional `mask` sampler (or constant `t`) and a selectable blend mode.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `base` | group | — | Background sampler |
| `overlay` | group | — | Foreground sampler |
| `mask` | group | — | Grayscale mask sampler (average of RGB); if absent, `t` is used |
| `t` | float | `1.0` | Constant blend when no mask; range 0–1 |
| `mode` | string | `"lerp"` | `"lerp"`, `"multiply"`, `"add"`, or `"screen"` |

```
diffuse = {
    type    = mix_masked
    mode    = multiply
    t       = 1.0
    base    = { type = fbm_wood }
    overlay = { type = edge_wear }
    mask    = { type = voronoi_normal, cells = 32 }
}
```

### `triplanar`
Projects a single child sampler along all three axes and blends the results by the axis-aligned weights derived from the 3D sample position (uvw). Eliminates seams on arbitrary meshes without UV unwrapping.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `child` | group | — | The sampler to project |
| `scale` | float | `1.0` | UV scale applied before projecting |
| `blend_sharpness` | float | `4.0` | Blend weight exponent; higher = sharper axis transitions |

```
diffuse = {
    type            = triplanar
    scale           = 2.0
    blend_sharpness = 6.0
    child           = { type = checker, a = col3(0.9,0.9,0.9), b = col3(0.1,0.1,0.1) }
}
```
