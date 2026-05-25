/*
 * mat_thin_dielectric.cl — Thin dielectric (flat-slab) BSDF.
 *
 * Fresnel reflects or passes the ray straight through (no refraction bending).
 * An optional roughness parameter perturbs both lobes via a power cosine lobe,
 * matching CPU ThinDielectric::sample_path() behaviour.
 *
 * Treated as quasi-delta (prev_delta = true) — no NEE, consistent with
 * CPU bsdf_is_delta() always returning true.
 *
 * Field mapping in gpu_material_t:
 *   albedo      — transmission tint (MAT_SAMPLER_TRANSMISSION / DIFFUSE fallback)
 *   ior         — index of refraction
 *   roughness   — surface roughness [0, 1] (0..0.02 = delta, >0.02 = rough)
 *   metallic    — transparency scalar [0, 1]
 *   normal_tex  — normal map (−1 = none)
 *
 * Depends on:
 *   mat_dielectric.cl — fresnel_dielectric()
 *   pathtracer.cl     — power_cosine_sample()
 */

/* Maps roughness to Phong exponent: max(2, 2/r^2 - 2) — mirrors CPU helper. */
static float thin_dielectric_exponent(float roughness)
{
    float r = clamp(roughness, 0.02f, 1.f);
    return fmax(2.f, 2.f / (r * r) - 2.f);
}

static void shade_thin_dielectric(
    MAT_SCENE_PARAMS,
    __global const gpu_material_t *mat,
    hit_t hit,
    float3 *throughput,
    float3 *ro, float3 *rd,
    float  *prev_bsdf_pdf, bool *prev_delta,
    pcg32_t *rng)
{
    float3 n = hit.normal;
    if (dot(n, -*rd) < 0.f) n = -n;
    n = apply_normal_map(n, hit.uv, hit.pos, mat->normal_tex, tex_descs, pixels, samplers);
    float3 wo = f3norm(-(*rd));

    float  ior          = mat->ior > 0.f ? mat->ior : 1.5f;
    float  roughness    = clamp(mat->roughness, 0.f, 1.f);
    float  transparency = mat->metallic > 0.f ? mat->metallic : 1.f;
    float3 tint         = (float3)(mat->albedo.x, mat->albedo.y, mat->albedo.z);

    float fr        = fresnel_dielectric(fmax(0.f, dot(n, wo)), 1.f, ior);
    float p_reflect = clamp(fr, 0.02f, 0.98f);
    float p_transmit = 1.f - p_reflect;

    bool  is_delta = (roughness <= 0.02f);
    float exponent = thin_dielectric_exponent(roughness);

    float3 new_dir;
    if (pcg32_f(rng) < p_reflect) {
        /* Reflect */
        float3 r_axis = f3norm(2.f * dot(wo, n) * n - wo);
        new_dir = is_delta ? r_axis : f3norm(power_cosine_sample(r_axis, exponent, rng));
        *throughput *= fr / p_reflect;
        *ro = hit.pos + n * 0.001f;
    } else {
        /* Transmit straight through (thin slab — no bending). */
        float3 t_axis = f3norm(*rd);
        new_dir = is_delta ? t_axis : f3norm(power_cosine_sample(t_axis, exponent, rng));
        *throughput *= tint * ((1.f - fr) / p_transmit) * transparency;
        *ro = hit.pos + new_dir * 0.001f;
    }

    *rd            = new_dir;
    *prev_delta    = true;   /* quasi-delta: skip NEE at this bounce */
    *prev_bsdf_pdf = 0.f;
}
