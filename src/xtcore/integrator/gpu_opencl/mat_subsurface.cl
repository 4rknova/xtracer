/*
 * mat_subsurface.cl — Subsurface scattering BSDF.
 * Blend of Lambertian reflection (weight 1-sss_w) and Beer-Lambert translucent
 * transmission (weight sss_w). Mirrors CPU Subsurface material.
 *
 * Field aliasing in gpu_material_t (gpu_types.h comment documents the encoding):
 *   roughness          → sss_weight
 *   ior                → thickness (mean free path)
 *   metallic           → sss_color.r
 *   anisotropy         → sss_color.g
 *   anisotropy_rotation→ sss_color.b
 *   clearcoat          → sss_radius.r
 *   clearcoat_roughness→ sss_radius.g
 *   roughness_tex      → sss_radius.b  (float bits stored in gpu_int via memcpy)
 */

static float3 subsurface_tint(float3 radius, float thickness, float cos_i, float cos_o)
{
    float path = 0.5f * (1.f / fmax(1e-4f, fabs(cos_i)) + 1.f / fmax(1e-4f, cos_o));
    return (float3)(exp(-path * thickness / fmax(1e-4f, radius.x)),
                    exp(-path * thickness / fmax(1e-4f, radius.y)),
                    exp(-path * thickness / fmax(1e-4f, radius.z)));
}

static void shade_subsurface(
    MAT_SCENE_PARAMS,
    __global const gpu_material_t *mat,
    hit_t hit, float3 albedo,
    float3 *colour, float3 *throughput,
    float3 *ro, float3 *rd,
    float  *prev_bsdf_pdf, bool *prev_delta,
    pcg32_t *rng)
{
    float3 n = hit.normal;
    if (dot(n, -*rd) < 0.f) n = -n;
    n = apply_normal_map(n, hit.uv, hit.pos, mat->normal_tex, tex_descs, pixels, samplers);
    float3 shadow_orig = hit.pos + n * 0.001f;

    float  sss_w    = mat->roughness;
    float  sss_thick = mat->ior;
    float3 sss_col  = (float3)(mat->metallic, mat->anisotropy, mat->anisotropy_rotation);
    float3 sss_rad  = (float3)(mat->clearcoat, mat->clearcoat_roughness,
                                as_float(mat->roughness_tex));

    float cos_i_in = fabs(dot(*rd, n));

    /* ---- NEE: area lights ---- */
    if (header.emissive_count > 0 && header.total_select_weight > 0.f) {
        light_sample_t ls = sample_area_light(shadow_orig, header,
            emissives, spheres, triangles, materials, rng);
        if (ls.valid) {
            float cos_s_front = fmax(0.f,  dot(ls.wi, n));
            float cos_s_back  = fmax(0.f, -dot(ls.wi, n));
            float cos_s_abs   = cos_s_front + cos_s_back;
            if (cos_s_abs > 1e-5f) {
                bool occ = shadow_occluded_tmax(shadow_orig, ls.wi, ls.dist - 0.01f,
                               header, tlas_nodes, tlas_items, infinite,
                               objects, bvh_nodes, triangles, spheres, planes, fractals);
                if (!occ) {
                    float3 f_bsdf = (float3)(0.f, 0.f, 0.f);
                    if (cos_s_front > 1e-5f)
                        f_bsdf += (1.f - sss_w) * albedo * cos_s_front / 3.14159265359f;
                    if (cos_s_back > 1e-5f) {
                        float3 tint = subsurface_tint(sss_rad, sss_thick, cos_i_in, cos_s_back);
                        f_bsdf += sss_w * sss_col * tint * cos_s_back / 3.14159265359f;
                    }
                    float p_bsdf = cos_s_abs / 3.14159265359f;
                    float w_nee  = power_heuristic(ls.p_nee, p_bsdf);
                    *colour += *throughput * f_bsdf * ls.le / ls.p_nee * w_nee;
                }
            }
        }
    }

    /* ---- NEE: sky (reflect lobe — sample from +n hemisphere) ---- */
    float3 wi_sky_r = cosine_sample_hemisphere(n, rng);
    bool sky_r_occ = shadow_occluded(shadow_orig, wi_sky_r,
                         header, tlas_nodes, tlas_items, infinite,
                         objects, bvh_nodes, triangles, spheres, planes, fractals);
    if (!sky_r_occ)
        *colour += *throughput * (1.f - sss_w) * albedo * eval_sky(wi_sky_r, header) * 0.5f;

    /* ---- NEE: sky (transmit lobe — sample from -n hemisphere) ---- */
    float3 wi_sky_t = cosine_sample_hemisphere(-n, rng);
    bool sky_t_occ = shadow_occluded(shadow_orig, wi_sky_t,
                         header, tlas_nodes, tlas_items, infinite,
                         objects, bvh_nodes, triangles, spheres, planes, fractals);
    if (!sky_t_occ) {
        float cos_t = fmax(1e-4f, dot(wi_sky_t, -n));
        float3 tint = subsurface_tint(sss_rad, sss_thick, cos_i_in, cos_t);
        *colour += *throughput * sss_w * sss_col * tint * eval_sky(wi_sky_t, header) * 0.5f;
    }

    /* ---- BSDF scatter ---- */
    if (pcg32_f(rng) < sss_w) {
        float3 wi    = cosine_sample_hemisphere(-n, rng);
        float  cos_o = fmax(1e-4f, dot(wi, -n));
        float3 tint  = subsurface_tint(sss_rad, sss_thick, cos_i_in, cos_o);
        *throughput  *= sss_col * tint;
        *prev_bsdf_pdf = cos_o / 3.14159265359f;
        *ro = hit.pos - n * 0.001f;
        *rd = wi;
    } else {
        float3 wi    = cosine_sample_hemisphere(n, rng);
        float  cos_o = fmax(0.f, dot(wi, n));
        *throughput  *= albedo;
        *prev_bsdf_pdf = cos_o / 3.14159265359f;
        *ro = shadow_orig;
        *rd = wi;
    }
    *prev_delta = false;
}
