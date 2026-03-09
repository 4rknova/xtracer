# AGENTS.md

This file is the orientation and operating guide for coding agents working in this repository.

## Project Snapshot

- Name: `xtracer`
- Language: C/C++ (CMake)
- Branch context: this workspace is currently on `develop`.
- Core purpose: experimental rendering framework with a shared rendering core and two frontends.

## Repository Map

- Core renderer and scene system: `src/xtcore/`
- CLI frontend: `src/frontend/cli/`
- GUI frontend (OpenGL + ImGui): `src/frontend/gui/`
- Supporting libraries: `lib/`
- Third-party dependencies: `ext/`
- Scene examples: `scene/`
- Build definition: `CMakeLists.txt`

## Build Notes

- Historically documented flow in README:
  - `./configure`
  - `make`
- Practical modern flow is CMake-driven.
- GUI build is optional (see `XTRACER_ENABLE_GUI` usage in `CMakeLists.txt`).
- In this workspace, in-source CMake artifacts exist (`CMakeCache.txt`, `CMakeFiles/`, etc.). Prefer out-of-tree builds for new runs.

## Known Current State (Develop)

- Integrator implementations exist under `src/xtcore/integrator/`.
- GUI exposes several integrators via menu in `src/frontend/gui/gui.cc`.
- CLI integrator selection code in `src/frontend/cli/xtracer.cc` is currently commented out; treat CLI rendering path as needing repair before relying on it.

## Realtime Integrator Initiative

A new `realtime_gl` integrator scaffold has been added:

- `src/xtcore/integrator/realtime_gl/integrator.h`
- `src/xtcore/integrator/realtime_gl/integrator.cc`

Current status:

- It participates in the tile-based pipeline.
- It is currently a CPU preview implementation (normal-based shading) used as scaffold.
- It is listed in GUI as `Realtime (OpenGL) [WIP]`.

Important architectural constraint:

- Existing render jobs run in a detached worker thread.
- OpenGL context is owned by GUI thread.
- True OpenGL tile rendering must move GL draw execution to GUI thread (or explicitly establish shared context strategy).

## Branch Relationship Reminder

See: `docs/BRANCH_RELATIONSHIP.md`

Short version:

- `master` and `develop` diverged years ago.
- `develop` contains newer 2024 rendering/build cleanup commits (remotery removal and optional GUI build).
- `master` contains several repo housekeeping commits not present on `develop`.

## Task-Specific References

- Architecture notes: `docs/ARCHITECTURE_NOTES.md`
- Renderer/integrator inventory: `docs/RENDERERS.md`
- Realtime GL implementation plan: `docs/REALTIME_GL_PLAN.md`

## Working Conventions For Agents

- Prefer minimal, surgical changes.
- Do not revert unrelated working tree changes.
- Keep tile-based architecture unless intentionally redesigning it.
- When changing rendering flow, validate GUI-thread vs worker-thread behavior.
- If adding new integrators, update both:
  - `src/xtcore/integrator.h`
  - GUI integrator registry in `src/frontend/gui/gui.cc`

## Commit Message Format

- Subject line should be short and scoped (e.g., `build: ...`).
- Body should use action-verb bullets in present tense.
- Prefer wording like `adds`, `fixes`, `removes`, `updates`, `gates`, `routes`.
