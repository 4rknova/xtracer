# Material Reference

All material types supported by xtracer's scene format. Source: `src/xtcore/parseutil.cc`, `src/xtcore/material/`, `src/xtcore/matdefs.h`.

---

## NCF Structure

```
material = {
    my_mat = {
        type = <type>
        properties = {
            samplers = {
                <slot> = { type = <sampler_type>, ... }
            }
            scalars = {
                <param> = <value>
            }
        }
    }
}
```

Sampler slot names and scalar parameter names are listed per material type below. See `docs/SAMPLERS.md` for sampler types.

---

## Universal Slot

Any material type can emit light by adding an `emissive` sampler. The integrator calls `is_emissive()` which checks for the presence of this slot — material type is irrelevant. This is how area lights work: any geometry with an `emissive` sampler on its material is picked up as a light source by `pathtracer_mis`.

```
samplers = {
    emissive = { type = color, value = col3(4, 3.5, 2.8) }
}
```

---

## Material Types

### `lambert`
Pure Lambertian diffuse. No specularity.

**Samplers**

| Slot | Meaning |
|------|---------|
| `diffuse` | Diffuse reflectance color |
| `normal` | Tangent-space normal map (optional) |

**Scalars** — none

```
type = lambert
properties = {
    samplers = {
        diffuse = { type = color, value = col3(0.7, 0.3, 0.1) }
    }
}
```

---

### `phong`
Diffuse + Phong specular lobe.

**Samplers**

| Slot | Meaning |
|------|---------|
| `diffuse` | Diffuse color |
| `specular` | Specular highlight color |
| `normal` | Tangent-space normal map (optional) |

**Scalars**

| Parameter | Range | Notes |
|-----------|-------|-------|
| `exponent` | ≥ 1 | Phong exponent; higher = sharper highlight |
| `reflectance` | 0–1 | Diffuse/specular lobe split weight |

---

### `blinn_phong`
Diffuse + Blinn-Phong specular lobe (half-vector formulation).

**Samplers**

| Slot | Meaning |
|------|---------|
| `diffuse` | Diffuse color |
| `specular` | Specular highlight color |
| `normal` | Tangent-space normal map (optional) |

**Scalars**

| Parameter | Range | Notes |
|-----------|-------|-------|
| `exponent` | ≥ 1 | Blinn-Phong exponent |
| `reflectance` | 0–1 | Lobe split weight |

---

### `emissive`
Purely emissive surface. No reflection or transmission — all sampled radiance comes from the `emissive` slot.

**Samplers**

| Slot | Meaning |
|------|---------|
| `emissive` | Emitted radiance |

**Scalars** — none

```
type = emissive
properties = {
    samplers = {
        emissive = { type = color, value = col3(8, 7, 6) }
    }
}
```

---

### `dielectric`
Perfect (delta) glass: Fresnel-weighted reflection and refraction with no roughness.

**Samplers** — none

**Scalars**

| Parameter | Default | Range | Notes |
|-----------|---------|-------|-------|
| `ior` | `1.5` | > 1.0 | Index of refraction |
| `transparency` | `1.0` | 0–1 | Transmission tint (1 = clear) |
| `reflectance` | `0.0` | 0–1 | Manual Fresnel override; if non-zero overrides physical Fresnel |

```
type = dielectric
properties = {
    scalars = { ior = 1.5, transparency = 1.0 }
}
```

---

### `rough_dielectric`
Frosted/ground glass using GGX microfacet transmission. Supports optional Beer-Lambert absorption.

**Samplers**

| Slot | Meaning |
|------|---------|
| `transmission` | Transmitted light color |
| `roughness` | Roughness texture override (luminance channel used) |
| `absorption_color` | Beer-Lambert absorption tint (optional) |
| `normal` | Tangent-space normal map (optional) |

**Scalars**

| Parameter | Default | Range | Notes |
|-----------|---------|-------|-------|
| `ior` | `1.5` | > 1.0 | Index of refraction |
| `roughness` | `0.02` | 0.02–1.0 | GGX alpha; 0.02 = nearly smooth |
| `transparency` | `1.0` | 0–1 | Transmission strength |
| `absorption_distance` | `0` | ≥ 0 | Beer-Lambert path length scale; 0 = no absorption |

```
type = rough_dielectric
properties = {
    samplers = {
        transmission = { type = color, value = col3(0.95, 0.98, 1.0) }
    }
    scalars = { ior = 1.52, roughness = 0.15 }
}
```

---

### `thin_dielectric`
Single-interface glass sheet. No slab thickness, no IOR stack tracking.

**Samplers**

| Slot | Meaning |
|------|---------|
| `transmission` | Transmitted color |
| `roughness` | Roughness texture override (optional) |
| `normal` | Tangent-space normal map (optional) |

**Scalars**

| Parameter | Default | Range | Notes |
|-----------|---------|-------|-------|
| `ior` | `1.5` | > 1.0 | |
| `roughness` | `0.02` | 0.0–1.0 | |
| `transparency` | `1.0` | 0–1 | |

---

### `thin_translucent`
Thin sheet with both reflective and transmissive lobes on the same surface. Thickness affects lobe sharpness, not physical distance.

**Samplers**

| Slot | Meaning |
|------|---------|
| `base_color` | Reflected surface color |
| `translucency_color` | Transmitted light color |
| `normal` | Tangent-space normal map (optional) |

**Scalars**

| Parameter | Default | Range | Notes |
|-----------|---------|-------|-------|
| `translucency` | `0.5` | 0–1 | Blend weight: 0 = fully reflective, 1 = fully transmissive |
| `thickness` | `0` | ≥ 0 | Controls lobe sharpness exponent, not physical path length |

---

### `principled`
Full PBR material: Lambertian diffuse + GGX specular + optional clearcoat. The recommended material for production renders.

**Samplers**

| Slot | Meaning |
|------|---------|
| `base_color` | Primary surface color (falls back to `diffuse` if absent) |
| `roughness` | Roughness texture override (luminance channel) |
| `metallic` | Metallic texture override (luminance channel) |
| `normal` | Tangent-space normal map (optional) |

**Scalars**

| Parameter | Default | Range | Notes |
|-----------|---------|-------|-------|
| `roughness` | `0.5` | 0.02–1.0 | GGX alpha; lower = glossier |
| `metallic` | `0` | 0–1 | 0 = dielectric, 1 = conductor |
| `ior` | `1.5` | > 1.0 | Dielectric IOR for Fresnel |
| `anisotropy` | `0` | 0–1 | Anisotropic roughness ratio |
| `anisotropy_rotation` | `0` | degrees | Anisotropy axis rotation |
| `clearcoat` | `0` | 0–1 | Additional coat layer weight |
| `clearcoat_roughness` | `0.08` | 0.02–0.6 | Coat layer roughness |

```
type = principled
properties = {
    samplers = {
        base_color = { type = color, value = col3(0.8, 0.8, 0.8) }
        normal     = { type = voronoi_normal, cells = 128, max_deviation = 10 }
    }
    scalars = {
        roughness = 0.3
        metallic  = 0.0
        ior       = 1.45
    }
}
```

---

### `subsurface`
Single-scattering subsurface approximation with Beer-Lambert exponential decay.

**Samplers**

| Slot | Meaning |
|------|---------|
| `base_color` | Surface albedo (falls back to `diffuse`) |
| `subsurface_color` | Transmitted light color (falls back to `base_color`) |
| `subsurface_radius` | Per-channel exponential decay radius (col3) |
| `normal` | Tangent-space normal map (optional) |

**Scalars**

| Parameter | Default | Range | Notes |
|-----------|---------|-------|-------|
| `subsurface` | `0` | 0–1 | 0 = fully reflective, 1 = fully transmissive |
| `thickness` | `0` | ≥ 0 | Path length scale for Beer-Lambert decay |

Attenuation per channel: `exp(-thickness / subsurface_radius.channel)`. A `subsurface_radius` of `col3(0,0,0)` disables SSS.

---

### `sheen`
Retroreflective cloth-like material. Models the grazing-angle bright rim visible on velvet and fabric.

**Samplers**

| Slot | Meaning |
|------|---------|
| `base_color` | Base fabric color (falls back to `diffuse`) |
| `sheen_color` | Sheen highlight color (falls back to `base_color`) |
| `normal` | Tangent-space normal map (optional) |

**Scalars**

| Parameter | Default | Range | Notes |
|-----------|---------|-------|-------|
| `sheen` | `0` | 0–1 | 0 = diffuse only, 1 = full retroreflection |

---

### `boundary`
Transparent interface with IOR tracking. Passes rays through without reflection or scattering. Used as the material on medium boundary geometry (`src/xtcore/material/boundary.h`).

**Samplers** — none

**Scalars** — none

```
type = boundary
```
