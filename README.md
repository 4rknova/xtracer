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
./build/release/xtracer_web --host 127.0.0.1 --port 8080 --scene-dir scene --web-root src/frontend/web-client --max-concurrent-renders 1 --render-reserve-threads 1 --verbose
```

Open: `http://127.0.0.1:8080`

Run CLI:

```bash
./build/release/xtracer_cli scene/lab-camera-modes-showcase.scn -renderer pathtracer_mis -res 1280x720 -samples 4 -aa 2
```

## Repository Layout

| Area | Path | Purpose |
|---|---|---|
| Core renderer | `src/xtcore/` | Scene parsing, render context, integrators, tone mapping |
| CLI frontend | `src/frontend/cli/` | Command-line scene rendering |
| Web frontend backend | `src/frontend/web-server/` | HTTP API, job manager, log stream |
| Frontend shared code | `src/frontend/common/` | Shared render service + integrator metadata |
| Web static app | `src/frontend/web-client/` | SPA for Render / Editor / Settings / Logs / About |
| Scenes | `scene/` | Example scene files (`.scn`) |
| Supporting libs | `lib/` | Internal libraries (`nimg`, `nmesh`, `nmath`, etc.) |
| Third-party deps | `ext/` | Vendored external dependencies |

## Feature Matrix

### Frontends

| Capability | CLI (`xtracer_cli`) | Web (`xtracer_web`) | WASM (`xtracer_wasm`) |
|---|---:|---:|---:|
| Load `.scn` scenes | Yes | Yes | Yes |
| Select camera | Yes (`-cam`) | Yes | Yes |
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
| `thin-lens` | Position/target/up/fov/flength/aperture |
| `ods` | Omni-directional stereo camera |
| `erp` | Equirectangular camera |
| `cubemap` | Cubemap camera |

#### Geometry Types

| Type | Notes |
|---|---|
| `plane` | Analytic plane |
| `sphere` | Analytic sphere |
| `point` | Parsed as epsilon-radius sphere |
| `triangle` | Triangle via `vecdata.v0/v1/v2` |
| `mesh` | External OBJ or procedural generator |

#### Procedural Mesh Generators (`geometry.type = mesh`, `source = gen(...)`)

| Generator | Generator | Generator | Generator |
|---|---|---|---|
| `plane` | `icosahedron` | `tetrahedron` | `cube` |
| `hexahedron` | `octahedron` | `dodecahedron` | `capsule` |
| `cylinder` | `capped_cylinder` | `cone` | `truncated_cone` |
| `ring` | `torus_knot` | `icosphere` | `geodesic_dome` |
| `icosa_cage` | `menger_sponge` | `sierpinski_tetrahedron` | `mobius_strip` |
| `klein_bottle` | `hairball` | `shell_spiral` | `rock` |
| `chain_link` | `lathe` | `snowflake` | - |

#### Material Types

| Type |
|---|
| `lambert` |
| `phong` |
| `blinn_phong` |
| `emissive` |
| `dielectric` |

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

## Web App + API

### Web App Tabs

| Tab | Key Capabilities |
|---|---|
| Render | Scene/camera/integrator selection, render settings, preview, export (with persistent left sidebar cards available across tabs) |
| Editor | Switchable `3D View` / `Graph` / `Text Editor` modes, scene source editor, create geometry, mesh translate/rotate/scale controls, click-select + Ctrl-drag move, `F` focus shortcut, visual viewport integration, scene save |
| Settings | Theme mode + dark palette selection, frontend behavior toggles, and render polling controls |
| Logs | Backend log stream with wait-based incremental updates and level filters |
| About | Build/backend metadata, project license text, and third-party license notices |

### Web API Endpoints

| Method | Endpoint | Purpose |
|---|---|---|
| GET | `/api/health` | Health probe |
| GET | `/api/about` | Backend/app metadata, runtime capacity stats, and license/third-party notice fields |
| GET | `/api/scenes` | List available scenes |
| GET | `/api/scenes/{scene}/cameras` | List cameras in scene (returns `202` while async scene load is in progress) |
| GET | `/api/scenes/{scene}/source` | Fetch scene source |
| GET | `/api/scenes/{scene}/geometry` | Extract mesh geometry payload (returns `202` while async scene load is in progress) |
| GET | `/api/scenes/{scene}/runtime_graph` | Fetch runtime-resolved scene graph (objects/surfaces/materials/cameras) (returns `202` while async scene load is in progress) |
| GET | `/api/scenes/{scene}/camera_resolve` | Resolve active camera metadata (returns `202` while async scene load is in progress) |
| GET | `/api/scenes/load_jobs/{id}` | Poll async scene load job status |
| GET | `/api/scenes/{scene}/asset?path=...` | Fetch referenced scene asset |
| GET | `/api/scenes/template/empty` | Empty scene template |
| POST | `/api/scenes/save` | Save scene source |
| GET | `/api/workspaces?client_id={id}` | List workspaces + active workspace + workspace-scoped settings snapshot |
| POST | `/api/workspaces` | Create workspace for client context |
| POST | `/api/workspaces/active` | Switch active workspace for client |
| POST | `/api/workspaces/delete` | Delete workspace |
| POST | `/api/workspaces/scene_draft` | Save workspace-local scene draft |
| POST | `/api/workspaces/settings` | Save workspace UI settings (quality/frame/integrator/tone mapping/post-filters) |
| GET | `/api/integrators` | List backend integrators + controls |
| GET | `/api/resolutions` | Resolution presets |
| POST | `/api/render` | Create render job |
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

> Note: native binaries are configured to output under repo-local `build/debug` (Debug) or `build/release` (non-Debug).

### CMake Options

| Option | Default | Description |
|---|---:|---|
| `XTRACER_ENABLE_RTMIDI` | `ON` | Build RtMidi/ALSA support |
| `XTRACER_ENABLE_WEB` | `ON` | Build HTTP web frontend |
| `XTRACER_ENABLE_WASM` | `OFF` | Build standalone WASM runtime |
| `XTRACER_ENABLE_WASM_DIST` | `OFF` | Build/package standalone WASM dist during native build |
| `XTRACER_ENABLE_VIZ` | `OFF` | Build OpenGL sampling visualization tool (`xtracer_viz_sampling`) |

## Run

### CLI

```bash
./build/release/xtracer_cli scene/lab-camera-modes-showcase.scn -renderer pathtracer_mis -res 1280x720 -samples 4 -aa 2
```

### Web Server

```bash
./build/release/xtracer_web --host 127.0.0.1 --port 8080 --scene-dir scene --web-root src/frontend/web-client --max-concurrent-renders 1 --render-reserve-threads 1 --verbose
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
./build/release/bench_nmath
```

### Sampling Visualization Tool

Build with visualization enabled:

```bash
cmake -S . -B build/intermediate/build-viz -DXTRACER_ENABLE_VIZ=ON
cmake --build build/intermediate/build-viz -j --target xtracer_viz_sampling
```

Run:

```bash
./build/release/xtracer_viz_sampling
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
- copied self-contained scenes from `scene/` into build output `build/release/scenes/` (or `build/debug/scenes/` for Debug-native builds)

Optional static packaging:

```bash
./util/package_wasm_standalone.sh
```

## Test Targets

| Test Name (CTest) | Binary |
|---|---|
| `colorspace::roundtrip` | `build/debug/test/test_nimg_colorspace` or `build/release/test/test_nimg_colorspace` |
| `colorspace::vectors` | `build/debug/test/test_nimg_colorspace_vectors` or `build/release/test/test_nimg_colorspace_vectors` |
| `xtcore::tile` | `build/debug/test/test_xtcore_tile` or `build/release/test/test_xtcore_tile` |
| `xtcore::context` | `build/debug/test/test_xtcore_context` or `build/release/test/test_xtcore_context` |
| `xtcore::sphere` | `build/debug/test/test_xtcore_sphere` or `build/release/test/test_xtcore_sphere` |
| `xtcore::triangle` | `build/debug/test/test_xtcore_triangle` or `build/release/test/test_xtcore_triangle` |
| `xtcore::white_furnace` | `build/debug/test/test_xtcore_white_furnace` or `build/release/test/test_xtcore_white_furnace` |
| `xtcore::raytracer_emissive` | `build/debug/test/test_xtcore_raytracer_emissive` or `build/release/test/test_xtcore_raytracer_emissive` |
| `cli::setup_parse` | `build/debug/test/test_xtracer_cli_setup` or `build/release/test/test_xtracer_cli_setup` |
| `ncf::inline_and_utf8` | `build/debug/test/test_ncf_parser` or `build/release/test/test_ncf_parser` |
| `nmath::sampling` | `build/debug/test/test_nmath_sampling` or `build/release/test/test_nmath_sampling` |
| `cli::stencil_smoke` | `build/debug/xtracer_cli` or `build/release/xtracer_cli` smoke render |

Run all tests:

```bash
ctest --test-dir build/intermediate/build --output-on-failure
```

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
