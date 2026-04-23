# Scene Format Reference

The xtracer scene format is NCF (Nested Configuration Format) — a block-structured text format. This document covers the top-level structure. For block-specific content, see the linked reference docs.

Source: `src/xtcore/parseutil.cc` (`load()`), `src/xtcore/proto.h`.

---

## File Structure

```
title       = My Scene
description = A short description.
version     = 1.0
default_camera = hero

environment = { ... }

camera   = { ... }
geometry = { ... }
material = { ... }
medium   = { ... }   # optional
object   = { ... }
variants = { ... }   # optional
```

The parser processes blocks in this order regardless of their position in the file: `environment` → `medium` → `camera` → `geometry` → `material` → `object`. Blocks can appear in any order in the file itself — the parse order above is enforced internally.

---

## Top-Level Properties

| Property | Type | Notes |
|----------|------|-------|
| `title` | string | Human-readable scene name |
| `description` | string | Free-form description |
| `version` | string | Arbitrary version string |
| `default_camera` | string | Name of the camera used if none is specified at render time |
| `ambient` | col3 | Ambient light color; multiplied by `k_ambient` |
| `k_ambient` | float | Ambient scale factor (default `1.0`) |

---

## `environment` Block

Defines the background and infinite-distance illumination. Exactly one environment per scene.

```
environment = {
    type   = <type>
    config = { ... }
}
```

The `config` sub-block holds type-specific parameters. See `docs/SAMPLERS.md` for all environment sampler types (`gradient`, `rayleigh_sky`, `erp`, `cubemap`, `color`).

---

## `camera` Block

One or more named cameras. The active camera at render time is determined by `default_camera` or the render request.

```
camera = {
    hero = {
        type = thin-lens
        ...
    }
    wide = {
        type = thin-lens
        ...
    }
}
```

See `docs/CAMERAS.md` for all camera types and their parameters.

---

## `geometry` Block

Named shape definitions. Geometry is not placed in the scene until it is referenced by an `object` entry.

```
geometry = {
    floor = {
        type = plane
        normal   = vec3(0,1,0)
        distance = 0
    }

    ball = {
        type   = mesh
        source = gen(icosphere)
        resolution = 3
        modifiers = {
            scale       = vec3(1,1,1)
            rotation    = vec3(0,0,0)
            translation = vec3(0,1,0)
        }
    }
}
```

See `docs/GEOMETRY.md` for all geometry types, generators, and their parameters.

`gen(svg)` uses one extra geometry property:

```
geometry = {
    badge = {
        type       = mesh
        source     = gen(svg)
        svg_source = assets/svg/badge.svg
        resolution = 128
        height     = 0.12
    }
}
```

`svg_source` is resolved relative to the scene file, the same way image and mesh asset paths are.

---

## `material` Block

Named material definitions. Materials are not applied until referenced by an `object` entry.

```
material = {
    mat_floor = {
        type = principled
        properties = {
            samplers = {
                base_color = { type = color, value = col3(0.8, 0.8, 0.8) }
            }
            scalars = {
                roughness = 0.5
                metallic  = 0.0
            }
        }
    }
}
```

See `docs/MATERIALS.md` for all material types, sampler slots, and scalar parameters.
See `docs/SAMPLERS.md` for sampler types used inside `properties.samplers`.

---

## `medium` Block

Optional. Defines volumetric participating media. Named media are referenced from `object` entries.

```
medium = {
    fog = {
        type = homogeneous
        properties = {
            scalars = {
                sigma_s = 0.08
                sigma_a = 0.01
                g       = 0.0
            }
        }
    }
}
```

| Type | Notes |
|------|-------|
| `homogeneous` | Spatially uniform medium |
| `heterogeneous_noise` | Density varies via procedural noise |

**Scalar properties**

| Property | Notes |
|----------|-------|
| `sigma_s` | Scattering coefficient |
| `sigma_a` | Absorption coefficient |
| `g` | Phase function anisotropy; −1 = back-scatter, 0 = isotropic, 1 = forward-scatter |
| `emission` | Emitted radiance (col3) |
| `density` | Density multiplier |
| `noise_scale` | Noise frequency (`heterogeneous_noise` only) |
| `noise_min` / `noise_max` | Density variation range (`heterogeneous_noise` only) |

---

## `object` Block

Instantiates scene objects by pairing a geometry definition with a material definition. This is the only block that places things in the scene.

```
object = {
    floor     = { geometry = floor,  material = mat_floor }
    sphere_01 = { geometry = ball,   material = mat_glass }
    key_light = { geometry = lamp,   material = mat_emissive }
    fog_box   = { geometry = box,    material = mat_boundary, medium = fog }
}
```

**Properties**

| Property | Required | Notes |
|----------|----------|-------|
| `geometry` | Yes | Name of a geometry definition in the `geometry` block |
| `material` | Yes | Name of a material definition in the `material` block |
| `medium` | No | Name of a medium definition in the `medium` block |

**External object import**

Objects can also be loaded directly from an OBJ file, bypassing the geometry/material block system:

```
object = {
    dragon = {
        source = "models/dragon.obj"
        prefix = "dragon"
    }
}
```

When `source` is set the parser imports the file and creates one object per sub-mesh, named `<prefix>_<mesh_name>`. The `medium` property is not supported on external objects.

---

## `variants` Block

Optional. Defines named overlays that selectively override parts of the base scene. The renderer applies exactly one variant (or none) per render job.

```
variants = {
    frosted = {
        name        = Frosted Glass
        description = All glass objects use frosted material.
        set = {
            default_camera = close_up
            object = {
                sphere_01 = { geometry = ball, material = mat_frosted }
            }
        }
    }

    no_fog = {
        name = No Fog
        remove = {
            object = {
                fog_box = {}
            }
        }
    }
}
```

**Variant properties**

| Property | Notes |
|----------|-------|
| `name` | Human-readable label shown in the UI |
| `description` | Optional description |
| `set` | Override block — any top-level block or property can appear here |
| `remove` | Remove block — lists named entries to delete from the base scene |

The `set` block is merged on top of the base scene before rendering. It can override top-level properties (e.g. `default_camera`), replace individual object entries, or add new geometry/material/object definitions. The `remove` block deletes named entries by name; the value is ignored (use `{}`).

---

## Value Syntax

| Type | Syntax | Example |
|------|--------|---------|
| Float | bare number | `1.5` |
| String | bare or quoted | `thin-lens` / `"path/to/file.obj"` |
| vec3 | `vec3(x,y,z)` | `vec3(0, 1, 0)` |
| col3 | `col3(r,g,b)` | `col3(0.8, 0.3, 0.1)` |
| Comment | `#` to end of line | `# this is ignored` |

Paths in `source`, `glob`, and sampler `source` fields are resolved relative to the scene file's directory.
