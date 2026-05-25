/*
 * mat_sheen.cl — Sheen BSDF.
 *
 * Cosine-hemisphere sampled Lambertian base with a retro-reflective Schlick
 * grazing boost, matching CPU Sheen::bsdf_eval exactly:
 *
 *   h     = normalize(wi + wo)
 *   fh    = (1 - dot(wi, h))^5
 *   retro = (0.25 + 0.75 * fh) * sheen_strength
 *   f     = (base_color + sheen_color * retro) / π
 *   pdf   = cos_i / π
 *
 * Field mapping in gpu_material_t:
 *   albedo / albedo_tex                         — base color
 *   metallic / anisotropy / anisotropy_rotation — sheen color (RGB packed)
 *                                                 (defaults to base_color when
 *                                                  MAT_SAMPLER_SHEEN_COLOR absent)
 *   metallic_tex                                — sheen color texture (−1 = constant)
 *   roughness                                   — sheen strength [0, 1]
 *   normal_tex                                  — normal map (−1 = none)
 */

static void shade_sheen(
    MAT_SCENE_PARAMS,
    __global const gpu_material_t *mat,
    hit_t hit, float3 base,
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

    float3 sheen_col = (float3)(mat->metallic, mat->anisotropy, mat->anisotropy_rotation);
    if (mat->metallic_tex >= 0)
        sheen_col = sample_texture(mat->metallic_tex, hit.uv, tex_descs, pixels);

    float sheen = clamp(mat->roughness, 0.f, 1.f);

    /* Inline BSDF eval: f = (base + sheen_col * retro) / π, pdf = cos_i / π */
    #define SHEEN_EVAL(wi_, f_out_, pdf_out_) {                          \
        float  cos_i_  = fmax(0.f, dot(wi_, n));                        \
        float3 hh_     = f3norm(wi_ + wo);                              \
        float  wih_    = fmax(0.f, dot(wi_, hh_));                      \
        float  fh_     = pow(fmax(0.f, 1.f - wih_), 5.f);              \
        float  retro_  = (0.25f + 0.75f * fh_) * sheen;                \
        f_out_   = (base + sheen_col * retro_) * (1.f / 3.14159265359f);\
        pdf_out_ = cos_i_ / 3.14159265359f;                             \
    }

    /* ---- NEE: area lights ---- */
    if (header.emissive_count > 0 && header.total_select_weight > 0.f) {
        light_sample_t ls = sample_area_light(shadow_orig, header,
            emissives, spheres, triangles, materials, rng);
        if (ls.valid) {
            float cos_s = dot(ls.wi, n);
            if (cos_s > 0.f) {
                bool occ = shadow_occluded_tmax(shadow_orig, ls.wi, ls.dist - 0.01f,
                               header, tlas_nodes, tlas_items, infinite,
                               objects, bvh_nodes, triangles, spheres, planes, fractals);
                if (!occ) {
                    float3 bsdf_f; float bsdf_pdf;
                    SHEEN_EVAL(ls.wi, bsdf_f, bsdf_pdf)
                    float w_nee = power_heuristic(ls.p_nee, bsdf_pdf);
                    *colour += *throughput * bsdf_f * ls.le * (cos_s / ls.p_nee) * w_nee;
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
                SHEEN_EVAL(wi_sky, bsdf_f, bsdf_pdf)
                float mis_w = power_heuristic(p_sky, bsdf_pdf);
                *colour += *throughput * bsdf_f * eval_sky(wi_sky, header)
                         * (dot(n, wi_sky) / p_sky) * mis_w;
            }
        }
    }

    /* ---- BSDF continuation ---- */
    float3 wi_cont = cosine_sample_hemisphere(n, rng);
    float  cos_cont = fmax(0.f, dot(wi_cont, n));
    float3 f_cont; float pdf_cont;
    SHEEN_EVAL(wi_cont, f_cont, pdf_cont)

    #undef SHEEN_EVAL

    if (pdf_cont < 1e-7f || cos_cont <= 0.f) return;

    *prev_bsdf_pdf = pdf_cont;
    *throughput   *= f_cont * (cos_cont / pdf_cont);
    *ro            = shadow_orig;
    *rd            = wi_cont;
    *prev_delta    = false;
}
