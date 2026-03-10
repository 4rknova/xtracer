# Architecture Notes

## High-Level Flow

1. Parse scene (`.scn`) into `xtcore::Scene`.
2. Build render context (`xtcore::render::context_t`) with tiles.
3. Select integrator (`xtcore::render::IIntegrator` implementation).
4. Render tiles (OpenMP parallel path in base integrator flow).
5. Assemble output framebuffer for export/preview.

## Main Components

- Scene parsing/loading:
  - `src/xtcore/parseutil.h`
  - `src/xtcore/parseutil.cc`
- Render context and tile assembly:
  - `src/xtcore/context.h`
  - `src/xtcore/context.cc`
- Integrator interface:
  - `src/xtcore/integrator.h`
  - `src/xtcore/integrator.cc`

## Frontends

- CLI:
  - Entrypoint: `src/frontend/cli/xtracer.cc`
  - Arg parsing: `src/frontend/cli/argparse.cc`
- GUI:
  - Entrypoint: `src/frontend/gui/xtracer.cc`
  - Workspace and render threading: `src/frontend/gui/workspace.cc`, `src/frontend/gui/action.cc`
  - Integrator selection/menu: `src/frontend/gui/gui.cc`
