<p align="center">
  <img src="https://raw.githubusercontent.com/4rknova/xtracer/develop/res/logo.svg" alt="xtracer" width="400">
</p>

<p align="center">
  <a href="https://hub.docker.com/r/4rknova/xtracer"><img src="https://img.shields.io/docker/pulls/4rknova/xtracer?label=Docker%20pulls" alt="Docker Pulls"></a>
  <a href="https://hub.docker.com/r/4rknova/xtracer"><img src="https://img.shields.io/docker/v/4rknova/xtracer?sort=semver&label=Docker%20Hub" alt="Docker Hub"></a>
  <a href="https://github.com/4rknova/xtracer/pkgs/container/xtracer"><img src="https://img.shields.io/badge/ghcr.io-available-blue?logo=github" alt="GitHub Container Registry"></a>
</p>

Experimental rendering framework written in C/C++ with a shared core (`xtcore`) and a web frontend.

`xtracer` is a physically-based ray/path tracing engine built for exploration and experimentation. The core library (`xtcore`) handles scene parsing, ray-geometry intersection, shading, and tone mapping — consumed by the web frontend (`xtracer`): an HTTP API server with a job queue, progressive preview streaming, and a browser-based SPA for scene selection, rendering, log inspection, and image export in multiple formats.

The engine supports a range of integrators from simple Whitted-style ray tracing to MIS path tracing with area-light and environment sampling, plus photon mapping, ambient occlusion, and several debug views. Scenes are described in a custom `.scn` format covering procedural and mesh geometry, spline-following tube curves, SVG-backed silhouette mesh generation, analytic cameras (thin-lens with polygonal bokeh, ODS, ERP, cubemap), and environment types including Rayleigh sky.

<p align="center">
<img src="https://raw.githubusercontent.com/4rknova/xtracer/develop/src/apps/web-client/res/ftue.png" alt="preview" width="100%">
</p>

## Quick Start (Install + Run)

Install dependencies (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libomp-dev zlib1g-dev
```

Configure + build:

```bash
cmake -S . -B build/intermediate/build
cmake --build build/intermediate/build -j
```

Run:

```bash
./build/intermediate/build/xtracer \
    --host 127.0.0.1 \
    --port 8080 \
    --scene-dir scene \
    --web-root src/apps/web-client \
    --max-concurrent-renders 1 \
    --render-reserve-threads 1 \
    --verbose
```

Open: `http://127.0.0.1:8080`

UI showcase: `http://127.0.0.1:8080/showcase.html`

## Repository Layout

| Area | Path | Purpose |
|---|---|---|
| Core renderer | `src/xtcore/` | Scene parsing, render context, integrators, tone mapping |
| App backend | `src/apps/xtracer/` | HTTP API, job manager, render service, log stream |
| Web static app | `src/apps/web-client/` | SPA for Scene / Render / Editor / Workspaces / Gallery / Settings / Logs / About, including runtime JSON config in `app/data/` |
| Mitsuba scene converter | `src/convertMitsuba/` | Standalone `convertMitsuba` tool for converting Mitsuba XML/zip scenes into `.scn` scenes plus packaged assets |
| Scenes | `scene/` | Example scene files (`.scn`) |
| Supporting libs | `lib/` | Internal libraries (`nimg`, `nmesh`, `nmath`, etc.) |
| Third-party deps | `ext/` | Vendored external dependencies (registry: `docs/DEPENDENCIES.md`) |

## Feature Matrix

### Frontends

| Capability | `xtracer` |
|---|---:|
| Load `.scn` scenes | Yes |
| Select camera | Yes |
| Select scene variant | Yes (`variant` API param) |
| Integrator selection | Yes (`/api/integrators`) |
| Progressive updates | Job progress + preview API |
| Image export | PNG/JPG/BMP/TGA/HDR/EXR + Raygraph PLY (`/api/jobs/{id}/export`) |
| HTTP API | Yes |

### Integrators

| Integrator ID | Type | Exposed In `/api/integrators` |
|---|---|---:|
| `raytracer` | Whitted-style | Yes |
| `pathtracer_mis` | MIS path tracing with emissive-area and environment sampling | Yes |
| `pathtracer` | Brute-force path tracing | Yes |
| `photon_mapping` | Photon mapping | Yes |
| `debug_views` | Multi-mode debug integrator | Yes |
| `ao` | Ambient occlusion | Yes |
| `pathtracer_bdpt` | Experimental bidirectional path tracing with MIS path connection | Yes |
| `depth` | Debug depth alias | No (alias accepted in `/api/render`) |
| `stencil` | Debug stencil alias | No (alias accepted in `/api/render`) |
| `normal` | Debug normal alias | No (alias accepted in `/api/render`) |
| `uv` | Debug UV alias | No (alias accepted in `/api/render`) |
| `emission` | Debug emission alias | No (alias accepted in `/api/render`) |

See [docs/INTEGRATORS.md](docs/INTEGRATORS.md) for integrator details, `debug_views` mode reference, and usage guidance.

### Scene Schema Support (`.scn`)

Full format reference: [docs/SCENE_FORMAT.md](docs/SCENE_FORMAT.md)

| Asset type | Reference |
|---|---|
| Geometry types, analytic primitives, fractals, mesh generators, CSG | [docs/GEOMETRY.md](docs/GEOMETRY.md) |
| Camera types and parameters | [docs/CAMERAS.md](docs/CAMERAS.md) |
| Material types, sampler slots, scalar parameters | [docs/MATERIALS.md](docs/MATERIALS.md) |
| Sampler types (procedural, image, environment) | [docs/SAMPLERS.md](docs/SAMPLERS.md) |


## Web App + API

### Web App Tabs

| Tab | Key Capabilities |
|---|---|
| Scene | File-manager-style scene browser plus fixed-size camera/variant cards (with variant name + description metadata), active selection panels, single-click selection, double-click activation, and scene file right-click actions (`Set Active`, `Delete`) |
| Render | Scene/camera/integrator selection, render settings, a square preview container that fills the render pane as the largest square that fits, tile-size presets (`8`, `32`, `64`, `Auto` where auto derives a square tile from frame size and effective thread count), a preview-toolbar export format dropdown + live format-aware save button, preview sampling toggle, in-flight abort support (render action toggles `Render`/`Abort`), render modes (`Direct`, `Progressive`, `Incremental`, `Interactive`) with `Progressive` as the default frontend mode, interactive camera controls/ramping, plus post-filter stack controls (enable/disable + chain) applied to preview/export |
| Editor | Switchable `3D View` / `Graph` / `Text Editor` modes, scene source editor, create geometry, mesh translate/rotate/scale controls, 3D scene scale multiplier, click-select + Ctrl-drag move, `F` focus shortcut, visual viewport integration, scene save |
| Gallery | Cached render browser with card grid, detail view (with tone mapping operator + parameter controls), pass thumbnails for progressive/incremental renders, refresh, and delete. Renders are cached server-side in full-precision EXR format; the server decodes and tonemaps to PNG on each request. Callers can opt a render out of gallery persistence with `save_to_gallery=0` when using `/api/render`. |
| Settings | Theme mode + light/dark palette selection, frontend behavior toggles, render polling controls, and first-time tutorial reset/start controls |
| Logs | Backend log stream with wait-based incremental updates and level filters |
| About | Build/backend metadata (including build type), project license text, and third-party dependency notices including usage/location |

Shared sidebar jobs card:
- Shows a live 1 minute thread-usage graph plus current active/queued jobs, queue ordering controls, and abort actions wherever the card is enabled.

First-time use tutorial (FTUE):
- On first launch, the web app opens a guided tutorial for scene selection, rendering, and scene editing flow.
- In `Settings`, enable `Show tutorial on next launch` to reset onboarding state for the next app start.
- In `Settings`, use `Start Tutorial Now` to reopen the tutorial immediately.
- Tutorial steps are config-driven via `src/apps/web-client/app/data/ftue_steps.json` (`steps[]` entries support `title`, `body`, `target_selector`, `placement`, `tab`, `editor_view`, `open_cards`, and optional `focus_selector`).

Post-filter stack:
- Current filters: `desaturate`; `chromatic_aberration` (`amount`, `center_x`, `center_y`, `falloff`); `vignette` (`strength`, `radius`, `softness`, `center_x`, `center_y`); `film_grain` (`amount`, `size`, `seed`, `luma_weighted`); `denoise` (bilateral: `strength`, `radius`, `sigma`); `fxaa` (`subpix`, `edge_threshold`, `edge_threshold_min`); `sharpen` (`amount`, `radius`, `threshold`); `brightness` (`amount`); `contrast` (`amount`, `pivot`); `raindrops_lens` (`density`, `size`, `distortion`, `seed`).

Render preview interactions:
- Mouse wheel zooms the preview image.
- Drag pans the preview while zoomed.
- Double-click or `Reset View` resets preview pan/zoom.
- Clicking or dragging on the preview minimap recenters the current zoom on that region.
- At high zoom levels, per-pixel RGB values are overlaid directly on each source pixel.
- The preview toolbar includes export format + save controls and a two-icon sampling switch (`Smooth`, `Nearest`).

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

See [docs/API.md](docs/API.md) for the full REST and WebSocket API reference, including all endpoint parameters, response shapes, tonemapping parameters, and the XTDR binary tile protocol.

## Build

### Prerequisites (Debian/Ubuntu)

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libomp-dev zlib1g-dev
```

### Recommended Native Build (Out-of-Tree)

```bash
cmake -S . -B build/intermediate/build
cmake --build build/intermediate/build -j
```

> Note: native binaries are configured to output under the chosen CMake build directory
> (for example `build/intermediate/build/`).

### CMake Options

| Option | Default | Description |
|---|---:|---|
| `XTRACER_ENABLE_VIZ` | `OFF` | Build OpenGL sampling visualization tool (`xtracer_viz_sampling`) |
| `XTRACER_ENABLE_NMATH_SIMD` | `ON` | Enable x86 SSE2 SIMD fast-paths for `nmath` double-precision vector and matrix operations |
| `XTRACER_ENABLE_NMATH_SIMD_AVX` | `ON` | Use AVX path for `nmath` SIMD (`XTRACER_ENABLE_NMATH_SIMD` must be `ON`) |

Notes:
- SIMD paths are used only when `nmath` is built in double precision (default configuration); scalar fallback remains available.
- `XTRACER_ENABLE_NMATH_SIMD_AVX` enables AVX codegen and runtime AVX instructions for supported hosts.

## Run

### Mitsuba Converter

```bash
./build/intermediate/build/convertMitsuba path/to/scene.xml -o scene.scn
./build/intermediate/build/convertMitsuba path/to/archive.zip -d scene
```

### Server

```bash
./build/intermediate/build/xtracer \
    --host 127.0.0.1 \
    --port 8080 \
    --scene-dir scene \
    --web-root src/apps/web-client \
    --max-concurrent-renders 1 \
    --render-reserve-threads 1 \
    --verbose
```

Open: `http://127.0.0.1:8080`

`--gallery-dir` controls where cached gallery renders and per-pass previews are stored (default: `gallery`). Renders are stored in full-precision EXR format; the server decodes and applies tonemapping to PNG on each image request. Gallery entry directories are named `YYYYMMDD-HHMMSS-mmm_XXXXXXXX` (UTC datetime with milliseconds + 8-hex FNV-1a hash of the scene name), ensuring unique, stable IDs that survive server restarts.
`--max-concurrent-renders` controls how many render jobs execute simultaneously (default: `1`).
`--render-reserve-threads` controls how many threads auto-render mode keeps free for server responsiveness (default: `1`).
When `/api/render` uses `threads=0`, backend auto mode resolves to `max(1, runtime_threads - reserve_threads)` (single-core hosts still render with `1` thread).
The web backend also bounds pending render backlog to `32` queued jobs; extra `/api/render` requests return `503` instead of accumulating unbounded queued state.
Workspace state is also bounded: the backend retains at most `32` workspaces, evicts orphaned idle workspaces after `60` minutes, and returns `409` from `/api/workspaces` if all retained slots are still active.
Normal scene renders continue to use workspace-backed state, but `/api/render` also accepts an inline `scene_source` override for ad-hoc inline scene renders without creating a workspace.
Startup prints an ASCII banner with runtime info (host/port, paths, concurrency, and detected core/thread limits).
Material gallery thumbnails are intended to be pre-rendered into `src/apps/web-client/res/lib/materials/` rather than generated on each tab visit. Refresh them with:

```bash
./util/gen_material_thumbnails.sh
```

The material gallery sidebar reads those static PNGs and only submits render jobs when opening the larger preview via `POST /api/materials/<id>/preview`.

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
- Set `XTRACER_OMP_NUM_THREADS` in `.env` to control OpenMP worker count.

Stop:

```bash
docker compose down
```

### Math Microbenchmark

```bash
./build/intermediate/build/bench_nmath
```

### Mesh Intersection Benchmark

```bash
./build/intermediate/build/bench_mesh_intersection
```

Optional tuning:

```bash
./build/intermediate/build/bench_mesh_intersection \
    --resolution 128 \
    --width 512 \
    --height 512 \
    --passes 8
```

### Sampling Visualization Tool

Build with visualization enabled:

```bash
cmake -S . -B build/intermediate/build-viz -DXTRACER_ENABLE_VIZ=ON
cmake --build build/intermediate/build-viz -j \
    --target xtracer_viz_sampling
```

Run:

```bash
./build/intermediate/build-viz/xtracer_viz_sampling
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
| `xtcore::csg` | `<build-dir>/test/test_xtcore_csg` |
| `xtcore::boundary_material` | `<build-dir>/test/test_xtcore_boundary_material` |
| `xtcore::white_furnace` | `<build-dir>/test/test_xtcore_white_furnace` |
| `xtcore::raytracer_emissive` | `<build-dir>/test/test_xtcore_raytracer_emissive` |
| `xtcore::object_medium_parse` | `<build-dir>/test/test_xtcore_object_medium_parse` |
| `xtcore::fbx_import` | `<build-dir>/test/test_xtcore_fbx_import` |
| `xtcore::gltf_import` | `<build-dir>/test/test_xtcore_gltf_import` |
| `xtcore::svg_mesh_generator` | `<build-dir>/test/test_xtcore_svg_mesh_generator` |
| `ncf::inline_and_utf8` | `<build-dir>/test/test_ncf_parser` |
| `scene::validate_all` | `<build-dir>/test/test_xtcore_scene_validator` |
| `nmath::sampling` | `<build-dir>/test/test_nmath_sampling` |
| `nmath::simd` | `<build-dir>/test/test_nmath_simd` |
| `nmath::simd_perf_compare` | `<build-dir>/test/test_nmath_simd_perf_compare` |

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

See [docs/DEPENDENCIES.md](docs/DEPENDENCIES.md) for the complete vendored dependency registry.

## License

BSD 3-Clause. See `LICENSE`.

Copyright (c) 2010-2026 Nikolaos Papadopoulos.
