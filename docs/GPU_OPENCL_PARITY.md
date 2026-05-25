# GPU OpenCL Integrator — Feature Parity Tracker

Tracks progress toward 1:1 parity between `gpu_opencl` and `pathtracer_mis`.
Parity means both integrators accept the same scene and produce the same image (within noise).

Legend: ✅ done · 🚧 partial · ❌ missing

---

## Geometry

### Analytic Primitives

| Primitive | `pathtracer_mis` | `gpu_opencl` | Notes |
|-----------|:---:|:---:|-------|
| Sphere | ✅ | ✅ | |
| Plane (infinite) | ✅ | ✅ | |
| Triangle (standalone) | ✅ | ✅ | Serialized as single-leaf mesh |
| Point | ✅ | ✅ | Parser emits a Sphere with radius=ε; handled by sphere path |

### Mesh

Procedural mesh generators (listed below) all tessellate into triangle meshes at load time — parity reduces to whether the mesh primitive itself is supported.

| Primitive | `pathtracer_mis` | `gpu_opencl` | Notes |
|-----------|:---:|:---:|-------|
| Mesh (OBJ / BVH) | ✅ | ✅ | |
| Meshgroup | ✅ | ✅ | Resolves to mesh |
| Icosahedron | ✅ | ✅ | Generates mesh |
| Tetrahedron | ✅ | ✅ | Generates mesh |
| Hexahedron (Cube) | ✅ | ✅ | Generates mesh |
| Octahedron | ✅ | ✅ | Generates mesh |
| Dodecahedron | ✅ | ✅ | Generates mesh |
| Pyramid | ✅ | ✅ | Generates mesh |
| Capsule | ✅ | ✅ | Generates mesh |
| Cylinder | ✅ | ✅ | Generates mesh |
| Capped Cylinder | ✅ | ✅ | Generates mesh |
| Cone | ✅ | ✅ | Generates mesh |
| Truncated Cone | ✅ | ✅ | Generates mesh |
| Ring / Torus | ✅ | ✅ | Generates mesh |
| Rounded Ring | ✅ | ✅ | Generates mesh |
| Torus Knot | ✅ | ✅ | Generates mesh |
| Icosphere | ✅ | ✅ | Generates mesh |
| Geodesic Dome | ✅ | ✅ | Generates mesh |
| Icosahedral Cage | ✅ | ✅ | Generates mesh |
| Menger Sponge (mesh) | ✅ | ✅ | Generates mesh |
| Sierpinski Tetrahedron (mesh) | ✅ | ✅ | Generates mesh |
| Möbius Strip | ✅ | ✅ | Generates mesh |
| Klein Bottle | ✅ | ✅ | Generates mesh |
| Hairball | ✅ | ✅ | Generates mesh |
| Shell Spiral | ✅ | ✅ | Generates mesh |
| Rock | ✅ | ✅ | Generates mesh |
| Terrain | ✅ | ✅ | Generates mesh |
| Draped Cloth Strip | ✅ | ✅ | Generates mesh |
| SVG Silhouette | ✅ | ✅ | Generates mesh |
| Chain Link | ✅ | ✅ | Generates mesh |
| Tube Curve | ✅ | ✅ | Generates mesh |
| Lathe | ✅ | ✅ | Generates mesh |
| Snowflake (Koch) | ✅ | ✅ | Generates mesh |
| Gear | ✅ | ✅ | Generates mesh |
| Spring | ✅ | ✅ | Generates mesh |
| Hemisphere | ✅ | ✅ | Generates mesh |
| Disc | ✅ | ✅ | Generates mesh |
| Star | ✅ | ✅ | Generates mesh |
| Superellipsoid | ✅ | ✅ | Generates mesh |
| Crystal | ✅ | ✅ | Generates mesh |
| Tree | ✅ | ✅ | Generates mesh |
| Coral | ✅ | ✅ | Generates mesh |
| City | ✅ | ✅ | Generates mesh |
| Lowpoly Terrain | ✅ | ✅ | Generates mesh |
| Displaced Sphere | ✅ | ✅ | Generates mesh |

### Fractal / Implicit Surfaces (SDE)

| Fractal | `pathtracer_mis` | `gpu_opencl` | Notes |
|---------|:---:|:---:|-------|
| Mandelbulb | ✅ | ✅ | |
| Julia (3D) | ✅ | ✅ | |
| MandelBox | ✅ | ✅ | |
| Quaternion Julia | ✅ | ✅ | |
| Menger Sponge (implicit) | ✅ | ✅ | |
| Sierpinski Tetrahedron (implicit) | ✅ | ✅ | |
| Burning Ship 3D | ✅ | ✅ | |
| Cantor Dust 3D | ✅ | ✅ | |
| Icosahedral IFS | ✅ | ✅ | |

### CSG (Constructive Solid Geometry)

| Operation | `pathtracer_mis` | `gpu_opencl` | Notes |
|-----------|:---:|:---:|-------|
| Union | ✅ | ❌ | |
| Soft union (smooth blend) | ✅ | ❌ | |
| Intersection | ✅ | ❌ | |
| Difference | ✅ | ❌ | |

---

## Materials / BSDFs

| Material | `pathtracer_mis` | `gpu_opencl` | Notes |
|----------|:---:|:---:|-------|
| Emissive | ✅ | ✅ | |
| Lambert (diffuse) | ✅ | ✅ | |
| Dielectric (delta) | ✅ | ✅ | Exact Fresnel; `transparency` applied to refracted throughput; IOR stack not implemented (exit always η=1) |
| Rough Dielectric | ✅ | ✅ | NDF half-vector sampling; exact Fresnel; Smith G1 weights; no Beer-Lambert absorption; IOR stack not implemented (exit η=1) |
| Principled | ✅ | ✅ | Full anisotropic GGX + clearcoat; albedo/normal/roughness/metallic textures |
| Phong | ✅ | ✅ | Power cosine lobe about reflect(wo,n); NEE + MIS; diffuse/specular/normal textures |
| Blinn-Phong | ✅ | ✅ | Same shader as Phong (bsdf_eval/sample are identical in path tracing) |
| Subsurface | ✅ | ✅ | Lambertian reflect + Beer-Lambert transmit; aliased into Principled fields |
| Sheen | ✅ | ✅ | Cosine-hemisphere sampled; Schlick grazing boost; base/sheen color textures; normal map |
| Thin Dielectric | ✅ | ✅ | Fresnel reflect or straight-through transmit; optional roughness perturbation via power cosine |
| Thin Translucent | ✅ | ✅ | Two-lobe cosine Lambertian; wrap term + Beer-Lambert thickness tint on transmit lobe; NEE active |
| Boundary | ✅ | ✅ | Pass-through; advances ray origin across surface; no throughput change; delta (no NEE) |

---

## Light Sources

| Light type | `pathtracer_mis` | `gpu_opencl` | Notes |
|------------|:---:|:---:|-------|
| Emissive sphere (NEE) | ✅ | ✅ | |
| Emissive triangle (NEE) | ✅ | ✅ | |
| Emissive mesh (NEE, weighted) | ✅ | ✅ | Area-weighted triangle selection; area×luminance light selection |
| Environment — gradient | ✅ | ✅ | |
| Environment — ERP texture | ✅ | ❌ | |
| Environment — cubemap | ✅ | ❌ | |
| Environment — Rayleigh sky | ✅ | ❌ | |
| Environment importance sampling | ✅ | ❌ | Required for IS on env maps |

---

## Sampling

| Strategy | `pathtracer_mis` | `gpu_opencl` | Notes |
|----------|:---:|:---:|-------|
| BSDF sampling | ✅ | ✅ | |
| NEE / direct light sampling | ✅ | ✅ | Spheres only in GPU |
| MIS power heuristic (β=2) | ✅ | ✅ | |
| Russian Roulette (after 3 bounces) | ✅ | ✅ | |
| Stratified pixel sampling | ✅ | ✅ | |
| Cone sampling (sphere lights) | ✅ | ❌ | Solid-angle sampling; more efficient than area |

---

## Camera / Lens

| Feature | `pathtracer_mis` | `gpu_opencl` | Notes |
|---------|:---:|:---:|-------|
| Pinhole | ✅ | ✅ | |
| Depth of Field (thin lens) | ✅ | ✅ | Disk aperture; same math as CPU `get_primary_ray()` |
| Aperture blades (polygon bokeh) | ✅ | ❌ | |

---

## Participating Media

| Feature | `pathtracer_mis` | `gpu_opencl` | Notes |
|---------|:---:|:---:|-------|
| Per-object interior medium | ✅ | ❌ | |
| Henyey-Greenstein phase function | ✅ | ❌ | |
| Volumetric absorption | ✅ | ❌ | |
| Volumetric emission | ✅ | ❌ | |

---

## Texturing

| Feature | `pathtracer_mis` | `gpu_opencl` | Notes |
|---------|:---:|:---:|-------|
| File textures (albedo) | ✅ | ✅ | Bilinear filtering; Lambert + Principled |
| Normal maps | ✅ | ✅ | Tangent-space decode + TBN rotation; Lambert + Principled |
| Texture-driven material parameters | ✅ | ✅ | roughness, metallic (luminance) for Principled |

### Procedural Samplers

Native OpenCL implementations; evaluated on-the-fly per ray (not baked).

| Sampler | `pathtracer_mis` | `gpu_opencl` | Notes |
|---------|:---:|:---:|-------|
| SolidColor | ✅ | ✅ | Stored inline as material albedo; no `gpu_sampler_t` entry |
| Texture2D | ✅ | ✅ | `GPU_SAMPLER_TEXTURE` wraps `tex_descs[]` bilinear path |
| Checker | ✅ | ✅ | |
| Brick | ✅ | ✅ | |
| Dots | ✅ | ✅ | |
| GraphPaper | ✅ | ✅ | |
| Stars | ✅ | ✅ | Planar UV; grid_scale=256 |
| Weave | ✅ | ✅ | |
| VoronoiNormal | ✅ | ✅ | |
| FBMMarble | ✅ | ✅ | Value noise + FBM |
| FBMWood | ✅ | ✅ | Value noise + FBM |
| CurlNoise | ✅ | ✅ | Numerical curl via FBM finite differences |
| Scratches | ✅ | ✅ | |
| EdgeWear | ✅ | ✅ | |
| Gradient | ✅ | ✅ | Maps `uv.y` |
| Blend | ✅ | ✅ | Composite; calls `eval_sampler_leaf()` for children (depth-capped) |
| MixMasked | ✅ | ✅ | Composite; calls `eval_sampler_leaf()` for children (depth-capped) |
| Triplanar | ✅ | ✅ | Composite; projects on YZ/XZ/XY planes weighted by normal |
