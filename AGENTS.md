# AGENTS.md

This file is the orientation and operating guide for coding agents working in this repository.

## Project Snapshot

- Name: `xtracer`
- Language: C/C++ (CMake)
- Branch context: this workspace is currently on `develop`.
- Core purpose: experimental rendering framework with a shared rendering core and multiple frontends (CLI, Web, WASM).

## Repository Map

- Core renderer and scene system: `src/xtcore/`
- CLI frontend: `src/frontend/cli/`
- Web frontend backend: `src/frontend/web-server/`
- WASM frontend runtime: `src/frontend/wasm/`
- Frontend shared helpers: `src/frontend/common/`
- Web static assets: `src/frontend/web-client/`
- Supporting libraries: `lib/`
- Third-party dependencies: `ext/`
- Scene examples: `scene/`
- Build definition: `CMakeLists.txt`

## Build Notes

- Historically documented flow in README:
  - `./configure`
  - `make`
- Practical modern flow is CMake-driven.
- Web build is optional (see `XTRACER_ENABLE_WEB` usage in `CMakeLists.txt`).
- WASM build is optional (see `XTRACER_ENABLE_WASM` usage in `CMakeLists.txt`).
- In this workspace, in-source CMake artifacts exist (`CMakeCache.txt`, `CMakeFiles/`, etc.). Prefer out-of-tree builds for new runs.

## Known Current State (Develop)

- Integrator implementations exist under `src/xtcore/integrator/`.
- Integrator metadata exposed by web/WASM is defined in `src/frontend/common/render_service.*`.
- CLI integrator selection code in `src/frontend/cli/xtracer.cc` is currently commented out; treat CLI rendering path as needing repair before relying on it.
- Web frontend is implemented as `xtracer_web`:
  - HTTP + WebSocket server via `ext/crow/crow_all.h` (crow framework) + standalone Asio (`ext/asio/`)
  - Async render jobs managed in `src/frontend/web-server/job_manager.*`
  - Shared render pipeline in `src/frontend/common/render_service.*`
  - Backend log stream in `src/frontend/web-server/backend_log.*`
  - Request logger middleware in `src/frontend/web-server/request_logger_middleware.h`
  - WebSocket pub/sub hub in `src/frontend/web-server/ws_hub.h`
  - Static SPA in `src/frontend/web-client/` with tabs: `Render`, `Editor`, `Settings`, `Logs`, `About`

### Web API Surface (Current)

Full REST and WebSocket API reference: `docs/API.md`.

**Key implementation notes:**
- Crow's void-return route handlers **must** call `res.end()` explicitly — crow does not call it automatically for void handlers (only for return-value handlers). Missing `res.end()` produces "Empty reply from server".
- Per-tile push sends binary only (no redundant text snapshot per tile). The binary header carries `tiles_done`/`tiles_total`/`elapsed_ms` so the client can update progress bars with server-authoritative time without a separate text message.
- XTDR packet builder: `src/frontend/web-server/job_manager.cc`; client parser: `src/frontend/web-client/app/preview.js` (`parseImageDeltaPacket`); push hub: `src/frontend/web-server/ws_hub.h`.

## Branch Relationship Reminder

See: `docs/BRANCH_RELATIONSHIP.md`

Short version:

- `master` and `develop` diverged years ago.
- `develop` contains newer 2024 rendering/build cleanup commits (remotery removal and optional GUI build).
- `master` contains several repo housekeeping commits not present on `develop`.

## Task-Specific References

- Architecture notes: `docs/ARCHITECTURE_NOTES.md`
- Renderer/integrator inventory: `docs/INTEGRATORS.md`
- Geometry type reference: `docs/GEOMETRY.md`
- Web API and WebSocket reference: `docs/API.md`
- CLI switch reference: `docs/CLI.md`
- Web UI/feature behavior: `src/frontend/web-client/index.html`, `src/frontend/web-client/app.js`, `src/frontend/web-client/styles.css`
- Web widget library: `src/frontend/web-client/app/widgets/`
- Widget showcase (live reference): `src/frontend/web-client/showcase.js`

## Scene Format Reference (Parser-Backed)

Source: `src/xtcore/parseutil.cc`, `src/xtcore/proto.h`. See also `scene/` for real examples and `.claude/commands/create-scene.md` for the interactive creation skill.

### Value Types

| Type | Syntax | Example |
|------|--------|---------|
| bool | `true` / `false` / `1` / `0` | `flip_normals = true` |
| int | decimal | `resolution = 64` |
| float | decimal or scientific | `radius = 1.5` |
| string | quoted or bare | `source = "path/file.obj"` |
| col3 | `col3(r,g,b)` | `col3(1,0.5,0)` — values 0–1 typical; higher for HDR/emissive |
| vec3 | `vec3(x,y,z)` | `vec3(0,1,0)` |
| tex2 | `tex2(u,v)` | `tex2(0.5,0.5)` |

Coordinate system: **right-handed, Y-up**. Modifier order when applied to meshes: rotation → scale → translation.

### Top-Level Structure

```
title       = "My Scene"
description = "Optional"
version     = "1.0"

environment = { type = ..., config = { ... } }
camera      = { cam_name = { type = ..., ... } }
geometry    = { geom_name = { type = ..., ... } }
material    = { mat_name  = { type = ..., properties = { samplers = { ... }, scalars = { ... } } } }
object      = { obj_name  = { geometry = geom_name, material = mat_name } }
medium      = { med_name  = { ... } }   # optional, for volumetrics
variants    = { var_name  = { set = { ... }, remove = { ... } } }  # optional
```

### Environment Types

| Type | Required config fields |
|------|----------------------|
| `gradient` | `a` col3, `b` col3 |
| `color` | `value` col3 |
| `erp` | `source` string (path to .hdr) |
| `cubemap` | `posx/posy/posz/negx/negy/negz` strings |
| `rayleigh_sky` | `sun_direction` vec3, `sun_intensity` col3, `beta_rayleigh` col3, `ground_color` col3; optional: `density`, `horizon_falloff`, `sun_disk_radius`, `sun_disk_intensity`, `sun_glow_radius`, `sun_glow_intensity`, `sun_glow_falloff` |

`rayleigh_sky` produces the best outdoor lighting and pairs well with `principled` materials.

### Camera Types

| Type | Key fields |
|------|-----------|
| `thin-lens` | `position` vec3, `target` vec3, `up` vec3, `fov` float (degrees); DOF: `flength` float, `aperture` float, `aperture_blades` int, `aperture_rotation` float |
| `erp` | `position` vec3, `orientation` vec3 (Euler degrees) — full 360° panoramic |
| `ods` | `position` vec3, `ipd` float — omni-directional stereo for VR |
| `cubemap` | `position` vec3 |

`thin-lens` is the default choice. Set `aperture > 0` and `flength > 0` to enable depth-of-field. `aperture_blades >= 3` produces polygonal bokeh.

### Geometry Types

**Primitives:**

| Type | Fields |
|------|--------|
| `sphere` | `position` vec3, `radius` float |
| `plane` | `normal` vec3, `distance` float |
| `triangle` | `vecdata = { v0, v1, v2 }` as vec3 |
| `point` | `position` vec3 (near-zero sphere internally) |

**Mesh — external import:**
```
type = mesh, source = ext(relative/path/to/file.obj)
```

**Mesh — procedural generators** (`type = mesh, source = gen(<name>)`):

*Platonic solids:* `tetrahedron`, `hexahedron` / `cube`, `octahedron`, `dodecahedron`, `icosahedron`

*Smooth/derived:* `icosphere` (subdivided sphere), `geodesic_dome`, `plane` (grid)

*Cylinders/rings:* `cylinder`, `capped_cylinder`, `cone`, `truncated_cone`, `capsule`, `ring` / `torus` (params: `radius`, `height`, `thickness`, `height_resolution`), `rounded_ring` (param: `profile_resolution`)

*Organic/complex:*
- `rock` — `seed`, `radius`, `roughness` (0–2), `octaves` (1–8)
- `shell_spiral` — `turns`, `growth`, `tube_radius`, `resolution`
- `chain_link` — `count`, `major_radius`, `minor_radius`, `spacing`
- `hairball` — `seed`, `radius`, `resolution`, `fibers`
- `lathe` — `profile` group of `tex2` points (2D profile rotated around Y), `cap_ends`, `resolution`
- `draped_cloth_strip` — `resolution`, `dimensions` vec3, `folds`, `edge_lift`, `curl`, `taper`, `sway`
- `terrain` — `resolution`, `dimensions` vec3 (width/amplitude/depth)

*Mathematical:*
- `mobius_strip` — `radius`, `width`
- `klein_bottle`, `torus_knot`, `snowflake`
- `menger_sponge` — `resolution` (1–5, default 2)
- `sierpinski_tetrahedron` — `resolution` (1–8)
- `mandelbulb` — `resolution` (1–64), `power` (2–16), `bailout`
- `julia` — `julia_c` vec3, `power`, `bailout`, `resolution`
- `pyramid` — `base_size`, `height`
- `icosa_cage`
- `superellipsoid` — `e1`, `e2` (Lamé exponents, 0.1–4.0; 1=diamond, 2=sphere, <1=pillow, >2=cube-like)

*Mechanical/structural:*
- `gear` — `tooth_count` (3–128), `tooth_depth`, `inner_radius`, `outer_radius`, `height`
- `spring` — `coils`, `wire_radius`, `spring_radius`, `height`
- `disc` — `inner_radius`, `outer_radius` (set inner > 0 for annular ring)
- `star` — `points` (3–32), `inner_radius`, `outer_radius`, `height`
- `hemisphere` — smooth dome with flat bottom cap

*Organic/generative:*
- `crystal` — `count` (1–32), `radius` (cluster spread), `height`, `tip_height`, `seed`
- `tree` — `depth` (1–7), `branch_count` (1–6), `branch_angle` (radians), `trunk_height`, `trunk_radius`, `seed`
- `coral` — `depth` (1–7), `branch_count` (1–8), `branch_angle`, `height`, `branch_radius`, `seed`

All mesh generators accept:
```
resolution = 32          # tessellation quality
modifiers = {
    rotation    = vec3(x_deg, y_deg, z_deg)
    scale       = vec3(sx, sy, sz)
    translation = vec3(tx, ty, tz)
    flip_normals = true
}
```

**CSG (Constructive Solid Geometry):**
```
type = csg
op   = union | soft_union | intersection | difference
smoothness = 0.15        # only for soft_union (0.01–1.0)
left  = { <inline geometry definition> }
right = { <inline geometry definition> }
```
CSG leaf nodes must be `sphere`, `plane`, `point`, or fractal types.

UV scale for all geometry: `u_scale`, `v_scale` (floats).

### Material Types

| Type | Description | Sampler slots | Key scalars |
|------|-------------|---------------|-------------|
| `lambert` | Matte diffuse | `diffuse` | — |
| `phong` | Phong specular | `diffuse`, `specular` | `exponent` (32–256), `reflectance` (0–1) |
| `blinn_phong` | Blinn-Phong specular | `diffuse`, `specular` | `exponent`, `reflectance` |
| `emissive` | Self-emitting light | `emissive` | — (use high col3 values for brightness) |
| `dielectric` | Perfect glass | `diffuse` | `ior` (1.0–2.5), `transparency` (0–1), `reflectance` (0–1) |
| `rough_dielectric` | Frosted glass | `diffuse` | `ior`, `roughness` (0–1), `transparency` |
| `thin_dielectric` | Thin transparent sheet | `diffuse`, `transmission` | `ior`, `transparency`, `reflectance` |
| `principled` | GGX PBR — most versatile | `base_color`, `normal` (optional) | `metallic` (0–1), `roughness` (0–1), `ior`, `clearcoat` (0–1), `clearcoat_roughness` |
| `subsurface` | Skin/wax/marble SSS | `diffuse`, `scattering` | `scattering_distance`, `scattering_strength` (0–1) |
| `sheen` | Velvet/cloth | `base_color`, `sheen_color`, `normal` (opt) | `sheen` (0–1) |
| `thin_translucent` | Paper/fabric translucency | `diffuse` | `transmission` (0–1), `thickness` |

Use `principled` as the default starting point for any physically-based look. Use `emissive` for light sources (not `lambert` with bright colors — that won't illuminate the scene).

### Sampler Types

Used inside `material.properties.samplers.<slot> = { type = ..., ... }`:

| Type | Fields |
|------|--------|
| `color` | `value` col3 |
| `texture` | `source` string, `filtering` (`bilinear` default / `nearest`), `flip_x`, `flip_y` |
| `gradient` | `a` col3, `b` col3 |
| `erp` | `source` string |
| `cubemap` | `posx/posy/posz/negx/negy/negz` strings |
| `rayleigh_sky` | same fields as the environment type |
| `checker` | `a` col3, `b` col3, `scale_u`, `scale_v`, `offset_u`, `offset_v` |
| `graphpaper` | `base`, `minor`, `major` col3; `scale`, `minor_width`, `major_width`, `major_every` |
| `weave` | `base`, `warp`, `weft` col3; `scale`, `band_width` |
| `fbm_marble` | `a`, `b`, `vein` col3; `scale`, `vein_frequency`, `turbulence`, `octaves`, `lacunarity`, `gain`, `vein_strength`, `vein_sharpness` |
| `voronoi_normal` | `cells` int, `max_deviation` float (degrees, 0–89), `seed` int — use in `normal` slot |
| `scenery_heightfield` | `seed`, `scale`, `octaves`, `lacunarity`, `gain`, `ridge_strength`, `mountain_strength`, `valley_strength` |

### Volumetric Mediums

```
medium = {
    med_name = {
        type     = homogeneous | heterogeneous_noise
        sigma_a  = col3(...)   # absorption
        sigma_s  = col3(...)   # scattering
        emission = col3(...)   # optional self-emission
        g        = 0.0         # anisotropy -1 to 1 (0 = isotropic)
        # heterogeneous_noise only:
        density      = 1.0
        noise_scale  = 1.0
        noise_min    = 0.25
        noise_max    = 1.0
        octaves      = 4
        lacunarity   = 2.0
        gain         = 0.5
        seed         = 1337
    }
}
object = { fog_sphere = { geometry = sphere1, material = glass1, medium = med_name } }
```

### Variants

Allow alternate scene configurations activated at render time:
```
variants = {
    night = {
        set    = { environment = { type = color, config = { value = col3(0.01,0.01,0.03) } } }
        remove = { object = { sun_light = {} } }
    }
}
```

### Integrators (Render-Time, Not in Scene File)

Set via API/UI, not the `.ncf` file:

| Integrator | Best for | Notes |
|-----------|----------|-------|
| `pathtracer` | Ground-truth renders | Needs samples ≥ 64, bounces ≥ 8 |
| `pathtracer_mis` | Production quality | Full MIS; best variance reduction |
| `raytracer` | Fast preview | 1 sample, no GI |
| `ao` | Occlusion-only preview | Fast, no color |
| `photon_mapping` | Caustics | Two-pass; set emit_photons, gather_k, gather_radius |

### Scene Design Rules

**Lighting:**
- Every scene needs at least one `emissive` object or a bright `rayleigh_sky` / HDR environment — dark environments produce pure black renders.
- Use high emissive values: `col3(8,7,5)` for warm white, not `col3(1,1,1)`. The renderer has no automatic exposure.
- Place light sources where rays can reach them — behind opaque geometry they contribute nothing.
- Use multiple lights (key + fill + rim) to avoid unlit regions that accumulate noise.

**Materials:**
- `principled` is the go-to for any real-world surface. Tune `metallic` and `roughness` before reaching for specialised types.
- `emissive` is the only material that acts as a light source. Do not try to approximate lighting with bright `lambert` colors.
- Glass (`dielectric`, `rough_dielectric`) requires `bounces ≥ 8` to resolve transmission correctly.
- `subsurface` and `sheen` are slow — use only where the effect is prominent.

**Geometry:**
- Prefer procedural generators over external OBJ for reproducibility and normal quality.
- Increase `resolution` for smooth curved surfaces — default is often too low.
- A floor plane (`type = plane, normal = vec3(0,1,0), distance = -1`) is almost always needed to ground objects.
- CSG `soft_union` blends surfaces cleanly; keep `smoothness` in the 0.05–0.3 range.

**Camera:**
- `fov = 45–60` for neutral perspectives. Wider (75+) for environments, narrower (25–35) for portraits/macro.
- Enable DOF only intentionally: set both `flength` (focal distance in world units) and `aperture` (radius). Start at `aperture = 0.05` and increase.
- The `up` vector must not be parallel to the view direction — use `vec3(0,1,0)` unless looking straight up/down.

**Parser Quirks:**
- Last definition wins for duplicate names — no error is raised.
- Relative paths in `source` are relative to the scene file location.
- `point` geometry is a sphere internally; use small emissive spheres instead for predictable results.
- Unknown keys are silently ignored — typos in field names produce no warning.
- CSG recursion limit: 32 levels deep.

## Web Client UI Architecture

The web client uses a **widget-based UI architecture**. All dynamic DOM construction must go through the shared widget library. Do not reach for `document.createElement` or `innerHTML` when a widget already covers the pattern.

### Widget Library

All widgets are defined in `src/frontend/web-client/app/widgets/` and exposed on the global `window.XTracerWidgets` object. The library is always fully loaded before any app code runs — no availability checks are needed.

| Module | Exports |
|--------|---------|
| `dom.js` | `dom.el`, `dom.clear`, `dom.mount`, `dom.svgIcon` — foundational DOM helpers used by all other widgets |
| `button.js` | `createButton`, `createIconButton` |
| `pill.js` | `createPill` — selectable or pressable toggle chip |
| `tag.js` | `createTag` — semantic status badge (tones: neutral, success, warning, error, info) |
| `field.js` | `createField` (base, takes any `control`), `createNumberField`, `createSelectField`, `createCheckboxField` |
| `progress.js` | `createProgressBar` |
| `stat.js` | `createStatHint` (creates element), `renderStatHint` (upgrades existing element in place) |
| `key_hint.js` | `createKeyHint` |
| `toolbar.js` | `createToolbar` |
| `empty_state.js` | `createEmptyState` |
| `container.js` | `createStack`, `createSurface` |
| `card.js` | `createPanelCard`, `createCollapsibleCard`, `createAccordionCard` (alias of collapsible) |
| `panel_header.js` | `createPanelHeader`, `upgradePanelHeaders` |
| `dropdown.js` | `enhanceSelect` (single), `enhanceSelects` (batch — enhances all eligible selects under a root) |
| `menu.js` | `createMenu`, `createMenuItem`, `createMenuDivider`, `createMainTabs`, `renderMainTabs` |
| `create_field.js` | `createCreateField`, `renderCreateField` |
| `job_row.js` | `createJobRow` |
| `workspace_card.js` | `createWorkspaceCard` |
| `scene_card.js` | `createSceneCard`, `createCameraCard`, `createVariantCard` |
| `dependencies.js` | `createDependencyItem`, `renderDependencyList` |

The showcase page (`src/frontend/web-client/showcase.js`, served at `/showcase.html`) renders a live demo of every widget and must be kept in sync whenever widgets are added or changed.

### Rules For UI Work

**Always use a widget when one exists.**
Before writing any DOM construction code, check the widget table above. If a widget covers the pattern, use it. The common cases:

- Buttons → `createButton` / `createIconButton`
- Form labels wrapping any input → `createField({ label, control, help })`
- Typed inputs → `createNumberField` / `createSelectField` / `createCheckboxField`
- State chips/filters → `createPill` (selectable/pressable) or `createTag` (read-only badge)
- Collapsible sections → `createCollapsibleCard`
- Label + value display → `createStatHint` / `renderStatHint`
- "No data" messaging → `createEmptyState`
- Native `<select>` enhancement → `enhanceSelect`

**Never add defensive fallback branches.**
The pattern `typeof window.XTracerWidgets.createX === "function" ? ... : legacyDOMBlock` is an anti-pattern and must not be introduced. Widgets are guaranteed to be available. If you see an existing fallback, remove it.

**When raw DOM is unavoidable, use `dom.el`.**
If a pattern genuinely has no widget equivalent (complex stateful components, data visualisations, SVG charts), use `window.XTracerWidgets.dom.el()` instead of `document.createElement`. It handles `className`, `attrs`, `dataset`, `props`, `children`, and `text` in a single call and is consistent with the widget layer.

**When to create a new widget.**
A new widget is justified when the same structural pattern appears in three or more independent call sites. Below that threshold, inline `dom.el` composition is preferred. When adding a widget:
1. Create it in `src/frontend/web-client/app/widgets/<name>.js` following the existing IIFE pattern, exporting onto `window.XTracerWidgets`.
2. Add a demo card for it in the `buildWidgets()` function in `showcase.js`.
3. Update the widget table in this file.

### Known Structural Gap

`src/frontend/web-client/app/sidebar_cards.js` defines sidebar card bodies as raw HTML template strings processed by `htmlToFragment()`. Widget calls cannot reach elements inside these strings. Post-render upgrade functions (`renderStatHint`, `enhanceSelects`) compensate for the stat hints and dropdowns, but buttons, pills, and checkboxes inside the bodies are static HTML. Converting these bodies from HTML strings to `dom.el`-based builders is the remaining step to full widget coverage of the sidebar.

## Working Conventions For Agents

- When adding or modifying any geometry type — analytic primitive, analytic fractal, mesh generator, CSG, or external mesh — update `docs/GEOMETRY.md` in the same task. This includes parameter changes, new generators, renamed keywords, and resolution-range adjustments.
- When adding or modifying any material type — including new BRDFs, new sampler slots, renamed scalar parameters, new scalar parameters, or default value changes — update `docs/MATERIALS.md` in the same task. The file is the canonical reference for all types registered in `deserialize_material()` in `src/xtcore/parseutil.cc` and all slot/scalar names defined in `src/xtcore/matdefs.h`.
- When adding or modifying any sampler type — including new procedural patterns, noise samplers, normal map generators, environment samplers, or image-based samplers — update `docs/SAMPLERS.md` in the same task. This includes parameter changes, new types, renamed keywords, default value changes, and range adjustments. The file is the canonical reference for all types registered in `deserialize_sampler_node()` in `src/xtcore/parseutil.cc`.
- When adding or modifying a camera type — new type, renamed parameters, new parameters, or changed defaults — update `docs/CAMERAS.md` in the same task. The file is the canonical reference for all types registered in `src/xtcore/parseutil.cc` via the `XTPROTO_LTRL_CAM_*` constants.
- When adding, renaming, or removing a top-level NCF scene block, or changing how `variants` / `object` / `medium` are parsed, update `docs/SCENE_FORMAT.md` in the same task.
- When adding a new procedural geometry generator, add a corresponding variant to `scene/lab-procedural-geometry-showcase.scn`. The file covers all supported generators exhaustively — one variant per type, with a matching geometry block, a principled material, and an entry in the `variants` section. Follow the existing naming convention (`g_<name>` for geometry, `mat_<name>` for material).
- When adding or modifying a REST or WebSocket endpoint in `src/frontend/web-server/routes.cc` — new endpoint, changed parameters, changed response shape, new error codes — update `docs/API.md` in the same task. The file is the canonical API reference for external callers.
- When adding, removing, or changing CLI flags in `src/frontend/cli/argdefs.h` / `src/frontend/cli/argparse.cc`, or startup flags in `src/frontend/web-server/main.cc`, update `docs/CLI.md` in the same task.
- Prefer minimal, surgical changes.
- Do not revert unrelated working tree changes.
- Keep tile-based architecture unless intentionally redesigning it.
- When a code/config/API/feature change affects documented behavior, update `README.md` in the same task so it reflects the current repository state.
- When changing rendering flow, validate worker-thread behavior in web/WASM frontends.
- When changing web API responses, update both:
  - backend route handlers in `src/frontend/web-server/routes.cc`
  - frontend consumers in `src/frontend/web-client/app.js`
- If adding new integrators, update both:
  - `src/xtcore/integrator.h`
  - Integrator registry in `src/frontend/common/render_service.cc`

## Commit Message Format

- Subject line should be short and scoped (e.g., `build: ...`).
- Body should use action-verb bullets in present tense.
- Prefer wording like `adds`, `fixes`, `removes`, `updates`, `gates`, `routes`.
