Create a complete, renderable xtracer scene file in the NCF format.

## Your task

Write a valid `.ncf` scene file based on what the user wants to render. If the user gave a description or subject in their message, use it. If they gave no description, ask one clarifying question — what they want the scene to show — then proceed.

Output the complete scene file as a single fenced code block, followed by a short paragraph explaining the key creative choices (lighting setup, material strategy, camera framing). Then suggest a file path under `scene/` that fits the subject.

---

## Scene format rules (follow exactly)

**Top-level structure — use all that apply:**
```
title       = "..."
description = "..."
version     = "1.0"

environment = { type = ..., config = { ... } }
camera      = { default = { type = thin-lens, ... } }
geometry    = { name = { type = ..., ... }, ... }
material    = { name = { type = ..., properties = { samplers = { ... }, scalars = { ... } } }, ... }
object      = { name = { geometry = ..., material = ... }, ... }
```

**Value syntax:**
- `col3(r,g,b)` — colour; values 0–1 for normal surfaces, higher (e.g. `col3(10,8,6)`) for emissive light sources
- `vec3(x,y,z)` — 3D vector; coordinate system is right-handed, Y-up
- `tex2(u,v)` — 2D texture coordinate

---

## Choosing an environment

Pick whichever fits the mood:

| Situation | Type | Example config |
|-----------|------|----------------|
| Outdoor daylight | `rayleigh_sky` | `sun_direction = vec3(0.4,0.9,0.2)`, `sun_intensity = col3(6,5,4)`, `beta_rayleigh = col3(0.014,0.035,0.085)`, `ground_color = col3(0.12,0.10,0.08)` |
| Studio / indoor | `gradient` | `a = col3(0.15,0.15,0.18)`, `b = col3(0.05,0.05,0.06)` |
| Night / dark | `color` | `value = col3(0.02,0.02,0.04)` |
| HDR panorama | `erp` | `source = "path/to/env.hdr"` |

Always add explicit `emissive` light objects when using `gradient` or `color` environments — they produce no illumination on their own.

---

## Lighting rules

- **At least one light source is mandatory.** A scene with no emissive objects and a dark environment renders black.
- Use emissive spheres as area lights: `type = emissive`, sampler slot `emissive`, value `col3(12,10,8)` or higher.
- Three-light setup for clear subjects: key light (large, bright, off to one side), fill light (smaller, dimmer, opposite side), optional rim light (behind/above subject).
- Place lights where rays can reach them — behind opaque walls they do nothing.
- `rayleigh_sky` provides free directional lighting; supplement with a ground fill (low emissive plane) to lift shadows.

---

## Geometry selection guide

**Use primitives when exact maths is enough:**
- `sphere` — balls, droplets, planets, light sources
- `plane` — floors, walls, infinite backdrops
- `triangle` — custom facets

**Use procedural generators for everything else** (`type = mesh, source = gen(<name>)`). Full parameter reference: `docs/GEOMETRY.md`.

| Subject | Generator | Useful params |
|---------|-----------|---------------|
| Smooth sphere | `icosphere` | `resolution = 4` |
| Box / architectural | `hexahedron` | `modifiers.scale` to reshape |
| Organic blob | `rock` | `roughness = 0.4`, `octaves = 5` |
| Natural shell | `shell_spiral` | `turns = 5`, `growth = 0.22`, `tube_radius = 0.12` |
| Fabric / cloth | `draped_cloth_strip` | `folds = 4`, `curl = 0.3` |
| Fractal / abstract | `menger_sponge` | `resolution = 3` |
| Mathematical surface | `mobius_strip`, `torus_knot`, `klein_bottle` | — |
| Terrain / ground | `terrain` | `resolution = 256`, `dimensions = vec3(10,1.2,10)` |
| Chain / interlocked | `chain_link` | `count = 8`, `major_radius = 0.5` |
| Revolved profile | `lathe` | profile points as `tex2(radius, height)` |

Always add `resolution = 48` or higher for anything that must look smooth. Add a `modifiers` block to position and orient without moving the camera.

**CSG for boolean shapes:**
```
type = csg
op   = soft_union          # or: union, intersection, difference
smoothness = 0.1           # only for soft_union
left  = { type = sphere, position = vec3(0,0,0), radius = 1.2 }
right = { type = sphere, position = vec3(0.8,0,0), radius = 0.9 }
```

**Floor plane — include in almost every scene:**
```
floor = { type = plane, normal = vec3(0,1,0), distance = -1.0 }
```

---

## Material selection guide

Default to `principled` for physical surfaces. Use specialised types only when the effect is prominent.

| Surface | Material | Key settings |
|---------|----------|-------------|
| Generic matte | `principled` | `metallic = 0`, `roughness = 0.6–0.8` |
| Plastic / ceramic | `principled` | `metallic = 0`, `roughness = 0.2–0.4`, `clearcoat = 0.5` |
| Polished metal | `principled` | `metallic = 1`, `roughness = 0.05–0.2` |
| Brushed metal | `principled` | `metallic = 1`, `roughness = 0.4–0.6` |
| Glass | `dielectric` | `ior = 1.52`, `transparency = 1`, `reflectance = 0.04` |
| Frosted glass | `rough_dielectric` | `ior = 1.5`, `roughness = 0.3`, `transparency = 0.9` |
| Skin / wax | `subsurface` | `scattering_distance = 0.4`, `scattering_strength = 0.6` |
| Velvet / cloth | `sheen` | `sheen = 0.9`, provide both `base_color` and `sheen_color` |
| Light source | `emissive` | `emissive` sampler with `col3(10,8,6)` or brighter |
| Diffuse floor | `lambert` | simple, fast, no specularity |
| Marble / procedural | `principled` | use `fbm_marble` sampler for `base_color` |

**Sampler quick-reference inside `properties.samplers`:**
```
diffuse    = { type = color,    value = col3(0.8,0.3,0.1) }
diffuse    = { type = checker,  a = col3(1,1,1), b = col3(0.1,0.1,0.1), scale_u = 4, scale_v = 4 }
diffuse    = { type = fbm_marble, a = col3(0.9,0.85,0.8), b = col3(0.6,0.55,0.5), vein = col3(0.2,0.18,0.16), scale = 2, turbulence = 0.5, vein_frequency = 2 }
normal     = { type = voronoi_normal, cells = 200, max_deviation = 12, seed = 42 }
emissive   = { type = color,    value = col3(12,10,8) }
```

---

## Camera setup

```
default = {
    type     = thin-lens
    position = vec3(0, 2, -6)
    target   = vec3(0, 0, 0)
    up       = vec3(0, 1, 0)
    fov      = 45
}
```

Framing guide:
- `fov = 45–60` — neutral, general use
- `fov = 28–40` — portrait/product, slight compression
- `fov = 65–80` — wide, environmental, architectural
- DOF: add `flength = <focus_distance_in_world_units>` and `aperture = 0.03–0.15`; `aperture_blades = 6` for hexagonal bokeh

`up` must not be parallel to the view direction (`position → target` vector).

---

## Complete minimal example (for reference, do not copy verbatim)

```
title       = "Studio Sphere"
description = "Single sphere under three-point studio lighting"
version     = "1.0"

environment = {
    type   = gradient
    config = { a = col3(0.12,0.12,0.15), b = col3(0.04,0.04,0.05) }
}

camera = {
    default = {
        type     = thin-lens
        position = vec3(0,2,-5)
        target   = vec3(0,0,0)
        up       = vec3(0,1,0)
        fov      = 45
    }
}

geometry = {
    subject   = { type = sphere, position = vec3(0,0,0), radius = 1 }
    floor     = { type = plane,  normal = vec3(0,1,0), distance = -1 }
    key_geo   = { type = sphere, position = vec3(-3,4,-1), radius = 0.6 }
    fill_geo  = { type = sphere, position = vec3(3,2,-3),  radius = 0.8 }
    rim_geo   = { type = sphere, position = vec3(0,3,3),   radius = 0.4 }
}

material = {
    ceramic = {
        type = principled
        properties = {
            samplers = { base_color = { type = color, value = col3(0.9,0.9,0.95) } }
            scalars  = { metallic = 0, roughness = 0.15, clearcoat = 0.8, clearcoat_roughness = 0.1 }
        }
    }
    floor_mat = {
        type = lambert
        properties = { samplers = { diffuse = { type = color, value = col3(0.4,0.4,0.42) } } }
    }
    key_light  = { type = emissive, properties = { samplers = { emissive = { type = color, value = col3(14,12,9)  } } } }
    fill_light = { type = emissive, properties = { samplers = { emissive = { type = color, value = col3(4,5,8)    } } } }
    rim_light  = { type = emissive, properties = { samplers = { emissive = { type = color, value = col3(10,9,7)   } } } }
}

object = {
    subject  = { geometry = subject,  material = ceramic    }
    floor    = { geometry = floor,    material = floor_mat  }
    key      = { geometry = key_geo,  material = key_light  }
    fill     = { geometry = fill_geo, material = fill_light }
    rim      = { geometry = rim_geo,  material = rim_light  }
}
```

---

## Quality checklist before outputting

- [ ] At least one `emissive` object or `rayleigh_sky` provides illumination
- [ ] Every `geometry` name referenced in `object` is defined in `geometry`
- [ ] Every `material` name referenced in `object` is defined in `material`
- [ ] Camera `up` is not parallel to the view direction
- [ ] Glass/SSS/sheen materials have a scene that can actually reveal the effect
- [ ] `emissive` values are bright enough to illuminate (≥ `col3(6,5,4)`)
- [ ] A floor plane is present unless intentionally floating in space
- [ ] `resolution` is set on any generator that needs to look smooth (≥ 48 for hero geometry)
