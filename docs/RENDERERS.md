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

## Exposed Via Web API

In `src/frontend/common/render_service.cc` integrator registry:

- raytracer
- pathtracer
- pathtracer_mis
- pathtracer_mis_full
- photon_mapping
- debug_views
- ao

## Notes

- CLI integrator switch logic currently commented in `src/frontend/cli/xtracer.cc`.
