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

## Exposed In GUI Menu

In `src/frontend/gui/gui.cc` integrator registry:

- Pathtracer
- Depth
- Stencil
- Normal
- UV
- Emission
- Ambient Occlusion

## Notes

- CLI integrator switch logic currently commented in `src/frontend/cli/xtracer.cc`.
