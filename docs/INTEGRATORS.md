# Renderers / Integrators

## Implemented Integrators (Develop)

Found under `src/xtcore/integrator/`:

1. `pathtracer`
2. `pathtracer_bdpt`
3. `pathtracer_mis`
4. `photon_mapping`
5. `raytracer`
6. `ao`
7. `debug_views`

The `debug_views` integrator subsumes what were previously standalone integrators. Those legacy names are still accepted by the web API for backwards compatibility and are routed to `debug_views` with the appropriate `mode` parameter. Available modes:

| Mode | Legacy API name | Notes |
|---|---|---|
| `normal` | `debug_views` (default) | Shading normal, respects normal maps |
| `depth` | `depth` | Supports `depth_encoding` (`legacy`, `linear`, `log`, `inverse`) and `max_distance` options |
| `stencil` | `stencil` | Binary hit mask |
| `uv` | `uv` | UV coordinates visualized as RG |
| `emission` | `emission` | Emissive channel only |
| `object_mask` | — | Per-object binary mask; takes `objects` option (comma-separated names) |

## Exposed Via Web API

Registered in `k_integrator_ids` / `create_integrator()` in `src/frontend/common/render_service.cc`:

| Name | Notes |
|---|---|
| `raytracer` | |
| `pathtracer` | |
| `pathtracer_mis` | |
| `pathtracer_bdpt` | |
| `photon_mapping` | |
| `debug_views` | Multi-mode debug integrator (see modes table above) |
| `ao` | |

## Notes

- CLI integrator switch logic currently commented in `src/frontend/cli/xtracer.cc`.
