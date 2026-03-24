<img align="center" src="https://raw.githubusercontent.com/4rknova/xtracer/develop/res/preview.jpg">

# XTRACER

Experimental rendering framework written in C/C++ with a shared core (`xtcore`) and multiple frontends (CLI, Web, WASM runtime).

[![CI](https://github.com/4rknova/xtracer/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/4rknova/xtracer/actions/workflows/ci.yml)

## Current Status (`develop`)

- Build system is CMake-driven (legacy `./configure && make` still exists, but CMake is the maintained path).
- Core renderer and scene parsing are active under `src/xtcore/`.
- HTTP web frontend is optional via `XTRACER_ENABLE_WEB`.
- Standalone WASM renderer runtime is optional via `XTRACER_ENABLE_WASM`.

## Quick Start (Install + Run)

Install dependencies (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libomp-dev zlib1g-dev libasound2-dev
```

Configure + build:

```bash
cmake -S . -B build/intermediate/build -DXTRACER_ENABLE_WEB=ON
cmake --build build/intermediate/build -j
```

Run web server:

```bash
./build/intermediate/build/xtracer_web --host 127.0.0.1 --port 8080 --scene-dir scene --web-root src/frontend/web-client --max-concurrent-renders 1 --render-reserve-threads 1 --verbose
```

Open: `http://127.0.0.1:8080`

Run CLI:

```bash
./build/intermediate/build/xtracer_cli scene/lab-camera-modes-showcase.scn -renderer pathtracer_mis -res 1280x720 -samples 4 -aa 2
```

With scene variant:

```bash
./build/intermediate/build/xtracer_cli scene/lab-camera-modes-showcase.scn -variant night -renderer pathtracer_mis -res 1280x720 -samples 4 -aa 2
```

## Repository Layout

| Area | Path | Purpose |
|---|---|---|
| Core renderer | `src/xtcore/` | Scene parsing, render context, integrators, tone mapping |
| CLI frontend | `src/frontend/cli/` | Command-line scene rendering |
| Web frontend backend | `src/frontend/web-server/` | HTTP API, job manager, log stream |
| Frontend shared code | `src/frontend/common/` | Shared render service + integrator metadata |
| Web static app | `src/frontend/web-client/` | SPA for Render / Editor / Settings / Logs / About, including runtime JSON config in `app/data/` |
| Scenes | `scene/` | Example scene files (`.scn`) |
| Supporting libs | `lib/` | Internal libraries (`nimg`, `nmesh`, `nmath`, etc.) |
| Third-party deps | `ext/` | Vendored external dependencies |

## Feature Matrix

### Frontends

| Capability | CLI (`xtracer_cli`) | Web (`xtracer_web`) | WASM (`xtracer_wasm`) |
|---|---:|---:|---:|
| Load `.scn` scenes | Yes | Yes | Yes |
| Select camera | Yes (`-cam`) | Yes | Yes |
| Select scene variant | Yes (`-variant`) | Yes (`variant` API param) | Via backend API |
| Integrator selection | Yes (`-renderer`) | Yes (`/api/integrators`) | Yes (through web app adapter) |
| Progressive updates | Terminal progress | Job progress + preview API | Progressive snapshots |
| Image export | PNG | PNG/JPG/BMP/TGA/HDR/EXR + Raygraph PLY (`/api/jobs/{id}/export`) | PNG snapshots (adapter flow) |
| HTTP API | No | Yes | No |

### Integrators

| Integrator ID | Type | Exposed In `/api/integrators` |
|---|---|---:|
| `raytracer` | Whitted-style | Yes |
| `pathtracer` | Brute-force path tracing | Yes |
| `pathtracer_mis` | MIS diffuse path tracing | Yes |
| `pathtracer_mis_full` | MIS full path tracing | Yes |
| `photon_mapping` | Photon mapping | Yes |
| `ao` | Ambient occlusion | Yes |
| `debug_views` | Multi-mode debug integrator | Yes |
| `depth` | Debug depth alias | No (alias accepted in `/api/render`) |
| `stencil` | Debug stencil alias | No (alias accepted in `/api/render`) |
| `normal` | Debug normal alias | No (alias accepted in `/api/render`) |
| `uv` | Debug UV alias | No (alias accepted in `/api/render`) |
| `emission` | Debug emission alias | No (alias accepted in `/api/render`) |

### Scene Schema Support (`.scn`)

#### Environment Types

| Type | Notes |
|---|---|
| `gradient` | Uses `config.a`, `config.b` colors |
| `color` | Uses `config.value` |
| `cubemap` | Uses face sources: `posx/posy/posz/negx/negy/negz` |
| `erp` | Uses panoramic source texture |

#### Camera Types

| Type | Notes |
|---|---|
| `thin-lens` | Position/target/up/fov/flength/aperture (+ optional aperture shape controls) |
| `ods` | Omni-directional stereo camera |
| `erp` | Equirectangular camera |
| `cubemap` | Cubemap camera |

`thin-lens` optional aperture-shape fields:
- `aperture_blades` (integer): `0` or `<3` keeps circular aperture; `>=3` uses an N-gon aperture for polygonal bokeh.
- `aperture_rotation` (float, degrees): rotates the polygonal aperture when `aperture_blades >= 3`.

#### Geometry Types

| Type | Notes |
|---|---|
| `plane` | Analytic plane |
| `sphere` | Analytic sphere |
| `point` | Parsed as epsilon-radius sphere |
| `triangle` | Triangle via `vecdata.v0/v1/v2` |
| `menger_sponge` | Implicit fractal surface (`position`, `radius`, `resolution`) |
| `sierpinski_tetrahedron` | Implicit fractal surface (`position`, `radius`, `resolution`) |
| `mandelbulb` | Implicit fractal surface (`position`, `radius`, `resolution`, `power`, `bailout`) |
| `julia` | Implicit fractal surface (`position`, `radius`, `resolution`, `power`, `bailout`, `julia_c`) |
| `csg` | Constructive solid geometry tree (`op`, `left`, `right`) |
| `mesh` | External OBJ or procedural generator |

#### CSG (`geometry.type = csg`)

CSG geometry uses binary tree nodes:
- `op`: `union`, `soft_union` (`smooth_union` alias), `intersection`, or `difference`
- `left`: nested node
- `right`: nested node
- `smoothness` (optional): soft union blend width (`> 0`, default `0.15`), used only with `soft_union`

`left`/`right` can be:
- another CSG node (contains `op`, `left`, `right`), or
- a supported leaf geometry definition (v1): `sphere`, `point`, `plane`, `menger_sponge`, `sierpinski_tetrahedron`, `mandelbulb`, `julia`.

Example:

```scn
geometry = {
  shape = {
    type = csg
    op = difference
    left = { type = sphere, position = vec3(0,0,0), radius = 1.0 }
    right = { type = sphere, position = vec3(0.4,0,0), radius = 0.6 }
  }
}
```

#### Procedural Mesh Generators (`geometry.type = mesh`, `source = gen(...)`)

| Generator | Generator | Generator | Generator |
|---|---|---|---|
| `plane` | `icosahedron` | `tetrahedron` | `cube` |
| `hexahedron` | `octahedron` | `dodecahedron` | `capsule` |
| `cylinder` | `capped_cylinder` | `cone` | `truncated_cone` |
| `ring` | `torus_knot` | `icosphere` | `geodesic_dome` |
| `icosa_cage` | `menger_sponge` | `sierpinski_tetrahedron` | `mobius_strip` |
| `menger_sponge_implicit` | `sierpinski_tetrahedron_implicit` |  |  |
| `klein_bottle` | `hairball` | `shell_spiral` | `rock` |
| `chain_link` | `lathe` | `snowflake` | `pyramid` |

Note: `menger_sponge` and `sierpinski_tetrahedron` can be used either as:
- `geometry.type` values (native implicit xtcore surfaces), or
- `gen(...)` mesh generators (triangulated geometry).

Sizing note:
- `gen(menger_sponge)` and `gen(menger_sponge_implicit)` are normalized to unit canonical bounds (`[-0.5, 0.5]` per axis) before scene modifiers.

`gen(pyramid)` supports:
- `base_size` (float, `> 0`, default `1.0`)
- `height` (float, `> 0`, default `1.0`)

#### Seeded Random Generators (`random`)

Scenes may define a top-level `random` group to synthesize deterministic, repeatable scene content.
Each random entry must provide an explicit `seed`.

Supported generator types:

- `sphere_grid`: creates many sphere objects with seeded random placement/material assignment.

`sphere_grid` properties:
- `seed` (integer, required)
- `prefix` (string, optional): name prefix for generated geometry/material/object ids
- `x_min`, `x_max`, `z_min`, `z_max` (integers): loop bounds (`[min, max)`)
- `y` (float): sphere center Y
- `radius` (float, `> 0`)
- `jitter` (float): per-cell random XY jitter magnitude
- `avoid_center` (`vec3`) and `avoid_radius` (float): exclusion sphere
- `lambert_ratio` (float `[0,1]`): diffuse probability threshold
- `metal_ratio` (float `[lambert_ratio,1]`): metal threshold (`> metal_ratio` becomes glass)
- `metal_fuzz_min`, `metal_fuzz_max` (float `[0,1]`): seeded fuzz range for generated metals
- `ior` (float `>= 1`): generated glass index of refraction
- `glass_reflectance` (float `[0,1]`): generated glass reflection probability

#### Material Types

| Type |
|---|
| `lambert` |
| `phong` |
| `blinn_phong` |
| `emissive` |
| `dielectric` |
| `boundary` |

`boundary` is an interface-only material. It does not add surface shading and forwards rays through the hit point unchanged (useful for object-local medium boundaries).

#### Object-Local Participating Media (v1)

Participating media are defined in a top-level `medium` group and referenced by object.

If an object has no `medium` reference, the renderer uses vacuum behavior.

Current v1 constraints:
- medium types: `homogeneous`, `heterogeneous_noise`
- intended boundary material: `boundary`
- currently applied in path tracers (`pathtracer`, `pathtracer_mis`, `pathtracer_mis_full`)
- inline `object.medium = { ... }` blocks are not supported

Example:

```text
medium = {
  fog_main = {
    type = homogeneous
    sigma_a = col3(0.06,0.04,0.03)
    sigma_s = col3(0.24,0.20,0.14)
    g = 0.15
    emission = col3(0.00,0.00,0.00)
  }
}

object = {
  fog_shell = {
    geometry = fog_volume
    material = boundary_shell
    medium = fog_main
  }
}
```

`heterogeneous_noise` supports these extra parameters (all optional):
- `density` (default `1.0`)
- `noise_scale` (default `1.0`)
- `noise_min` (default `0.25`)
- `noise_max` (default `1.0`)
- `octaves` (default `4`)
- `lacunarity` (default `2.0`)
- `gain` (default `0.5`)
- `seed` (default `1337`)

#### Sampler Types

| Type | Notes |
|---|---|
| `color` | Solid color |
| `texture` | 2D texture |
| `cubemap` | Cubemap texture |
| `erp` | Equirectangular texture |
| `gradient` | Gradient sampler |
| `graphpaper` | Procedural graph paper |
| `checker` | Procedural checker |
| `weave` | Procedural weave |
| `fbm_marble` | Procedural marble |
| `voronoi_normal` | Procedural Voronoi tangent-space normal map |

Lambert material supports an optional sampler named `normal` for tangent-space normal mapping.
Supported normal sampler types:
- `texture` (normal-map texture)
- `voronoi_normal` (procedural normal generator)

`voronoi_normal` parameters:
- `cells` (integer, `>= 1`): Number of Voronoi cells.
- `max_deviation` (float, degrees, clamped to `[0, 89]`): Maximum angular deviation from tangent-space +Z.
- `seed` (integer): Deterministic random seed.

#### Mesh Modifiers

| Modifier |
|---|
| `rotation` |
| `scale` |
| `translation` |
| `flip_normals` |
| `extrude` |

#### Scene Variants

Scenes can define optional overlays under `variants`:

```scn
variants = {
  hide_base = true

  base = {
    name        = Baseline
    description = Uses the unmodified scene definition.
  }

  night = {
    name        = Night Lighting
    description = Dims emissive and swaps to night camera.
    remove = {
      object = {
        light_fill = {}
      }
    }
    set = {
      default_camera = cam_night
      material = {
        lamp = {
          properties = {
            samplers = {
              emissive = { type = color, value = col3(0.03,0.03,0.06) }
            }
          }
        }
      }
    }
  }
}
```

Rules:
- `remove`: presence-based nested groups (`name = {}`) remove matching properties/groups in the base scene.
- `set`: deep-merge overlay; properties overwrite and groups merge recursively.
- `hide_base` (optional bool): when `true`, hides the `(base)` option in the web variant picker.
- `name` / `description`: optional metadata for frontend variant pickers.
- `variants.base`: optional metadata for the base (no variant) selection.
- Apply order: `remove` -> `set` -> CLI `-mod` overrides.
- Requesting a missing variant returns a scene-load error.

## Web App + API

### Web App Tabs

| Tab | Key Capabilities |
|---|---|
| Scene | File-manager-style scene browser plus fixed-size camera/variant cards (with variant name + description metadata), active selection panels, single-click selection, double-click activation, and scene file right-click actions (`Set Active`, `Delete`) |
| Render | Scene/camera/integrator selection, render settings, preview, export, in-flight abort support (render action toggles `Render`/`Abort`), and render modes (`Normal`, `Progressive`, `Interactive`) with interactive camera controls/ramping available in Interactive mode |
| Editor | Switchable `3D View` / `Graph` / `Text Editor` modes, scene source editor, create geometry, mesh translate/rotate/scale controls, click-select + Ctrl-drag move, `F` focus shortcut, visual viewport integration, scene save |
| Settings | Theme mode + light/dark palette selection, frontend behavior toggles, render polling controls, and first-time tutorial reset/start controls |
| Logs | Backend log stream with wait-based incremental updates and level filters |
| About | Build/backend metadata, project license text, and third-party license notices |

First-time use tutorial (FTUE):
- On first launch, the web app opens a guided tutorial for scene selection, rendering, and scene editing flow.
- In `Settings`, enable `Show tutorial on next launch` to reset onboarding state for the next app start.
- In `Settings`, use `Start Tutorial Now` to reopen the tutorial immediately.
- Tutorial steps are config-driven via `src/frontend/web-client/app/data/ftue_steps.json` (`steps[]` entries support `title`, `body`, `target_selector`, `placement`, `tab`, `editor_view`, `open_cards`, and optional `focus_selector`).

Interactive preview controls (Render tab, with `Render Mode = Interactive`):
- `Left drag`: look around
- `Middle/Right drag` or `Shift + Left drag`: pan
- `Wheel` or touch pinch: zoom
- `W/A/S/D`: move forward/left/back/right
- `Q/E`: move down/up
- `Shift`: speed boost
- `Interactive Speed` slider: scales fly movement speed
- `Save Interactive Camera`: appends a new `camera` entry to the active scene from the current interactive pose and saves it
- Preview HUD: shows mode, speed, and current interactive quality stage
- Moving quality auto-adapts toward a low-latency frame-time target before settle refinement
- During active movement, interactive mode temporarily uses a low-cost navigation profile (raytracer + lightweight settings), then restores settle refinement

### Web API Endpoints

| Method | Endpoint | Purpose |
|---|---|---|
| GET | `/api/health` | Health probe |
| GET | `/api/about` | Backend/app metadata, runtime capacity stats, and license/third-party notice fields |
| GET | `/api/scenes` | List available scenes |
| GET | `/api/scenes/{scene}/cameras` | List cameras in scene (optional `variant=<name>`; includes `camera_entries` with `name` + `type`; returns `202` while async scene load is in progress) |
| GET | `/api/scenes/{scene}/source` | Fetch scene source |
| GET | `/api/scenes/{scene}/geometry` | Extract mesh geometry payload (optional `variant=<name>`; returns `202` while async scene load is in progress) |
| GET | `/api/scenes/{scene}/runtime_graph` | Fetch runtime-resolved scene graph (objects/surfaces/materials/media/cameras; object entries may include `medium`) (optional `variant=<name>`; returns `202` while async scene load is in progress) |
| GET | `/api/scenes/{scene}/camera_resolve` | Resolve active camera metadata (optional `variant=<name>`; returns `202` while async scene load is in progress) |
| GET | `/api/scenes/load_jobs/{id}` | Poll async scene load job status |
| GET | `/api/scenes/{scene}/asset?path=...` | Fetch referenced scene asset |
| GET | `/api/scenes/template/empty` | Empty scene template |
| POST | `/api/scenes/save` | Save scene source |
| POST | `/api/scenes/delete` | Delete a scene file by name |
| GET | `/api/workspaces?client_id={id}` | List workspaces + active workspace + workspace-scoped settings snapshot |
| POST | `/api/workspaces` | Create workspace for client context |
| POST | `/api/workspaces/active` | Switch active workspace for client |
| POST | `/api/workspaces/delete` | Delete workspace |
| POST | `/api/workspaces/scene_draft` | Save workspace-local scene draft |
| POST | `/api/workspaces/settings` | Save workspace UI settings (quality/frame/integrator/tone mapping/preview/post-filters) |
| GET | `/api/integrators` | List backend integrators + controls |
| GET | `/api/resolutions` | Resolution presets |
| POST | `/api/render` | Create render job (optional `variant=<name>`, optional `render_mode={normal,progressive,interactive}`, and optional interactive camera override: `cam_px/cam_py/cam_pz`, `cam_tx/cam_ty/cam_tz`, `cam_upx/cam_upy/cam_upz`, `cam_hfov`) |
| GET | `/api/jobs/active` | Server-authoritative list of active jobs (`jobs[]`, running first then queued) |
| POST | `/api/jobs/abort/{id}` | Abort explicit job id |
| GET | `/api/jobs/{id}` | Job status snapshot |
| GET | `/api/jobs/{id}/image` | PNG preview/final image |
| GET | `/api/jobs/{id}/image_delta?since={n}&limit={m}` | Incremental preview tiles since tile index `n` (binary packet, tone mapping params supported) |
| GET | `/api/jobs/{id}/export?format={png,jpg,bmp,tga,exr,hdr,ply}` | Download final export (PLY is raygraph) |
| GET | `/api/jobs/{id}/photons` | Photon debug points |
| GET | `/api/logs?since={id}` | Incremental backend logs |
| GET | `/api/logs/wait?since={id}&timeout_ms={n}` | Wait for new backend logs (long-poll) |

## Build

### Prerequisites (Debian/Ubuntu)

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libomp-dev zlib1g-dev
```

RtMidi/ALSA support (`XTRACER_ENABLE_RTMIDI=ON`):

```bash
sudo apt install -y libasound2-dev
```

WASM toolchain (`XTRACER_ENABLE_WASM=ON`):

```bash
sudo apt install -y emscripten
```

### Recommended Native Build (Out-of-Tree)

```bash
cmake -S . -B build/intermediate/build -DXTRACER_ENABLE_WEB=ON
cmake --build build/intermediate/build -j
```

> Note: native binaries are configured to output under the chosen CMake build directory (for example `build/intermediate/build/`).

### CMake Options

| Option | Default | Description |
|---|---:|---|
| `XTRACER_ENABLE_RTMIDI` | `ON` | Build RtMidi/ALSA support |
| `XTRACER_ENABLE_WEB` | `ON` | Build HTTP web frontend |
| `XTRACER_ENABLE_WASM` | `OFF` | Build standalone WASM runtime |
| `XTRACER_ENABLE_WASM_DIST` | `OFF` | Build/package standalone WASM dist during native build |
| `XTRACER_ENABLE_VIZ` | `OFF` | Build OpenGL sampling visualization tool (`xtracer_viz_sampling`) |
| `XTRACER_ENABLE_NMATH_SIMD` | `ON` | Enable x86 SSE2 SIMD fast-paths for `nmath` double-precision vector and matrix operations |
| `XTRACER_ENABLE_NMATH_SIMD_AVX` | `ON` | Use AVX path for `nmath` SIMD (`XTRACER_ENABLE_NMATH_SIMD` must be `ON`) |

Notes:
- `XTRACER_ENABLE_NMATH_SIMD` currently targets native x86/x86_64 builds and is ignored for Emscripten.
- SIMD paths are used only when `nmath` is built in double precision (default configuration); scalar fallback remains available.
- `XTRACER_ENABLE_NMATH_SIMD_AVX` enables AVX codegen and runtime AVX instructions for supported hosts.

## Run

### CLI

```bash
./build/intermediate/build/xtracer_cli scene/lab-camera-modes-showcase.scn -renderer pathtracer_mis -res 1280x720 -samples 4 -aa 2
```

### Web Server

```bash
./build/intermediate/build/xtracer_web --host 127.0.0.1 --port 8080 --scene-dir scene --web-root src/frontend/web-client --max-concurrent-renders 1 --render-reserve-threads 1 --verbose
```

Open: `http://127.0.0.1:8080`

`--max-concurrent-renders` controls how many render jobs execute simultaneously (default: `1`).
`--render-reserve-threads` controls how many threads auto-render mode keeps free for server responsiveness (default: `1`).
When `/api/render` uses `threads=0`, backend auto mode resolves to `max(1, runtime_threads - reserve_threads)` (single-core hosts still render with `1` thread).
Startup prints an ASCII banner with runtime info (host/port, paths, concurrency, and detected core/thread limits).

### Docker Deployment

Build and start on a host machine:

```bash
cp .env.example .env
docker compose up --build -d
```

Check container status:

```bash
docker compose ps
docker compose logs -f xtracer-web
```

Open: `http://127.0.0.1:${XTRACER_PORT:-8080}`

Notes:

- Scene files are mounted from `./scene` into the container at `/app/scene`.
- The image builds web frontend support with `XTRACER_ENABLE_RTMIDI=OFF` for simpler runtime dependencies.
- Set `XTRACER_OMP_NUM_THREADS` in `.env` to control OpenMP worker count.

Stop:

```bash
docker compose down
```

### Math Microbenchmark

```bash
./build/intermediate/build/bench_nmath
```

### Sampling Visualization Tool

Build with visualization enabled:

```bash
cmake -S . -B build/intermediate/build-viz -DXTRACER_ENABLE_VIZ=ON
cmake --build build/intermediate/build-viz -j --target xtracer_viz_sampling
```

Run:

```bash
./build/intermediate/build-viz/xtracer_viz_sampling
```

### WASM Runtime Build

```bash
emcmake cmake -S . -B build/intermediate/build-wasm \
  -DXTRACER_ENABLE_WEB=OFF \
  -DXTRACER_ENABLE_RTMIDI=OFF \
  -DXTRACER_ENABLE_WASM=ON
cmake --build build/intermediate/build-wasm -j --target xtracer_wasm
```

Expected output:

- `src/frontend/web-client/xtracer_wasm.js`
- `src/frontend/web-client/xtracer_wasm.wasm`
- copied self-contained scenes from `scene/` into build output `<build-dir>/scenes/` (for example `build/intermediate/build-wasm/scenes/`)

Optional static packaging:

```bash
./util/package_wasm_standalone.sh
```

## Test Targets

| Test Name (CTest) | Binary |
|---|---|
| `colorspace::roundtrip` | `<build-dir>/test/test_nimg_colorspace` |
| `colorspace::vectors` | `<build-dir>/test/test_nimg_colorspace_vectors` |
| `xtcore::tile` | `<build-dir>/test/test_xtcore_tile` |
| `xtcore::context` | `<build-dir>/test/test_xtcore_context` |
| `xtcore::sphere` | `<build-dir>/test/test_xtcore_sphere` |
| `xtcore::triangle` | `<build-dir>/test/test_xtcore_triangle` |
| `xtcore::boundary_material` | `<build-dir>/test/test_xtcore_boundary_material` |
| `xtcore::white_furnace` | `<build-dir>/test/test_xtcore_white_furnace` |
| `xtcore::raytracer_emissive` | `<build-dir>/test/test_xtcore_raytracer_emissive` |
| `xtcore::object_medium_parse` | `<build-dir>/test/test_xtcore_object_medium_parse` |
| `cli::setup_parse` | `<build-dir>/test/test_xtracer_cli_setup` |
| `ncf::inline_and_utf8` | `<build-dir>/test/test_ncf_parser` |
| `scene::validate_all` | `<build-dir>/test/test_xtcore_scene_validator` |
| `nmath::sampling` | `<build-dir>/test/test_nmath_sampling` |
| `cli::stencil_smoke` | `<build-dir>/xtracer_cli` smoke render |

Run all tests:

```bash
ctest --test-dir build/intermediate/build --output-on-failure
```

Convenience one-liners:

```bash
make check
make perf
```

- `make check`: configures/builds `build/intermediate/build` and runs all tests.
- `make perf`: configures/builds `build/perf-release` and runs `nmath::simd_perf_compare` via `nmath_perf_check`.

## Third-Party Dependencies

| Name | License | URL |
|---|---|---|
| TinyObjLoader | MIT | https://github.com/syoyo/tinyobjloader |
| STB | Public Domain / MIT | https://github.com/nothings/stb |
| TinyEXR | BSD-3-Clause | https://github.com/syoyo/tinyexr |
| strpool | Public Domain | https://github.com/mattiasgustavsson/libs |
| cpp-httplib | MIT | https://github.com/yhirose/cpp-httplib |
| RtMidi | MIT-style | https://github.com/thestk/rtmidi |

## License

BSD 3-Clause. See `LICENSE`.

Copyright (c) 2010-present Nikolaos Papadopoulos.
