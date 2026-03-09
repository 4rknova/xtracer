# Realtime GL Integrator Plan (Raster-First)

## Goal

Implement interactive realtime rendering while preserving tile-based scheduling where practical.

## Current Status

- `realtime_gl` integrator scaffold exists and is wired in.
- Current implementation is CPU-based preview shading (not GPU raster path yet).

## Phased Plan

1. Introduce GPU utility layer
- Add `src/xtcore/gpu/` helpers for program/FBO/state setup.
- Keep APIs small and testable.

2. Resolve GL threading ownership
- Option A (preferred): execute GL tile draws on GUI thread.
- Option B: create shared GL context for worker thread (higher complexity/risk).

3. Tile draw path
- For each tile, use viewport/scissor to restrict raster region.
- Render scene subset and write into workspace texture/FBO.
- Reuse existing tile completion signaling for progress updates.

4. Minimal v1 feature set
- Cameras: perspective
- Geometry: triangles/spheres (or triangles only initially)
- Materials: lambert/phong/emissive approximation
- Lighting: direct only, no GI/recursion

5. GUI controls
- Add tile budget per frame.
- Add quality preset and resolution scale.
- Add explicit accumulation reset trigger.

6. Validation
- Verify interactive frame times on representative sample scenes.
- Verify no regressions in non-realtime integrators.

## Non-Goals (v1)

- Full path tracing parity
- Perfect material equivalence with offline integrators
- Embree integration in realtime path

