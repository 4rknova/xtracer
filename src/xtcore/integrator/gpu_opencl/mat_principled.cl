/*
 * mat_principled.cl — Anisotropic GGX Principled BSDF.
 * Mirrors CPU Principled: diffuse lobe (Lambert), specular lobe (anisotropic GGX),
 * clearcoat lobe (isotropic GGX, F0=0.04). Albedo/normal/roughness/metallic textures.
 */

/* ---- GGX helpers ---- */

static float ggx_ndf_aniso(float3 n, float3 t, float3 b, float3 h, float ax, float ay)
{
    float hx = dot(h, t);
    float hy = dot(h, b);
    float hn = dot(h, n);
    if (hn <= 0.f) return 0.f;
    float d = (hx*hx)/(ax*ax) + (hy*hy)/(ay*ay) + hn*hn;
    return 1.f / (3.14159265359f * ax * ay * d * d);
}

static float ggx_ndf_iso(float3 n, float3 h, float alpha)
{
    float nh = dot(n, h);
    if (nh <= 0.f) return 0.f;
    float a2 = alpha * alpha;
    float d  = nh*nh*(a2 - 1.f) + 1.f;
    return a2 / (3.14159265359f * d * d);
}

static float smith_g1_aniso(float3 v, float3 n, float3 t, float3 b, float ax, float ay)
{
    float vn = dot(v, n);
    if (vn <= 0.f) return 0.f;
    float vx = dot(v, t);
    float vy = dot(v, b);
    float denom = vn + sqrt((ax*vx)*(ax*vx) + (ay*vy)*(ay*vy) + vn*vn);
    return (denom > 1e-7f) ? (2.f * vn / denom) : 0.f;
}

static float smith_g1_iso(float3 v, float3 n, float alpha)
{
    float vn = dot(v, n);
    if (vn <= 0.f) return 0.f;
    float vn2  = vn * vn;
    float tan2 = fmax(0.f, 1.f - vn2) / fmax(vn2, 1e-7f);
    return 2.f / (1.f + sqrt(1.f + alpha*alpha*tan2));
}

static float3 fresnel_schlick3(float3 f0, float cos_theta)
{
    float x  = 1.f - clamp(cos_theta, 0.f, 1.f);
    float x2 = x * x;
    float x5 = x2 * x2 * x;
    return f0 + (1.f - f0) * x5;
}

static float fresnel_schlick1(float f0, float cos_theta)
{
    float x  = 1.f - clamp(cos_theta, 0.f, 1.f);
    float x2 = x * x;
    float x5 = x2 * x2 * x;
    return f0 + (1.f - f0) * x5;
}

static float3 sample_ggx_h_aniso(float3 n, float3 t, float3 b, float ax, float ay, pcg32_t *rng)
{
    float u1 = pcg32_f(rng), u2 = pcg32_f(rng);
    float phi  = atan2(ay * sin(6.28318530718f * u1), ax * cos(6.28318530718f * u1));
    float cp   = cos(phi), sp = sin(phi);
    float ap2  = cp*cp*ax*ax + sp*sp*ay*ay;
    float cos_t = sqrt(fmax(0.f, (1.f - u2) / fmax(1e-7f, u2*(ap2 - 1.f) + 1.f)));
    float sin_t = sqrt(fmax(0.f, 1.f - cos_t*cos_t));
    return f3norm(t*(sin_t*cp) + b*(sin_t*sp) + n*cos_t);
}

static float3 sample_ggx_h_iso(float3 n, float alpha, pcg32_t *rng)
{
    float u1  = pcg32_f(rng), u2 = pcg32_f(rng);
    float a2  = alpha * alpha;
    float cos_t = sqrt(fmax(0.f, (1.f - u2) / fmax(1e-7f, u2*(a2 - 1.f) + 1.f)));
    float sin_t = sqrt(fmax(0.f, 1.f - cos_t*cos_t));
    float phi = 6.28318530718f * u1;
    float3 tt, bb;
    tangent_basis(n, &tt, &bb);
    return f3norm(tt*(sin_t*cos(phi)) + bb*(sin_t*sin(phi)) + n*cos_t);
}

static float luma(float3 c) { return dot(c, (float3)(0.299f, 0.587f, 0.114f)); }

/* ---- Principled BSDF eval / sample ---- */

typedef struct { float3 f; float pdf; } bsdf_r_t;

static bsdf_r_t principled_eval(
    __global const gpu_material_t *mat, float3 base_color,
    float roughness, float metallic,
    float3 n, float3 t, float3 b,
    float3 wo, float3 wi)
{
    bsdf_r_t r; r.f = (float3)(0.f,0.f,0.f); r.pdf = 0.f;

    float cos_i = dot(n, wi);
    float cos_o = dot(n, wo);
    if (cos_i <= 1e-5f || cos_o <= 1e-5f) return r;
    float aniso     = mat->anisotropy;
    float aniso_rot = mat->anisotropy_rotation;
    float cc_raw    = mat->clearcoat;
    float cc_rough  = mat->clearcoat_roughness;
    float ior       = mat->ior > 1.f ? mat->ior : 1.5f;

    if (fabs(aniso_rot) > 1e-4f) {
        float rad = aniso_rot * (3.14159265359f / 180.f);
        float cs = cos(rad), sn = sin(rad);
        float3 rt = t*cs + b*sn;
        float3 rb = b*cs - t*sn;
        t = f3norm(rt); b = f3norm(rb);
    }

    float aspect = sqrt(fmax(0.1f, 1.f - 0.9f*aniso));
    float ax = fmax(0.02f, roughness / aspect);
    float ay = fmax(0.02f, roughness * aspect);

    float f0_d = (ior - 1.f) / (ior + 1.f); f0_d *= f0_d;
    float3 kd  = base_color * (1.f - metallic);
    float3 f0  = (float3)(f0_d,f0_d,f0_d)*(1.f - metallic) + base_color*metallic;
    float cc   = cc_raw * 0.6f;

    float3 h = f3norm(wo + wi);
    if (length(wo + wi) < 0.5f) return r;

    float nih      = fmax(0.f, dot(n, h));
    float voh      = fmax(1e-5f, dot(wo, h));

    float  D  = ggx_ndf_aniso(n, t, b, h, ax, ay);
    float  Gs = smith_g1_aniso(wo, n, t, b, ax, ay) * smith_g1_aniso(wi, n, t, b, ax, ay);
    float3 F  = fresnel_schlick3(f0, voh);
    float  denom = fmax(1e-7f, 4.f * cos_i * cos_o);
    float3 spec  = F * (D * Gs / denom);

    float coat_fo  = fresnel_schlick1(0.04f, cos_o);
    float coat_fi  = fresnel_schlick1(0.04f, cos_i);
    float base_att = (1.f - cc*coat_fo) * (1.f - cc*coat_fi);
    float  Dc = ggx_ndf_iso(n, h, cc_rough);
    float  Gc = smith_g1_iso(wo, n, cc_rough) * smith_g1_iso(wi, n, cc_rough);
    float  Fc = fresnel_schlick1(0.04f, voh);
    float3 coat_spec = (float3)(Fc,Fc,Fc) * (cc * Dc * Gc / denom);

    r.f = kd * (base_att / 3.14159265359f) + spec * base_att + coat_spec;

    float w_diff = fmax(0.02f, luma(kd) * (1.f - 0.35f*cc));
    float w_spec = fmax(0.04f, luma(f0) * (1.f - 0.35f*cc));
    float w_coat = cc > 1e-5f ? fmax(0.12f, cc) : 0.f;
    float w_sum  = fmax(1e-7f, w_diff + w_spec + w_coat);

    float pdf_diff = cos_i / 3.14159265359f;
    float pdf_spec = (nih > 0.f) ? (D * nih / (4.f * voh)) : 0.f;
    float pdf_coat = (nih > 0.f) ? (Dc * nih / (4.f * voh)) : 0.f;

    r.pdf = (w_diff*pdf_diff + w_spec*pdf_spec + w_coat*pdf_coat) / w_sum;
    return r;
}

static float3 principled_sample(
    __global const gpu_material_t *mat, float3 base_color,
    float roughness, float metallic,
    float3 n, float3 t, float3 b,
    float3 wo, pcg32_t *rng)
{
    float aniso     = mat->anisotropy;
    float aniso_rot = mat->anisotropy_rotation;
    float cc_raw    = mat->clearcoat;
    float cc_rough  = mat->clearcoat_roughness;
    float ior       = mat->ior > 1.f ? mat->ior : 1.5f;

    if (fabs(aniso_rot) > 1e-4f) {
        float rad = aniso_rot * (3.14159265359f / 180.f);
        float cs = cos(rad), sn = sin(rad);
        float3 rt = t*cs + b*sn;
        float3 rb = b*cs - t*sn;
        t = f3norm(rt); b = f3norm(rb);
    }

    float aspect = sqrt(fmax(0.1f, 1.f - 0.9f*aniso));
    float ax = fmax(0.02f, roughness / aspect);
    float ay = fmax(0.02f, roughness * aspect);

    float f0_d = (ior - 1.f) / (ior + 1.f); f0_d *= f0_d;
    float3 kd  = base_color * (1.f - metallic);
    float3 f0  = (float3)(f0_d,f0_d,f0_d)*(1.f - metallic) + base_color*metallic;
    float cc   = cc_raw * 0.6f;

    float w_diff = fmax(0.02f, luma(kd) * (1.f - 0.35f*cc));
    float w_spec = fmax(0.04f, luma(f0) * (1.f - 0.35f*cc));
    float w_coat = cc > 1e-5f ? fmax(0.12f, cc) : 0.f;
    float w_sum  = fmax(1e-7f, w_diff + w_spec + w_coat);
    float p_spec = w_spec / w_sum;
    float p_coat = w_coat / w_sum;

    float u = pcg32_f(rng);
    float3 wi;
    if (u < p_spec) {
        float3 h = sample_ggx_h_aniso(n, t, b, ax, ay, rng);
        wi = f3norm(2.f*dot(wo, h)*h - wo);
    } else if (u < p_spec + p_coat) {
        float3 h = sample_ggx_h_iso(n, cc_rough, rng);
        wi = f3norm(2.f*dot(wo, h)*h - wo);
    } else {
        wi = cosine_sample_hemisphere(n, rng);
    }
    if (dot(wi, n) <= 0.f) wi = cosine_sample_hemisphere(n, rng);
    return wi;
}

/* ---- shade_principled ---- */

static void shade_principled(
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
    float3 wo = f3norm(-*rd);

    float p_roughness = material_roughness(mat, hit.uv, hit.pos, tex_descs, pixels, samplers);
    float p_metallic  = material_metallic(mat, hit.uv, hit.pos, tex_descs, pixels, samplers);

    float3 tan_t, tan_b;
    tangent_basis(n, &tan_t, &tan_b);

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
                    bsdf_r_t b_l = principled_eval(mat, albedo, p_roughness, p_metallic,
                                                   n, tan_t, tan_b, wo, ls.wi);
                    float w_nee = power_heuristic(ls.p_nee, b_l.pdf);
                    *colour += *throughput * b_l.f * ls.le * (cos_s / ls.p_nee) * w_nee;
                }
            }
        }
    }

    /* ---- NEE: sky ---- */
    float3 wi_sky = cosine_sample_hemisphere(n, rng);
    float p_nee_sky = fmax(0.f, dot(n, wi_sky)) / 3.14159265359f;
    if (p_nee_sky > 1e-6f) {
        bsdf_r_t b_sky = principled_eval(mat, albedo, p_roughness, p_metallic,
                                         n, tan_t, tan_b, wo, wi_sky);
        bool sky_occ = shadow_occluded(shadow_orig, wi_sky,
                           header, tlas_nodes, tlas_items, infinite,
                           objects, bvh_nodes, triangles, spheres, planes, fractals);
        if (!sky_occ) {
            float mis_w   = power_heuristic(p_nee_sky, b_sky.pdf);
            float cos_nee = dot(n, wi_sky);
            *colour += *throughput * b_sky.f * eval_sky(wi_sky, header)
                     * (cos_nee / p_nee_sky) * mis_w;
        }
    }

    /* ---- BSDF continuation ---- */
    float3 wi_cont = principled_sample(mat, albedo, p_roughness, p_metallic,
                                       n, tan_t, tan_b, wo, rng);
    float cos_cont  = fmax(0.f, dot(wi_cont, n));
    bsdf_r_t b_cont = principled_eval(mat, albedo, p_roughness, p_metallic,
                                      n, tan_t, tan_b, wo, wi_cont);
    if (b_cont.pdf < 1e-7f || cos_cont <= 0.f) return;
    *prev_bsdf_pdf = b_cont.pdf;
    *throughput   *= b_cont.f * (cos_cont / b_cont.pdf);
    *ro            = shadow_orig;
    *rd            = wi_cont;
    *prev_delta    = false;
}
