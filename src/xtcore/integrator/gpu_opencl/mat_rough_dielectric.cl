/*
 * mat_rough_dielectric.cl — Rough dielectric (GGX microfacet) BSDF.
 *
 * Uses VNDF (Visible Normal Distribution Function) half-vector sampling,
 * matching CPU RoughDielectric::sample_path() exactly.
 * Weight formula: F * G1(wi) / p_lobe  — correct for VNDF sampling.
 * BSDF PDF:  p_reflect * D * G1(wo) / (4 * cos_o)  — VNDF reflected PDF.
 *
 * Heitz 2018: "Sampling the GGX Distribution of Visible Normals."
 *
 * Field mapping in gpu_material_t:
 *   ior         — index of refraction
 *   roughness   — surface roughness [0.02, 1]
 *   albedo      — transmission tint (constant or from albedo_tex)
 *   metallic    — transparency scalar [0, 1]
 *   normal_tex  — normal map (−1 = none)
 *   roughness_tex — roughness map (−1 = use scalar)
 *   albedo_tex  — transmission tint texture (−1 = use albedo constant)
 *
 * Depends on:
 *   mat_dielectric.cl  — fresnel_dielectric()
 *   mat_principled.cl  — ggx_ndf_iso(), smith_g1_iso(), tangent_basis()
 */

/* Isotropic GGX VNDF half-vector sampling (Heitz 2018).
 * Guarantees dot(wo, h) > 0, so reflected directions are always above the horizon.
 * The PDF for the reflected direction wi is: p(wi) = D * G1(wo) / (4 * cos_o). */
static float3 sample_ggx_h_vndf(float3 n, float3 wo, float alpha, pcg32_t *rng)
{
    float u1 = pcg32_f(rng), u2 = pcg32_f(rng);

    /* Build local tangent frame aligned with the surface normal. */
    float3 t, b;
    tangent_basis(n, &t, &b);

    /* Transform wo to local frame and stretch by alpha (isotropic). */
    float3 wo_l = (float3)(dot(wo, t), dot(wo, b), dot(wo, n));
    float3 wo_h = f3norm((float3)(alpha * wo_l.x, alpha * wo_l.y, wo_l.z));

    /* Orthonormal basis T1, T2 around the stretched wo_h vector. */
    float3 T1 = (wo_h.z < 0.9999f) ? f3norm(cross(wo_h, (float3)(0.f, 0.f, 1.f)))
                                     : (float3)(1.f, 0.f, 0.f);
    float3 T2 = cross(T1, wo_h);

    /* Sample a point on the disk, warped onto the projected hemispherical cap. */
    float a   = 1.f / (1.f + wo_h.z);
    float r   = sqrt(u1);
    float phi = (u2 < a) ? (u2 / a * 3.14159265359f)
                         : (3.14159265359f + (u2 - a) / fmax(1e-7f, 1.f - a) * 3.14159265359f);
    float p1  = r * cos(phi);
    float p2  = r * sin(phi) * ((u2 < a) ? 1.f : wo_h.z);

    /* Normal in stretched (hemisphere) space; unstretched back to ellipsoid. */
    float3 nh  = p1 * T1 + p2 * T2 + sqrt(fmax(0.f, 1.f - p1*p1 - p2*p2)) * wo_h;
    float3 h_l = f3norm((float3)(alpha * nh.x, alpha * nh.y, fmax(1e-7f, nh.z)));

    /* Return half-vector in world space. */
    return f3norm(t * h_l.x + b * h_l.y + n * h_l.z);
}

static void shade_rough_dielectric(
    MAT_SCENE_PARAMS,
    __global const gpu_material_t *mat,
    hit_t hit,
    float3 *throughput,
    float3 *ro, float3 *rd,
    float  *prev_bsdf_pdf, bool *prev_delta,
    pcg32_t *rng)
{
    float  ior   = mat->ior > 0.f ? mat->ior : 1.5f;
    float  rough = clamp(mat->roughness, 0.02f, 1.f);
    /* transparency in metallic field; transmission tint in albedo */
    float  transparency = mat->metallic > 0.f ? mat->metallic : 1.f;
    float3 tint = (float3)(mat->albedo.x, mat->albedo.y, mat->albedo.z);

    /* Entering vs exiting: check geometric normal before normal map. */
    bool   into  = dot(*rd, hit.normal) < 0.f;
    float  eta_i = into ? 1.f : ior;
    float  eta_t = into ? ior : 1.f;

    /* Shading normal: flip to face wo, then apply normal map. */
    float3 nn = hit.normal;
    if (dot(nn, -*rd) < 0.f) nn = -nn;
    nn = apply_normal_map(nn, hit.uv, hit.pos, mat->normal_tex, tex_descs, pixels, samplers);

    float3 wo   = f3norm(-(*rd));
    float  cos_o = fmax(1e-5f, dot(nn, wo));

    /* Roughness texture override. */
    if (mat->roughness_tex >= 0) {
        float rv = sample_texture_luma(mat->roughness_tex, hit.uv, tex_descs, pixels);
        rough = clamp(rv, 0.02f, 1.f);
    }

    /* Transmission tint texture override. */
    if (mat->albedo_tex >= 0)
        tint = sample_texture(mat->albedo_tex, hit.uv, tex_descs, pixels);

    /* Sample VNDF half-vector — guarantees dot(wo, h) > 0. */
    float3 h        = sample_ggx_h_vndf(nn, wo, rough, rng);
    float  cos_wo_h = fmax(1e-7f, dot(wo, h));

    /* Exact Fresnel and lobe probabilities. */
    float fr        = fresnel_dielectric(cos_wo_h, eta_i, eta_t);
    float p_reflect = clamp(fr, 0.02f, 0.98f);
    float p_transmit = 1.f - p_reflect;

    /* NDF and G1(wo) for VNDF PDF:  p(wi) = D * G1(wo) / (4 * cos_o). */
    float D    = ggx_ndf_iso(nn, h, rough);
    float G1wo = smith_g1_iso(wo, nn, rough);

    float3 new_dir;
    if (pcg32_f(rng) < p_reflect) {
        /* Reflect off microfacet. */
        new_dir = f3norm(2.f * cos_wo_h * h - wo);
        if (dot(new_dir, nn) <= 0.f) {
            /* Below horizon — terminate path cleanly. */
            *throughput    = (float3)(0.f, 0.f, 0.f);
            *prev_bsdf_pdf = 1.f;
            *prev_delta    = false;
            return;
        }
        float G1wi   = smith_g1_iso(new_dir, nn, rough);
        *throughput  *= fr * G1wi / p_reflect;
        /* VNDF reflected PDF: p_reflect * D * G1(wo) / (4 * cos_o) */
        *prev_bsdf_pdf = p_reflect * D * G1wo / fmax(1e-7f, 4.f * cos_o);
        *ro = hit.pos + nn * 0.001f;
    } else {
        /* Refract through microfacet. */
        float  eta  = eta_i / eta_t;
        float3 refr = f3refract(*rd, h, eta);
        if (length(refr) < 0.5f) {
            /* TIR: fall back to reflection. */
            new_dir = f3norm(2.f * cos_wo_h * h - wo);
            if (dot(new_dir, nn) <= 0.f) {
                *throughput    = (float3)(0.f, 0.f, 0.f);
                *prev_bsdf_pdf = 1.f;
                *prev_delta    = false;
                return;
            }
            float G1wi   = smith_g1_iso(new_dir, nn, rough);
            *throughput  *= G1wi / p_reflect;
            *prev_bsdf_pdf = p_reflect * D * G1wo / fmax(1e-7f, 4.f * cos_o);
            *ro = hit.pos + nn * 0.001f;
        } else {
            new_dir = f3norm(refr);
            /* -new_dir: transmitted ray opposes nn; G1 expects direction on positive side. */
            float G1wi   = smith_g1_iso(-new_dir, nn, rough);
            *throughput  *= tint * ((1.f - fr) * G1wi / p_transmit) * transparency;
            /* Transmission PDF is complex (refraction Jacobian); 0 → MIS falls back to 0.5. */
            *prev_bsdf_pdf = 0.f;
            *ro = hit.pos - nn * 0.001f;
        }
    }
    *rd         = new_dir;
    *prev_delta = false;
}
