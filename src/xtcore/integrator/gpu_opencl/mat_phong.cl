/*
 * mat_phong.cl — Phong / Blinn-Phong BSDF (path tracing variant).
 *
 * Both GPU_MAT_PHONG and GPU_MAT_BLINNPHONG dispatch here.  In the CPU
 * integrator, bsdf_eval/bsdf_sample for BlinnPhong is identical to Phong
 * (power cosine lobe about reflect(wo, n)); the "Blinn" difference only
 * appears in the legacy direct shade() path which is not used in path tracing.
 *
 * Field mapping in gpu_material_t:
 *   albedo / albedo_tex                        — kd (diffuse color / texture)
 *   metallic / anisotropy / anisotropy_rotation — ks (specular color, RGB packed)
 *   metallic_tex                               — ks texture (−1 = use constant)
 *   roughness                                  — Phong exponent (1 … ∞)
 *   clearcoat                                  — reflectance scalar [0,1]
 *                                                (0 = derive from kd/ks luma ratio)
 *   normal_tex                                 — normal map (−1 = none)
 *
 * Supports NEE (area lights + sky) with MIS power heuristic, matching
 * the CPU bsdf_eval formula.
 */

static void shade_phong(
    MAT_SCENE_PARAMS,
    __global const gpu_material_t *mat,
    hit_t hit, float3 kd,
    float3 *colour, float3 *throughput,
    float3 *ro, float3 *rd,
    float  *prev_bsdf_pdf, bool *prev_delta,
    pcg32_t *rng)
{
    float3 n = hit.normal;
    if (dot(n, -*rd) < 0.f) n = -n;
    n = apply_normal_map(n, hit.uv, hit.pos, mat->normal_tex, tex_descs, pixels, samplers);
    float3 shadow_orig = hit.pos + n * 0.001f;
    float3 wo = f3norm(-*rd);

    /* Specular color (packed into metallic/anisotropy/anisotropy_rotation). */
    float3 ks = (float3)(mat->metallic, mat->anisotropy, mat->anisotropy_rotation);
    if (mat->metallic_tex >= 0)
        ks = sample_texture(mat->metallic_tex, hit.uv, tex_descs, pixels);

    float exponent    = fmax(1.f, mat->roughness);
    float reflectance = mat->clearcoat;

    /* Lobe selection weights — mirror CPU lobe_weights(). */
    float ld  = dot(kd, (float3)(0.299f, 0.587f, 0.114f));
    float ls  = dot(ks, (float3)(0.299f, 0.587f, 0.114f));
    float w_diff, w_spec;
    if (reflectance > 1e-4f && reflectance < (1.f - 1e-4f)) {
        w_spec = reflectance;
        w_diff = 1.f - reflectance;
    } else {
        float sum = ld + ls;
        if (sum < 1e-6f) { w_diff = 1.f; w_spec = 0.f; }
        else { w_diff = ld / sum; w_spec = ls / sum; }
    }

    float3 r_refl     = f3norm(2.f * dot(wo, n) * n - wo);
    float  brdf_norm  = (exponent + 2.f) / (2.f * 3.14159265359f);
    float  pdf_norm   = (exponent + 1.f) / (2.f * 3.14159265359f);

    /* Helper: combined BSDF value and pdf for a given wi. */
    #define PHONG_EVAL(wi_, f_out_, pdf_out_) { \
        float cos_i_ = fmax(0.f, dot(wi_, n)); \
        float ca_    = fmax(0.f, dot(r_refl, wi_)); \
        float3 f_d_  = kd * (1.f / 3.14159265359f); \
        float  p_ca_ = (ca_ > 1e-6f) ? pow(ca_, exponent) : 0.f; \
        float3 f_s_  = ks * (brdf_norm * p_ca_); \
        f_out_   = f_d_ * w_diff + f_s_ * w_spec; \
        pdf_out_ = w_diff * (cos_i_ / 3.14159265359f) + w_spec * (pdf_norm * p_ca_); \
    }

    /* ---- NEE: area lights ---- */
    if (header.emissive_count > 0 && header.total_select_weight > 0.f) {
        light_sample_t ls_nee = sample_area_light(shadow_orig, header,
            emissives, spheres, triangles, materials, rng);
        if (ls_nee.valid) {
            float cos_s = dot(ls_nee.wi, n);
            if (cos_s > 0.f) {
                bool occ = shadow_occluded_tmax(shadow_orig, ls_nee.wi, ls_nee.dist - 0.01f,
                               header, tlas_nodes, tlas_items, infinite,
                               objects, bvh_nodes, triangles, spheres, planes, fractals);
                if (!occ) {
                    float3 bsdf_f; float bsdf_pdf;
                    PHONG_EVAL(ls_nee.wi, bsdf_f, bsdf_pdf)
                    float w_nee = power_heuristic(ls_nee.p_nee, bsdf_pdf);
                    *colour += *throughput * bsdf_f * ls_nee.le * (cos_s / ls_nee.p_nee) * w_nee;
                }
            }
        }
    }

    /* ---- NEE: sky ---- */
    {
        float3 wi_sky = cosine_sample_hemisphere(n, rng);
        float  p_sky  = fmax(0.f, dot(n, wi_sky)) / 3.14159265359f;
        if (p_sky > 1e-6f) {
            bool sky_occ = shadow_occluded(shadow_orig, wi_sky,
                               header, tlas_nodes, tlas_items, infinite,
                               objects, bvh_nodes, triangles, spheres, planes, fractals);
            if (!sky_occ) {
                float3 bsdf_f; float bsdf_pdf;
                PHONG_EVAL(wi_sky, bsdf_f, bsdf_pdf)
                float mis_w = power_heuristic(p_sky, bsdf_pdf);
                *colour += *throughput * bsdf_f * eval_sky(wi_sky, header)
                         * (dot(n, wi_sky) / p_sky) * mis_w;
            }
        }
    }

    /* ---- BSDF continuation ---- */
    float3 wi_cont;
    if (w_spec > 1e-4f && pcg32_f(rng) < w_spec)
        wi_cont = power_cosine_sample(r_refl, exponent, rng);
    else
        wi_cont = cosine_sample_hemisphere(n, rng);

    float cos_cont = fmax(0.f, dot(wi_cont, n));
    float3 f_cont; float pdf_cont;
    PHONG_EVAL(wi_cont, f_cont, pdf_cont)

    #undef PHONG_EVAL

    if (pdf_cont < 1e-7f || cos_cont <= 0.f) return;

    *prev_bsdf_pdf = pdf_cont;
    *throughput   *= f_cont * (cos_cont / pdf_cont);
    *ro            = shadow_orig;
    *rd            = wi_cont;
    *prev_delta    = false;
}
