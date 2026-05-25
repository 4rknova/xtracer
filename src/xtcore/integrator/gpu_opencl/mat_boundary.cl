/*
 * mat_boundary.cl — Boundary (medium interface) material.
 *
 * Fully transparent pass-through: advances the ray origin across the surface
 * without deflecting it or modifying throughput. Required as a volume boundary
 * marker for participating media (not yet implemented on GPU).
 *
 * Treated as delta (prev_delta = true) — no NEE, consistent with
 * CPU Boundary::bsdf_is_delta() returning true.
 */

static void shade_boundary(
    __global const gpu_material_t *mat,
    hit_t hit,
    float3 *throughput,
    float3 *ro, float3 *rd,
    float  *prev_bsdf_pdf, bool *prev_delta)
{
    (void)mat;
    (void)throughput;
    /* Step the ray origin just past the surface, keeping direction unchanged. */
    *ro = hit.pos + (*rd) * 0.001f;
    /* rd unchanged */
    *prev_delta    = true;
    *prev_bsdf_pdf = 0.f;
}
