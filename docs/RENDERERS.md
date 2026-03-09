# Renderers / Integrators

## Implemented Integrators (Develop)

Found under `src/xtcore/integrator/`:

1. `pathtracer`
2. `ao`
3. `depth`
4. `stencil`
5. `normal`
6. `uv`
7. `emission`
8. `wireframe`
9. `embree` (feature-gated)
10. `realtime_gl` (WIP scaffold)

## Exposed In GUI Menu

In `src/frontend/gui/gui.cc` integrator registry:

- Pathtracer
- Depth
- Stencil
- Normal
- UV
- Emission
- Ambient Occlusion
- Realtime (OpenGL) [WIP]

## Notes

- `wireframe` and `embree` are implemented but not currently listed in GUI registry.
- CLI integrator switch logic currently commented in `src/frontend/cli/xtracer.cc`.

