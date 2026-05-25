/*
 * mat_lambert.cl — Lambertian diffuse BSDF.
 */

static void shade_lambert(
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
                    float p_bsdf = cos_s / 3.14159265359f;
                    float w_nee  = power_heuristic(ls.p_nee, p_bsdf);
                    *colour += *throughput * albedo * ls.le
                             * (cos_s / (3.14159265359f * ls.p_nee)) * w_nee;
                }
            }
        }
    }

    /* ---- NEE: sky ---- */
    float3 wi_nee = cosine_sample_hemisphere(n, rng);
    bool nee_blocked = shadow_occluded(shadow_orig, wi_nee,
                           header, tlas_nodes, tlas_items, infinite,
                           objects, bvh_nodes, triangles, spheres, planes, fractals);
    if (!nee_blocked)
        *colour += *throughput * albedo * eval_sky(wi_nee, header) * 0.5f;

    /* ---- BSDF continuation ---- */
    float3 wi    = cosine_sample_hemisphere(n, rng);
    float cos_wi = fmax(dot(wi, n), 0.f);
    *prev_bsdf_pdf = cos_wi / 3.14159265359f;
    *throughput   *= albedo;
    *ro            = shadow_orig;
    *rd            = wi;
    *prev_delta    = false;
}
