/*
 * mat_thin_translucent.cl — Thin translucent (two-lobe Lambertian) BSDF.
 *
 * Stochastic reflect / transmit based on a translucency scalar.
 * Reflected lobe:  base_color  × (1 − translucency) / π  (cosine-weighted)
 * Transmitted lobe: trans_color × wrap(−n·wi) / π × thickness_tint  (cosine-weighted below surface)
 *
 * wrap(v)  = clamp((v + 0.35) / 1.35, 0, 1)  — backs-cattered wrap term (CPU parity)
 * thickness_tint = 0.25 + 0.75 × exp(−thickness × path_scale × 0.7)
 *
 * Both lobes are cosine-hemisphere ⇒ prev_delta = false; NEE is active.
 *
 * Field mapping in gpu_material_t:
 *   albedo      — base color (reflected; MAT_SAMPLER_BASE_COLOR / DIFFUSE fallback)
 *   albedo_tex  — base color texture (−1 = use albedo scalar)
 *   metallic / anisotropy / anisotropy_rotation — translucency_color RGB
 *                 (MAT_SAMPLER_TRANSLUCENCY_COLOR / TRANSMISSION; falls back to base if absent)
 *   metallic_tex — translucency_color texture (−1 = use metallic/anisotropy/anisotropy_rotation)
 *   roughness   — translucency scalar [0, 1]  (probability of transmitting)
 *   ior         — thickness (≥ 0; 0 = no Beer-Lambert attenuation)
 *   normal_tex  — normal map (−1 = none)
 *
 * Depends on:
 *   pathtracer.cl — cosine_sample_hemisphere(), power_heuristic(), eval_sky()
 */

#define THIN_TRANSLUCENT_EVAL(wi_, f_out_, pdf_out_)                             \
{                                                                                 \
    float _ci = dot(n, wi_);                                                      \
    if (_ci > 1e-5f) {                                                            \
        /* Reflected lobe */                                                      \
        (f_out_)   = base_col * (p_reflect * M_1_PI_F);                          \
        (pdf_out_) = p_reflect * _ci * M_1_PI_F;                                 \
    } else if (_ci < -1e-5f) {                                                    \
        /* Transmitted lobe */                                                    \
        float _nc  = -_ci;                                                        \
        float _w   = clamp((_nc + 0.35f) / 1.35f, 0.f, 1.f);                    \
        float _tt  = 1.f;                                                         \
        if (thick > 0.f) {                                                        \
            float _ps  = 0.5f / fmax(0.2f, _nc) + 0.5f / fmax(0.2f, dot(n, wo)); \
            float _att = exp(-thick * _ps * 0.7f);                               \
            _tt = 0.25f + 0.75f * _att;                                          \
        }                                                                         \
        (f_out_)   = trans_col * (p_transmit * _w * M_1_PI_F) * _tt;            \
        (pdf_out_) = p_transmit * _nc * M_1_PI_F;                                \
    } else {                                                                      \
        (f_out_)   = (float3)(0.f, 0.f, 0.f);                                    \
        (pdf_out_) = 0.f;                                                         \
    }                                                                             \
}

static void shade_thin_translucent(
    MAT_SCENE_PARAMS,
    __global const gpu_material_t *mat,
    hit_t hit,
    float3 albedo,
    float3 *colour, float3 *throughput,
    float3 *ro, float3 *rd,
    float  *prev_bsdf_pdf, bool *prev_delta,
    pcg32_t *rng)
{
    float3 n = hit.normal;
    if (dot(n, -*rd) < 0.f) n = -n;
    n = apply_normal_map(n, hit.uv, hit.pos, mat->normal_tex, tex_descs, pixels, samplers);
    float3 wo = f3norm(-(*rd));

    /* Base color (reflected). */
    float3 base_col = albedo;

    /* Translucency color (transmitted). Falls back to base if absent (metallic=0). */
    float3 trans_col;
    if (mat->metallic_tex >= 0)
        trans_col = sample_texture(mat->metallic_tex, hit.uv, tex_descs, pixels);
    else
        trans_col = (float3)(mat->metallic, mat->anisotropy, mat->anisotropy_rotation);
    /* If all zero (no sampler set), use base_col as fallback. */
    if (trans_col.x < 1e-6f && trans_col.y < 1e-6f && trans_col.z < 1e-6f)
        trans_col = base_col;

    float p_transmit = clamp(mat->roughness, 0.f, 1.f);
    float p_reflect  = 1.f - p_transmit;
    float thick      = fmax(0.f, mat->ior);

    /* ---- NEE: direct lighting ---- */
    if (header.emissive_count > 0 && header.total_select_weight > 0.f) {
        /* Weighted light selection. */
        float sel = pcg32_f(rng) * header.total_select_weight;
        float acc = 0.f;
        uint li = 0;
        for (; li < header.emissive_count - 1; ++li) {
            acc += emissives[li].select_weight;
            if (sel <= acc) break;
        }
        __global const gpu_emissive_t *light = &emissives[li];
        float p_select = light->select_weight / header.total_select_weight;

        if (light->type == GPU_EMISSIVE_SPHERE && light->area > 0.f) {
            __global const gpu_sphere_t *sp = &spheres[light->geom_index];
            float3 sc = (float3)(sp->origin.x, sp->origin.y, sp->origin.z);
            float3 ldir_raw = sc - hit.pos;
            float  dist2    = dot(ldir_raw, ldir_raw);
            float  dist     = sqrt(dist2);
            float3 ldir     = ldir_raw / dist;

            float p_nee = (p_select / light->area) * dist2 / fmax(1e-6f, fabs(dot(ldir, n)));

            float3 lro = hit.pos + ldir * 0.001f;
            hit_t  lhit = scene_intersect(lro, ldir,
                header, tlas_nodes, tlas_items, infinite,
                objects, bvh_nodes, triangles, spheres, planes, fractals);

            if (lhit.hit && lhit.mat_index >= 0 &&
                lhit.mat_index < (int)header.material_count &&
                materials[lhit.mat_index].type == GPU_MAT_EMISSIVE &&
                lhit.object_index == light->object_index)
            {
                float3 le = (float3)(
                    materials[lhit.mat_index].albedo.x,
                    materials[lhit.mat_index].albedo.y,
                    materials[lhit.mat_index].albedo.z);
                float3 f_nee; float pdf_nee;
                THIN_TRANSLUCENT_EVAL(ldir, f_nee, pdf_nee)
                if (pdf_nee > 1e-7f) {
                    float w = power_heuristic(p_nee, pdf_nee);
                    *colour += *throughput * le * f_nee * (1.f / p_nee) * w;
                }
            }
        } else if (light->type == GPU_EMISSIVE_TRIANGLE && light->tri_count > 0 && light->area > 0.f) {
            uint tri_i = light->tri_base + (uint)(pcg32_f(rng) * (float)light->tri_count) % light->tri_count;
            __global const gpu_triangle_t *tri = &triangles[tri_i];
            float u1 = pcg32_f(rng), u2 = pcg32_f(rng);
            if (u1 + u2 > 1.f) { u1 = 1.f - u1; u2 = 1.f - u2; }
            float3 lp = (float3)(
                tri->v0.x + u1*(tri->v1.x-tri->v0.x) + u2*(tri->v2.x-tri->v0.x),
                tri->v0.y + u1*(tri->v1.y-tri->v0.y) + u2*(tri->v2.y-tri->v0.y),
                tri->v0.z + u1*(tri->v1.z-tri->v0.z) + u2*(tri->v2.z-tri->v0.z));
            float3 ldir_raw = lp - hit.pos;
            float  dist2    = dot(ldir_raw, ldir_raw);
            float  dist     = sqrt(dist2);
            float3 ldir     = ldir_raw / dist;

            float3 tn = f3norm((float3)(
                tri->n0.x+tri->n1.x+tri->n2.x,
                tri->n0.y+tri->n1.y+tri->n2.y,
                tri->n0.z+tri->n1.z+tri->n2.z));
            float cos_l = fabs(dot(-ldir, tn));
            if (cos_l > 1e-6f) {
                float tri_area = light->area / (float)light->tri_count;
                float p_nee    = (p_select / light->area) * dist2 / cos_l;

                float3 lro = hit.pos + ldir * 0.001f;
                hit_t  lhit = scene_intersect(lro, ldir,
                    header, tlas_nodes, tlas_items, infinite,
                    objects, bvh_nodes, triangles, spheres, planes, fractals);
                (void)tri_area;

                if (lhit.hit && lhit.mat_index >= 0 &&
                    lhit.mat_index < (int)header.material_count &&
                    materials[lhit.mat_index].type == GPU_MAT_EMISSIVE &&
                    lhit.object_index == light->object_index)
                {
                    float3 le = (float3)(
                        materials[lhit.mat_index].albedo.x,
                        materials[lhit.mat_index].albedo.y,
                        materials[lhit.mat_index].albedo.z);
                    float3 f_nee; float pdf_nee;
                    THIN_TRANSLUCENT_EVAL(ldir, f_nee, pdf_nee)
                    if (pdf_nee > 1e-7f) {
                        float w = power_heuristic(p_nee, pdf_nee);
                        *colour += *throughput * le * f_nee * (1.f / p_nee) * w;
                    }
                }
            }
        }
    }

    /* NEE sky (uniform hemisphere — same 0.5 MIS weight as Lambert). */
    {
        float3 sky_wi;
        float  sky_pdf_cos;
        bool reflect_sky = (pcg32_f(rng) < p_reflect);
        if (reflect_sky)
            sky_wi = cosine_sample_hemisphere(n, &sky_pdf_cos);
        else
            sky_wi = cosine_sample_hemisphere(-n, &sky_pdf_cos);

        float3 sky_ro = hit.pos + sky_wi * 0.001f;
        hit_t  sky_hit = scene_intersect(sky_ro, sky_wi,
            header, tlas_nodes, tlas_items, infinite,
            objects, bvh_nodes, triangles, spheres, planes, fractals);
        if (!sky_hit.hit) {
            float3 le = eval_sky(sky_wi, header);
            float3 f_sky; float pdf_sky;
            THIN_TRANSLUCENT_EVAL(sky_wi, f_sky, pdf_sky)
            if (pdf_sky > 1e-7f)
                *colour += *throughput * le * f_sky * 0.5f;
        }
    }

    /* ---- Sample BSDF direction ---- */
    float3 new_dir;
    float  bsdf_pdf;
    if (pcg32_f(rng) < p_reflect) {
        float tmp_pdf;
        new_dir = cosine_sample_hemisphere(n, &tmp_pdf);
    } else {
        float tmp_pdf;
        new_dir = cosine_sample_hemisphere(-n, &tmp_pdf);
    }

    float3 f_bsdf;
    THIN_TRANSLUCENT_EVAL(new_dir, f_bsdf, bsdf_pdf)

    if (bsdf_pdf < 1e-7f) {
        *prev_delta = false;
        *prev_bsdf_pdf = 0.f;
        return;
    }

    *throughput   *= f_bsdf / bsdf_pdf;
    *ro            = hit.pos + new_dir * 0.001f;
    *rd            = new_dir;
    *prev_delta    = false;
    *prev_bsdf_pdf = bsdf_pdf;
}

#undef THIN_TRANSLUCENT_EVAL
