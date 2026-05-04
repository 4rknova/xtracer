# Camera Reference

All camera types supported by xtracer's scene format. Source: `src/xtcore/parseutil.cc`, `src/xtcore/camera/`, `src/xtcore/proto.h`.

Cameras are defined inside the top-level `camera` block. A scene can contain multiple named cameras; the renderer uses the one specified by `default_camera` or the one passed at render time.

```
camera = {
    <name> = {
        type = <type>
        ...
    }
}
```

---

## `thin-lens`

Standard perspective camera with optional depth of field. The default choice for most renders.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `position` | vec3 | — | Camera position in world space |
| `target` | vec3 | — | Look-at point |
| `up` | vec3 | `vec3(0,1,0)` | Up direction |
| `fov` | float | `45` | Vertical field of view in degrees |
| `flength` | float | `0` | Focal length; DOF requires both `flength > 0` and `aperture > 0` |
| `aperture` | float | `0` | Aperture diameter; `0` disables DOF |
| `aperture_blades` | int | `0` | `0` = circular bokeh; `≥ 3` = N-sided polygon bokeh |
| `aperture_rotation` | float | `0` | Polygon aperture rotation in degrees |

```
hero = {
    type     = thin-lens
    fov      = 38
    position = vec3(0, 1.65, -8.0)
    target   = vec3(0, 1.15,  5.5)
    up       = vec3(0, 1, 0)
    flength  = 13.5
    aperture = 0.65
    aperture_blades   = 6
    aperture_rotation = 12
}
```

---

## `tilt-shift`

Perspective camera with a tilted focal plane and optional lens shift. Extends `thin-lens` with two extra parameters.

All `thin-lens` parameters apply, plus:

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `tilt` | float | `0` | Focal-plane tilt angle in degrees (Scheimpflug); positive tilts upward |
| `shift_x` | float | `0` | Horizontal lens shift in normalised sensor units (`1.0` = full frame width) |
| `shift_y` | float | `0` | Vertical lens shift in normalised sensor units |

```
arch = {
    type     = tilt-shift
    fov      = 58
    position = vec3(-28.0, 12.0, 10.0)
    target   = vec3(0, 0, 4.0)
    up       = vec3(0, 1, 0)
    flength  = 22.0
    aperture = 1.8
    tilt     = 12.0
    shift_x  = 0.0
    shift_y  = 0.0
}
```

---

## `erp`

Full 360° × 180° equirectangular panorama. Standard format for panoramic photos and 360° viewers.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `position` | vec3 | — | Camera position |
| `orientation` | vec3 | `vec3(0,0,0)` | Euler rotation in degrees (pitch, yaw, roll) |

No DOF support. Output aspect ratio should be ≥ 2:1 for a proper equirectangular map.

```
panorama = {
    type        = erp
    position    = vec3(0.0, 1.7, 2.8)
    orientation = vec3(0, 0, 0)
}
```

---

## `ods`

Omni-Directional Stereo — 360° stereoscopic panorama with left/right eye separation. Renders the top half of the output as the left eye and the bottom half as the right eye.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `position` | vec3 | — | Stereo baseline center in world space |
| `orientation` | vec3 | `vec3(0,0,0)` | Euler rotation in degrees |
| `ipd` | float | `0.064` | Inter-pupillary distance in scene units (standard human: 6.4 cm) |

Output height must be even (top = left eye, bottom = right eye). No DOF support.

```
vr = {
    type        = ods
    ipd         = 0.064
    position    = vec3(0.0, 1.7, 2.8)
    orientation = vec3(0, 0, 0)
}
```

---

## `cubemap`

Renders six cube faces as a vertical strip: +X, −X, +Y, −Y, +Z, −Z. Each face is `width × (height/6)`.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `position` | vec3 | — | Camera position |

Output height must be divisible by 6. No DOF support.

```
cubecam = {
    type     = cubemap
    position = vec3(0.0, 1.7, 2.8)
}
```

---

---

## `orthographic`

Parallel-projection camera — all rays travel in the same direction with no perspective foreshortening. Useful for technical/architectural renders, reference sheets, and any scene where scale consistency across depth matters.

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `position` | vec3 | — | Camera position in world space |
| `target` | vec3 | — | Look-at point |
| `up` | vec3 | `vec3(0,1,0)` | Up direction |
| `ortho_scale` | float | `1.0` | World-space width of the view volume; height is derived from the output aspect ratio |

No DOF support.

```
top = {
    type        = orthographic
    position    = vec3(0, 10, 0)
    target      = vec3(0, 0, 0)
    up          = vec3(0, 0, 1)
    ortho_scale = 8.0
}
```

---

## Summary

| Type | DOF | Notes |
|------|-----|-------|
| `thin-lens` | Yes | Standard perspective; polygon bokeh via `aperture_blades` |
| `tilt-shift` | Yes | Scheimpflug tilt + off-axis lens shift |
| `erp` | No | 360° mono panorama; aspect ratio ≥ 2:1 recommended |
| `ods` | No | 360° stereo panorama; output height must be even |
| `cubemap` | No | Six-face vertical strip; output height must be divisible by 6 |
| `orthographic` | No | Parallel projection; `ortho_scale` controls world-space view width |
