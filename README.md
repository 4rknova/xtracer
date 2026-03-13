<img align="center" src="https://raw.githubusercontent.com/4rknova/xtracer/develop/res/preview.jpg">

# XTRACER

Experimental rendering framework written in C/C++ with a shared core (`xtcore`) and multiple frontends (CLI, Web, WASM runtime).

[![CI](https://github.com/4rknova/xtracer/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/4rknova/xtracer/actions/workflows/ci.yml)

## Current Status (`develop`)

- Build system is CMake-driven (legacy `./configure && make` still exists, but CMake is the maintained path).
- Core renderer and scene parsing are active under `src/xtcore/`.
- HTTP web frontend is optional via `XTRACER_ENABLE_WEB`.
- Standalone WASM renderer runtime is optional via `XTRACER_ENABLE_WASM`.

## Repository Layout

| Area | Path | Purpose |
|---|---|---|
| Core renderer | `src/xtcore/` | Scene parsing, render context, integrators, tone mapping |
| CLI frontend | `src/frontend/cli/` | Command-line scene rendering |
| Web frontend backend | `src/frontend/web/` | HTTP API, job manager, log stream |
| Frontend shared code | `src/frontend/common/` | Shared render service + integrator metadata |
| Web static app | `res/web/` | SPA for Render / Editor / Settings / Logs / About |
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
| Settings | Theme mode + dark palette selection, frontend behavior toggles, and polling controls |
| Logs | Backend log stream with incremental polling and level filters |
| About | Build/backend metadata, project license text, and third-party license notices |

### Web API Endpoints

| Method | Endpoint | Purpose |
|---|---|---|
| GET | `/api/health` | Health probe |
| GET | `/api/about` | Backend/app metadata + license/third-party notice fields |
| GET | `/api/scenes` | List available scenes |
| GET | `/api/scenes/{scene}/cameras` | List cameras in scene |
| GET | `/api/scenes/{scene}/source` | Fetch scene source |
| GET | `/api/scenes/{scene}/geometry` | Extract mesh geometry payload |
| GET | `/api/scenes/{scene}/camera_resolve` | Resolve active camera metadata |
| GET | `/api/scenes/{scene}/asset?path=...` | Fetch referenced scene asset |
| GET | `/api/scenes/template/empty` | Empty scene template |
| POST | `/api/scenes/save` | Save scene source |
| GET | `/api/integrators` | List backend integrators + controls |
| GET | `/api/resolutions` | Resolution presets |
| POST | `/api/render` | Create render job |
| GET | `/api/jobs/{id}` | Job status snapshot |
| GET | `/api/jobs/{id}/image` | PNG preview/final image |
| GET | `/api/jobs/{id}/export?format={png,jpg,bmp,tga,exr,hdr,ply}` | Download final export (PLY is raygraph) |
| GET | `/api/jobs/{id}/photons` | Photon debug points |
| GET | `/api/logs?since={id}` | Incremental backend logs |

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
cmake -S . -B build -DXTRACER_ENABLE_WEB=ON
cmake --build build -j
```

> Note: native binaries are configured to output under repo-local `bin/debug` (Debug) or `bin/release` (non-Debug).

### CMake Options

| Option | Default | Description |
|---|---:|---|
| `XTRACER_ENABLE_RTMIDI` | `ON` | Build RtMidi/ALSA support |
| `XTRACER_ENABLE_WEB` | `ON` | Build HTTP web frontend |
| `XTRACER_ENABLE_WASM` | `OFF` | Build standalone WASM runtime |
| `XTRACER_ENABLE_WASM_DIST` | `OFF` | Build/package standalone WASM dist during native build |

## Run

### CLI

```bash
./bin/release/xtracer_cli scene/lab-camera-modes-showcase.scn -renderer pathtracer_mis -res 1280x720 -samples 4 -aa 2
```

### Web Server

```bash
./bin/release/xtracer_web --host 127.0.0.1 --port 8080 --scene-dir scene --web-root res/web --verbose
```

Open: `http://127.0.0.1:8080`

### Math Microbenchmark

```bash
./bin/release/bench_nmath
```

### WASM Runtime Build

```bash
emcmake cmake -S . -B build-wasm \
  -DXTRACER_ENABLE_WEB=OFF \
  -DXTRACER_ENABLE_RTMIDI=OFF \
  -DXTRACER_ENABLE_WASM=ON
cmake --build build-wasm -j --target xtracer_wasm
```

Expected output:

- `res/web/xtracer_wasm.js`
- `res/web/xtracer_wasm.wasm`

Optional static packaging:

```bash
./util/package_wasm_standalone.sh
```

## Test Targets

| Test Name (CTest) | Binary |
|---|---|
| `colorspace::roundtrip` | `bin/debug/test/test_nimg_colorspace` or `bin/release/test/test_nimg_colorspace` |
| `colorspace::vectors` | `bin/debug/test/test_nimg_colorspace_vectors` or `bin/release/test/test_nimg_colorspace_vectors` |
| `xtcore::tile` | `bin/debug/test/test_xtcore_tile` or `bin/release/test/test_xtcore_tile` |
| `xtcore::context` | `bin/debug/test/test_xtcore_context` or `bin/release/test/test_xtcore_context` |
| `xtcore::sphere` | `bin/debug/test/test_xtcore_sphere` or `bin/release/test/test_xtcore_sphere` |
| `xtcore::triangle` | `bin/debug/test/test_xtcore_triangle` or `bin/release/test/test_xtcore_triangle` |
| `xtcore::white_furnace` | `bin/debug/test/test_xtcore_white_furnace` or `bin/release/test/test_xtcore_white_furnace` |
| `xtcore::raytracer_emissive` | `bin/debug/test/test_xtcore_raytracer_emissive` or `bin/release/test/test_xtcore_raytracer_emissive` |
| `cli::setup_parse` | `bin/debug/test/test_xtracer_cli_setup` or `bin/release/test/test_xtracer_cli_setup` |
| `ncf::inline_and_utf8` | `bin/debug/test/test_ncf_parser` or `bin/release/test/test_ncf_parser` |
| `cli::stencil_smoke` | `bin/debug/xtracer_cli` or `bin/release/xtracer_cli` smoke render |

Run all tests:

```bash
ctest --test-dir build --output-on-failure
```

## Third-Party Dependencies

| Name | License | URL |
|---|---|---|
| TinyObjLoader | MIT | https://github.com/syoyo/tinyobjloader |
| TinyFiles | Public Domain | https://github.com/RandyGaul/tinyheaders/blob/master/tinyfiles.h |
| STB | Public Domain / MIT | https://github.com/nothings/stb |
| TinyEXR | BSD-3-Clause | https://github.com/syoyo/tinyexr |
| strpool | Public Domain | https://github.com/mattiasgustavsson/libs |
| cpp-httplib | MIT | https://github.com/yhirose/cpp-httplib |
| RtMidi | MIT-style | https://github.com/thestk/rtmidi |

## License

BSD 3-Clause. See `LICENSE`.

Copyright (c) 2010-present Nikolaos Papadopoulos.
