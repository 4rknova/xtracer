/*
 * mat_dielectric.cl — Delta dielectric (glass) BSDF.
 * No NEE: this is a specular delta material; shadow rays are never cast.
 */

/* Exact Fresnel for a dielectric interface (unpolarised). Returns 1.0 on TIR. */
static float fresnel_dielectric(float cos_i, float eta_i, float eta_t)
{
    float sin_t2 = (eta_i * eta_i) * (1.f - cos_i * cos_i) / (eta_t * eta_t);
    if (sin_t2 >= 1.f) return 1.f;   /* total internal reflection */
    float cos_t = sqrt(1.f - sin_t2);
    float r_parl = ((eta_t * cos_i) - (eta_i * cos_t))
                 / ((eta_t * cos_i) + (eta_i * cos_t));
    float r_perp = ((eta_i * cos_i) - (eta_t * cos_t))
                 / ((eta_i * cos_i) + (eta_t * cos_t));
    return 0.5f * (r_parl * r_parl + r_perp * r_perp);
}

static void shade_dielectric(
    __global const gpu_material_t *mat,
    hit_t hit,
    float3 *throughput,
    float3 *ro, float3 *rd,
    float  *prev_bsdf_pdf, bool *prev_delta,
    pcg32_t *rng)
{
    float3 n    = hit.normal;
    float  ior  = mat->ior > 0.f ? mat->ior : 1.5f;
    /* transparency stored in roughness field (unused for delta dielectric) */
    float  transparency = mat->roughness > 0.f ? mat->roughness : 1.f;

    bool   into  = dot(*rd, n) < 0.f;
    float3 nn    = into ? n : -n;
    float  eta_i = into ? 1.f : ior;
    float  eta_t = into ? ior : 1.f;
    float  cos_i = -dot(*rd, nn);
    float  fr    = fresnel_dielectric(cos_i, eta_i, eta_t);

    float3 new_dir;
    if (pcg32_f(rng) < fr) {
        new_dir = f3reflect(*rd, nn);
        *ro = hit.pos + nn * 0.001f;
    } else {
        float  eta  = eta_i / eta_t;
        float3 refr = f3refract(*rd, nn, eta);
        if (length(refr) < 0.5f) {
            /* TIR fallback */
            new_dir = f3reflect(*rd, nn);
            *ro = hit.pos + nn * 0.001f;
        } else {
            new_dir     = f3norm(refr);
            *ro         = hit.pos - nn * 0.001f;
            *throughput *= transparency;
        }
    }
    *rd            = new_dir;
    *prev_delta    = true;
    *prev_bsdf_pdf = 0.f;
}
