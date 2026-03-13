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
- Web frontend backend: `src/web-server/`
- WASM frontend runtime: `src/frontend/wasm/`
- Frontend shared helpers: `src/frontend/common/`
- Web static assets: `src/web-client/`
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
  - HTTP server via `ext/cpp-httplib/httplib.h`
  - Async render jobs managed in `src/web-server/job_manager.*`
  - Shared render pipeline in `src/frontend/common/render_service.*`
  - Backend log stream in `src/web-server/backend_log.*`
  - Static SPA in `src/web-client/` with tabs: `Render`, `Editor`, `Settings`, `Logs`, `About`

### Web API Surface (Current)

- `GET /api/health`
- `GET /api/about`
- `GET /api/scenes`
- `GET /api/scenes/{scene}/cameras`
- `GET /api/scenes/{scene}/source`
- `POST /api/scenes/save`
- `GET /api/integrators`
- `POST /api/render`
- `GET /api/jobs/{id}`
- `GET /api/jobs/{id}/image`
- `GET /api/logs?since=<id>`

### Web Runtime Notes

- `xtracer_web` defaults:
  - host: `127.0.0.1`
  - port: `8080`
  - scene dir: `scene/`
  - web root: `src/web-client/`
- Job execution is serialized via a global render mutex in web backend (avoids OpenMP oversubscription from concurrent jobs).
- PNG responses are currently produced by rendering to `nimg::Pixmap` then encoding via temporary file path.

## Branch Relationship Reminder

See: `docs/BRANCH_RELATIONSHIP.md`

Short version:

- `master` and `develop` diverged years ago.
- `develop` contains newer 2024 rendering/build cleanup commits (remotery removal and optional GUI build).
- `master` contains several repo housekeeping commits not present on `develop`.

## Task-Specific References

- Architecture notes: `docs/ARCHITECTURE_NOTES.md`
- Renderer/integrator inventory: `docs/RENDERERS.md`
- Web UI/feature behavior: `src/web-client/index.html`, `src/web-client/app.js`, `src/web-client/styles.css`

## Scene Format Reference (Parser-Backed)

Canonical scene fields parsed in `src/xtcore/parseutil.cc` / `src/xtcore/proto.h`:

- Header:
  - `title`, `description`, `version`
- Top-level groups:
  - `environment`, `camera`, `geometry`, `material`, `object`

Supported environment `type` values:

- `gradient` (`config.a`, `config.b` as `col3(...)`)
- `color` (`config.value` as `col3(...)`)
- `cubemap` (`config.posx/posy/posz/negx/negy/negz`)
- `erp` (`config.source`)

Supported camera `type` values:

- `thin-lens` (position/target/up/fov/flength/aperture)
- `ods` (position/orientation/ipd)
- `erp` (position/orientation)
- `cubemap` (position)

Supported geometry `type` values:

- `plane` (`normal`, `distance`)
- `sphere` (`position`, `radius`)
- `point` (`position`) (internally a zero-radius sphere)
- `triangle` (`vecdata.v0/v1/v2`)
- `mesh`:
  - external source: `source = <path>.obj`
  - procedural generators: `source = gen(plane|icosahedron|tetrahedron|cube|hexahedron|octahedron|dodecahedron|capsule|cylinder|capped_cylinder|cone|truncated_cone|ring|torus_knot|icosphere|geodesic_dome|menger_sponge|sierpinski_tetrahedron|mobius_strip|klein_bottle|snowflake)`
  - optional: `resolution` (mesh complexity / iterations for generated meshes), `modifiers` (`rotation`, `scale`, `translation`, `flip_normals`, `extrude`)

Supported material `type` values:

- `lambert`, `phong`, `blinn_phong`, `emissive`, `dielectric`

Sampler `type` values in `material.properties.samplers`:

- `color`, `texture`, `cubemap`, `erp`, `gradient`

Object forms:

- In-scene reference form:
  - `object.<name> = { geometry = <geometry_id>, material = <material_id> }`
- External OBJ import form:
  - `object.<name> = { source = <path>.obj, prefix = <string> }`

Notes:

- Many legacy scenes use ad-hoc extra keys; parser ignores unknown fields.
- Prefer canonical `description` / `version` keys for new scenes.
- `point` is represented internally as a sphere; parser now maps it to an epsilon radius to avoid zero-radius runtime issues.

## Working Conventions For Agents

- Prefer minimal, surgical changes.
- Do not revert unrelated working tree changes.
- Keep tile-based architecture unless intentionally redesigning it.
- When a code/config/API/feature change affects documented behavior, update `README.md` in the same task so it reflects the current repository state.
- When changing rendering flow, validate worker-thread behavior in web/WASM frontends.
- When changing web API responses, update both:
  - backend route handlers in `src/web-server/routes.cc`
  - frontend consumers in `src/web-client/app.js`
- If adding new integrators, update both:
  - `src/xtcore/integrator.h`
  - Integrator registry in `src/frontend/common/render_service.cc`

## Commit Message Format

- Subject line should be short and scoped (e.g., `build: ...`).
- Body should use action-verb bullets in present tense.
- Prefer wording like `adds`, `fixes`, `removes`, `updates`, `gates`, `routes`.
