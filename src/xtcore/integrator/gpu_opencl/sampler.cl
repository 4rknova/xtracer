/*
 * sampler.cl — GPU procedural sampler library.
 *
 * eval_sampler(idx, uv, pos, tex_descs, pixels, samplers) → float3
 *   idx       = index into samplers[] (-1 → returns (1,1,1) neutral)
 *   uv        = 2D texture coordinates
 *   pos       = 3D object-space position (used by TRIPLANAR)
 *   tex_descs = texture descriptor array
 *   pixels    = texture pixel atlas
 *   samplers  = sampler descriptor array
 *
 * Composite samplers (BLEND, MIX_MASKED, TRIPLANAR) call eval_sampler_leaf()
 * for their children — one level of indirection, no recursion (OpenCL C §6.8).
 *
 * Sampler type constants are defined in gpu_types.h (GPU_SAMPLER_*).
 */

/* ---- Hash utilities ---- */

/* Two-operand Murmur-mix used by Scratches (mirrors CPU sampler_scratches.cc). */
static uint smpl_uhash2(uint a, uint b)
{
    uint h = a * 0x9e3779b9u ^ b * 0x85ebca6bu;
    h ^= (h >> 16u); h *= 0x45d9f3bu; h ^= (h >> 16u);
    return h;
}

static float smpl_h01_2(uint a, uint b)
{
    return (float)(smpl_uhash2(a, b) & 0x00ffffffu) / (float)0x01000000;
}

/* Three-operand seeded hash used by Stars, EdgeWear, VoronoiNormal. */
static uint smpl_uhash3(int x, int y, int seed)
{
    uint h = (uint)seed ^ ((uint)x * 0x9e3779b9u) ^ ((uint)y * 0x85ebca6bu);
    h ^= (h >> 16u); h *= 0x45d9f3bu; h ^= (h >> 16u);
    return h;
}

static float smpl_h01_3(int x, int y, int seed, uint ch)
{
    uint h = smpl_uhash3(x, y, seed) ^ (ch * 0xc2b2ae35u);
    return (float)(h & 0x00ffffffu) / (float)0x01000000;
}

/* Two-int hash used by FBM-family (mirrors CPU sampler_fbm_*.cc). */
static float smpl_hash2f(int x, int y)
{
    uint h = (uint)x * 0x8da6b343u ^ (uint)y * 0xd8163841u;
    h ^= (h >> 13u); h *= 0x85ebca6bu; h ^= (h >> 16u);
    return (float)(h & 0x00ffffffu) / (float)0x01000000;
}

/* ---- Value noise + FBM (shared by FBMMarble, FBMWood, CurlNoise) ---- */

static float smpl_value_noise(float x, float y)
{
    int   ix = (int)floor(x), iy = (int)floor(y);
    float fx = x - (float)ix,  fy = y - (float)iy;
    float sx = fx * fx * (3.f - 2.f * fx);
    float sy = fy * fy * (3.f - 2.f * fy);
    float a = smpl_hash2f(ix,     iy    );
    float b = smpl_hash2f(ix + 1, iy    );
    float c = smpl_hash2f(ix,     iy + 1);
    float d = smpl_hash2f(ix + 1, iy + 1);
    return a + (b - a) * sx + (c - a) * sy + (a - b - c + d) * sx * sy;
}

static float smpl_fbm(float x, float y, int octaves, float lacunarity, float gain)
{
    float v = 0.f, amp = 0.5f, freq = 1.f;
    int oct = max(1, octaves);
    for (int i = 0; i < oct; ++i) {
        v    += amp * (2.f * smpl_value_noise(x * freq, y * freq) - 1.f);
        freq *= lacunarity;
        amp  *= gain;
    }
    return v;
}

/* ---- Individual sampler implementations ---- */

static float3 smpl_solid_color(__global const gpu_sampler_t *s)
{
    return (float3)(s->color_a.x, s->color_a.y, s->color_a.z);
}

static float3 smpl_checker(__global const gpu_sampler_t *s, float2 uv)
{
    float su = fabs(s->scale_u) > 1e-6f ? s->scale_u : 2.f;
    float sv = fabs(s->scale_v) > 1e-6f ? s->scale_v : 2.f;
    float u  = uv.x * su + s->offset_u;
    float v  = uv.y * sv + s->offset_v;
    bool odd = (((int)(floor(u)) + (int)(floor(v))) & 1) != (s->i_param0 ? 1 : 0);
    return odd ? (float3)(s->color_b.x, s->color_b.y, s->color_b.z)
               : (float3)(s->color_a.x, s->color_a.y, s->color_a.z);
}

static float3 smpl_brick(__global const gpu_sampler_t *s, float2 uv)
{
    float su = fabs(s->scale_u) > 1e-6f ? s->scale_u : 8.f;
    float sv = fabs(s->scale_v) > 1e-6f ? s->scale_v : 4.f;
    float u  = uv.x * su;
    float v  = uv.y * sv;
    int   row = (int)floor(v);
    float off = (row & 1) ? 0.5f : 0.f;
    int   col = (int)floor(u + off);
    float fu  = u + off - (float)col;
    float fv  = v - (float)row;
    float hmu = clamp(s->param0, 0.f, 1.f) * 0.5f;
    float hmv = clamp(s->param1, 0.f, 1.f) * 0.5f;
    bool mortar = (fu < hmu) || (fu > 1.f - hmu) || (fv < hmv) || (fv > 1.f - hmv);
    if (mortar) return (float3)(s->color_b.x, s->color_b.y, s->color_b.z);
    uint h  = smpl_uhash3(row, col, s->i_param0);
    float var = ((float)(h & 0x00ffffffu) / (float)0x01000000) * 2.f - 1.f;
    float cv  = s->param2 * var;
    return (float3)(clamp(s->color_a.x + cv, 0.f, 1.f),
                    clamp(s->color_a.y + cv, 0.f, 1.f),
                    clamp(s->color_a.z + cv, 0.f, 1.f));
}

static float3 smpl_dots(__global const gpu_sampler_t *s, float2 uv)
{
    float sc = fabs(s->scale_u) > 1e-6f ? s->scale_u : 8.f;
    float u  = uv.x * sc, v = uv.y * sc;
    float fu = u - floor(u) - 0.5f;
    float fv = v - floor(v) - 0.5f;
    float d  = sqrt(fu * fu + fv * fv);
    float r    = clamp(s->param0, 0.f, 1.f);
    float soft = clamp(s->param1, 0.f, 1.f) + 1e-6f;
    float t    = clamp(1.f - (d - r) / soft, 0.f, 1.f);
    float3 ca = (float3)(s->color_a.x, s->color_a.y, s->color_a.z);
    float3 cb = (float3)(s->color_b.x, s->color_b.y, s->color_b.z);
    return ca * (1.f - t) + cb * t;
}

static float3 smpl_graphpaper(__global const gpu_sampler_t *s, float2 uv)
{
    float sc = s->scale_u > 1e-6f ? s->scale_u : 16.f;
    float u  = uv.x * sc, v = uv.y * sc;
    int   iu = (int)floor(u), iv = (int)floor(v);
    float fu = u - (float)iu,  fv = v - (float)iv;
    float mw = clamp(s->param0, 0.f, 1.f) * 0.5f;
    float Mw = clamp(s->param1, 0.f, 1.f) * 0.5f;
    bool on_minor = (fu <= mw) || (fu >= 1.f - mw) || (fv <= mw) || (fv >= 1.f - mw);
    int every = max(1, s->i_param0);
    bool maj_u = (((iu % every) + every) % every == 0) && ((fu <= Mw) || (fu >= 1.f - Mw));
    bool maj_v = (((iv % every) + every) % every == 0) && ((fv <= Mw) || (fv >= 1.f - Mw));
    if (maj_u || maj_v) return (float3)(s->color_c.x, s->color_c.y, s->color_c.z);
    if (on_minor)       return (float3)(s->color_b.x, s->color_b.y, s->color_b.z);
    return (float3)(s->color_a.x, s->color_a.y, s->color_a.z);
}

static float3 smpl_stars(__global const gpu_sampler_t *s, float2 uv)
{
    /* For material use uv is treated as planar grid coords (not spherical). */
    const float grid_scale = 256.f;
    float gu = uv.x * grid_scale,  gv = uv.y * grid_scale;
    int   iu = (int)floor(gu),      iv = (int)floor(gv);
    float fu = gu - (float)iu,      fv = gv - (float)iv;
    float density = s->scale_u;
    int   seed    = s->i_param0;
    float min_br  = s->param0, max_br = s->param1, star_sz = s->param2;
    float3 result = (float3)(s->color_a.x, s->color_a.y, s->color_a.z);
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int cx = iu + dx, cy = iv + dy;
            if (smpl_h01_3(cx, cy, seed, 0u) > density) continue;
            float sx = (float)dx + smpl_h01_3(cx, cy, seed, 1u);
            float sy = (float)dy + smpl_h01_3(cx, cy, seed, 2u);
            float d2 = (fu - sx) * (fu - sx) + (fv - sy) * (fv - sy);
            float sz2 = (star_sz + 1e-6f) * (star_sz + 1e-6f);
            float gaus = exp(-d2 / (2.f * sz2));
            float brightness = min_br + (max_br - min_br) * smpl_h01_3(cx, cy, seed, 3u);
            float temp = smpl_h01_3(cx, cy, seed, 4u);
            result += (float3)(
                clamp(brightness * (0.80f + 0.20f * (1.f - temp)), 0.f, 1.f),
                clamp(brightness * (0.85f + 0.10f * (1.f - temp)), 0.f, 1.f),
                clamp(brightness * (0.90f + 0.10f * temp),         0.f, 1.f)
            ) * gaus;
        }
    }
    return result;
}

static float3 smpl_weave(__global const gpu_sampler_t *s, float2 uv)
{
    float sc       = fabs(s->scale_u) > 1e-6f ? s->scale_u : 12.f;
    float bw       = clamp(s->param0, 0.02f, 0.98f);
    float half_bnd = 0.5f * bw;
    float u = uv.x * sc,  v = uv.y * sc;
    int   iu = (int)floor(u), iv = (int)floor(v);
    float fu = u - (float)iu,  fv = v - (float)iv;
    bool on_warp = (fabs(fu - 0.5f) <= half_bnd);
    bool on_weft = (fabs(fv - 0.5f) <= half_bnd);
    float3 base_c = (float3)(s->color_a.x, s->color_a.y, s->color_a.z);
    float3 warp_c = (float3)(s->color_b.x, s->color_b.y, s->color_b.z);
    float3 weft_c = (float3)(s->color_c.x, s->color_c.y, s->color_c.z);
    if (!on_warp && !on_weft) return base_c;
    bool warp_top = (((iu + iv) & 1) == 0);
    float denom   = half_bnd > 1e-6f ? half_bnd : 1.f;
    float below   = 0.62f;
    if (on_warp && on_weft) {
        if (warp_top) { float c = clamp(fabs(fu - 0.5f) / denom, 0.f, 1.f); return warp_c * (0.78f + 0.22f * (1.f - c)); }
        float c = clamp(fabs(fv - 0.5f) / denom, 0.f, 1.f); return weft_c * (0.78f + 0.22f * (1.f - c));
    }
    if (on_warp) { float c = clamp(fabs(fu - 0.5f) / denom, 0.f, 1.f); float vis = warp_top ? 1.f : below; return warp_c * (vis * (0.78f + 0.22f * (1.f - c))); }
    { float c = clamp(fabs(fv - 0.5f) / denom, 0.f, 1.f); float vis = warp_top ? below : 1.f; return weft_c * (vis * (0.78f + 0.22f * (1.f - c))); }
}

static float3 smpl_voronoi_normal(__global const gpu_sampler_t *s, float2 uv)
{
    int   cells = max(1, s->i_param0);
    int   grid  = max(1, (int)ceil(sqrt((float)cells)));
    int   seed  = s->i_param1;
    float u = uv.x - floor(uv.x),  v = uv.y - floor(uv.y);
    float gx = u * (float)grid,     gy = v * (float)grid;
    int   ix = (int)floor(gx),      iy = (int)floor(gy);
    float best_d2 = 1e30f;
    int   best_x = 0, best_y = 0;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int cx = ix + dx, cy = iy + dy;
            int wx = ((cx % grid) + grid) % grid;
            int wy = ((cy % grid) + grid) % grid;
            float jx = smpl_h01_3(wx, wy, seed, 0u);
            float jy = smpl_h01_3(wx, wy, seed, 1u);
            float dxp = gx - ((float)cx + jx);
            float dyp = gy - ((float)cy + jy);
            float d2  = dxp * dxp + dyp * dyp;
            if (d2 < best_d2) { best_d2 = d2; best_x = wx; best_y = wy; }
        }
    }
    float max_dev = clamp(s->param0, 0.f, 89.f);
    float theta   = smpl_h01_3(best_x, best_y, seed, 2u) * (max_dev * (3.14159265359f / 180.f));
    float phi     = smpl_h01_3(best_x, best_y, seed, 3u) * (2.f * 3.14159265359f);
    float st = sin(theta), ct = cos(theta);
    float3 n = normalize((float3)(st * cos(phi), st * sin(phi), ct));
    return (n + 1.f) * 0.5f;
}

static float3 smpl_fbm_marble(__global const gpu_sampler_t *s, float2 uv)
{
    float sc  = fabs(s->scale_u) > 1e-6f ? s->scale_u : 6.f;
    int   oct = max(1, s->i_param0);
    float lac = max(1.f, s->param2), gain = clamp(s->param3, 0.f, 1.f);
    float x = uv.x * sc, y = uv.y * sc;
    float n = smpl_fbm(x, y, oct, lac, gain);
    float m = sin((x + y) * s->param0 + s->param1 * n);
    float t = 0.5f * m + 0.5f;
    float3 ca = (float3)(s->color_a.x, s->color_a.y, s->color_a.z);
    float3 cb = (float3)(s->color_b.x, s->color_b.y, s->color_b.z);
    float3 vc = (float3)(s->color_c.x, s->color_c.y, s->color_c.z);
    float3 base  = ca * (1.f - t) + cb * t;
    float  sharp = max(1e-4f, s->param5);
    float  vein  = pow(1.f - fabs(m), sharp);
    float  blend = clamp(s->param4 * vein, 0.f, 1.f);
    return base * (1.f - blend) + vc * blend;
}

static float3 smpl_fbm_wood(__global const gpu_sampler_t *s, float2 uv)
{
    float sc  = fabs(s->scale_u) > 1e-6f ? s->scale_u : 4.f;
    int   oct = max(1, s->i_param0);
    float lac = max(1.f, s->param2), gain = clamp(s->param3, 0.f, 1.f);
    float x = uv.x * sc, y = uv.y * sc;
    float dist = sqrt(x * x + y * y);
    float ring  = dist * s->param0 + s->param1 * smpl_fbm(x, y, oct, lac, gain);
    float t     = clamp(0.5f + 0.5f * sin(ring), 0.f, 1.f);
    float3 ca = (float3)(s->color_a.x, s->color_a.y, s->color_a.z);
    float3 cb = (float3)(s->color_b.x, s->color_b.y, s->color_b.z);
    return ca * (1.f - t) + cb * t;
}

static float3 smpl_curl_noise(__global const gpu_sampler_t *s, float2 uv)
{
    float sc  = fabs(s->scale_u) > 1e-6f ? s->scale_u : 3.f;
    int   oct = max(1, s->i_param0);
    float lac = max(1.f, s->param1), gain = clamp(s->param2, 0.f, 1.f);
    float x = uv.x * sc, y = uv.y * sc;
    const float EPS = 1e-3f;
    float dydx = (smpl_fbm(x + EPS, y,       oct, lac, gain) - smpl_fbm(x - EPS, y,       oct, lac, gain)) / (2.f * EPS);
    float dydy = (smpl_fbm(x,       y + EPS, oct, lac, gain) - smpl_fbm(x,       y - EPS, oct, lac, gain)) / (2.f * EPS);
    float wx = x + s->param0 * dydy;
    float wy = y - s->param0 * dydx;
    float n  = clamp(0.5f + 0.5f * smpl_fbm(wx, wy, oct, lac, gain), 0.f, 1.f);
    float3 ca = (float3)(s->color_a.x, s->color_a.y, s->color_a.z);
    float3 cb = (float3)(s->color_b.x, s->color_b.y, s->color_b.z);
    return ca * (1.f - n) + cb * n;
}

static float3 smpl_scratches(__global const gpu_sampler_t *s, float2 uv)
{
    float sc = fabs(s->scale_u) > 1e-6f ? s->scale_u : 4.f;
    float u = uv.x * sc, v = uv.y * sc;
    int   iu = (int)floor(u), iv = (int)floor(v);
    float w  = (s->param0 > 1e-6f) ? s->param0 * sc : 0.02f;
    int   n  = max(1, s->i_param0);
    uint  sk = (uint)s->i_param1;
    float ang  = s->param1;
    float ajit = s->param2;
    float min_dist = 1e30f;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int cx = iu + dx, cy = iv + dy;
            uint base_h  = smpl_uhash2((uint)cx, (uint)cy);
            uint base_h2 = smpl_uhash2((uint)cx * 7u, (uint)cy * 13u);
            for (int k = 0; k < n; ++k) {
                float cx01 = smpl_h01_2(base_h,  (uint)(k * 3 + 0) ^ sk);
                float cy01 = smpl_h01_2(base_h,  (uint)(k * 3 + 1) ^ sk);
                float aj   = smpl_h01_2(base_h,  (uint)(k * 3 + 2) ^ sk);
                float len01= smpl_h01_2(base_h2, (uint)k ^ sk);
                float a    = ang + ajit * (aj * 2.f - 1.f) * 3.14159265359f;
                float cosA = cos(a), sinA = sin(a);
                float scx  = (float)cx + cx01, scy = (float)cy + cy01;
                float hl   = (0.2f + 0.8f * len01) * 0.5f;
                float ex   = u - scx, ey = v - scy;
                float along = clamp(ex * cosA + ey * sinA, -hl, hl);
                float rx = ex - along * cosA, ry = ey - along * sinA;
                float d  = sqrt(rx * rx + ry * ry);
                if (d < min_dist) min_dist = d;
            }
        }
    }
    float t = clamp(1.f - min_dist / w, 0.f, 1.f);
    float3 ca = (float3)(s->color_a.x, s->color_a.y, s->color_a.z);
    float3 cb = (float3)(s->color_b.x, s->color_b.y, s->color_b.z);
    return ca * (1.f - t) + cb * t;
}

static float3 smpl_edge_wear(__global const gpu_sampler_t *s, float2 uv)
{
    float sc = fabs(s->scale_u) > 1e-6f ? s->scale_u : 8.f;
    float u  = uv.x * sc, v = uv.y * sc;
    int   iu = (int)floor(u), iv = (int)floor(v);
    int   seed = s->i_param0;
    float f1 = 1e30f, f2 = 1e30f;
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            int   cx = iu + dx, cy = iv + dy;
            float jx = smpl_h01_3(cx, cy, seed, 0u);
            float jy = smpl_h01_3(cx, cy, seed, 1u);
            float px = (float)cx + jx - u;
            float py = (float)cy + jy - v;
            float d  = px * px + py * py;
            if (d < f1) { f2 = f1; f1 = d; } else if (d < f2) { f2 = d; }
        }
    }
    float edge = sqrt(f2) - sqrt(f1);
    float cov  = clamp(s->param1, 0.f, 1.f);
    float sp   = max(0.1f, s->param0);
    float raw  = clamp(edge / (cov + 1e-6f), 0.f, 1.f);
    float t    = 1.f - pow(raw, sp);
    float3 ca = (float3)(s->color_a.x, s->color_a.y, s->color_a.z);
    float3 cb = (float3)(s->color_b.x, s->color_b.y, s->color_b.z);
    return ca * (1.f - t) + cb * t;
}

static float3 smpl_gradient(__global const gpu_sampler_t *s, float2 uv)
{
    float t = clamp(uv.y, 0.f, 1.f);
    float3 ca = (float3)(s->color_a.x, s->color_a.y, s->color_a.z);
    float3 cb = (float3)(s->color_b.x, s->color_b.y, s->color_b.z);
    return ca * (1.f - t) + cb * t;
}

/* ---- eval_sampler_leaf: all non-composite types ---- */

static float3 eval_sampler_leaf(int idx, float2 uv, float3 pos,
                                __global const gpu_tex_desc_t *tex_descs,
                                __global const float4         *pixels,
                                __global const gpu_sampler_t  *samplers)
{
    if (idx < 0) return (float3)(1.f, 1.f, 1.f);
    __global const gpu_sampler_t *s = &samplers[idx];
    switch (s->type) {
        case GPU_SAMPLER_SOLID_COLOR:    return smpl_solid_color(s);
        case GPU_SAMPLER_TEXTURE:        return sample_texture((uint)s->tex_index, uv, tex_descs, pixels);
        case GPU_SAMPLER_CHECKER:        return smpl_checker(s, uv);
        case GPU_SAMPLER_BRICK:          return smpl_brick(s, uv);
        case GPU_SAMPLER_DOTS:           return smpl_dots(s, uv);
        case GPU_SAMPLER_GRAPHPAPER:     return smpl_graphpaper(s, uv);
        case GPU_SAMPLER_STARS:          return smpl_stars(s, uv);
        case GPU_SAMPLER_WEAVE:          return smpl_weave(s, uv);
        case GPU_SAMPLER_VORONOI_NORMAL: return smpl_voronoi_normal(s, uv);
        case GPU_SAMPLER_FBM_MARBLE:     return smpl_fbm_marble(s, uv);
        case GPU_SAMPLER_FBM_WOOD:       return smpl_fbm_wood(s, uv);
        case GPU_SAMPLER_CURL_NOISE:     return smpl_curl_noise(s, uv);
        case GPU_SAMPLER_SCRATCHES:      return smpl_scratches(s, uv);
        case GPU_SAMPLER_EDGE_WEAR:      return smpl_edge_wear(s, uv);
        case GPU_SAMPLER_GRADIENT:       return smpl_gradient(s, uv);
        default:                         return (float3)(1.f, 1.f, 1.f);
    }
}

/* ---- eval_sampler: full dispatch including composites ---- */
/* Composite children are resolved with eval_sampler_leaf (no recursion). */

static float3 eval_sampler(int idx, float2 uv, float3 pos,
                           __global const gpu_tex_desc_t *tex_descs,
                           __global const float4         *pixels,
                           __global const gpu_sampler_t  *samplers)
{
    if (idx < 0) return (float3)(1.f, 1.f, 1.f);
    __global const gpu_sampler_t *s = &samplers[idx];

    switch (s->type) {
        case GPU_SAMPLER_BLEND: {
            float3 ca = eval_sampler_leaf(s->child_a, uv, pos, tex_descs, pixels, samplers);
            float3 cb = eval_sampler_leaf(s->child_b, uv, pos, tex_descs, pixels, samplers);
            float t = clamp(s->param0, 0.f, 1.f);
            return ca * (1.f - t) + cb * t;
        }
        case GPU_SAMPLER_MIX_MASKED: {
            float3 base_c = eval_sampler_leaf(s->child_a, uv, pos, tex_descs, pixels, samplers);
            float3 over_c = eval_sampler_leaf(s->child_b, uv, pos, tex_descs, pixels, samplers);
            float m;
            if (s->child_c >= 0) {
                float3 mask_c = eval_sampler_leaf(s->child_c, uv, pos, tex_descs, pixels, samplers);
                m = (mask_c.x + mask_c.y + mask_c.z) / 3.f;
            } else {
                m = clamp(s->param0, 0.f, 1.f);
            }
            m = clamp(m, 0.f, 1.f);
            switch (s->i_param0) {
                case 1: { /* MULTIPLY */
                    return base_c * (1.f - m) + (base_c * over_c) * m;
                }
                case 2: { /* ADD */
                    return (float3)(min(1.f, base_c.x + over_c.x * m),
                                    min(1.f, base_c.y + over_c.y * m),
                                    min(1.f, base_c.z + over_c.z * m));
                }
                case 3: { /* SCREEN */
                    float3 sc = 1.f - (1.f - base_c) * (1.f - over_c);
                    return base_c * (1.f - m) + sc * m;
                }
                default: /* LERP */
                    return base_c * (1.f - m) + over_c * m;
            }
        }
        case GPU_SAMPLER_TRIPLANAR: {
            float scale = fabs(s->scale_u) > 1e-8f ? s->scale_u : 1.f;
            float sp    = max(0.1f, s->param0);
            float2 px = (float2)(pos.y * scale, pos.z * scale); /* YZ plane */
            float2 py = (float2)(pos.x * scale, pos.z * scale); /* XZ plane */
            float2 pz = (float2)(pos.x * scale, pos.y * scale); /* XY plane */
            float3 cx = eval_sampler_leaf(s->child_a, px, pos, tex_descs, pixels, samplers);
            float3 cy = eval_sampler_leaf(s->child_a, py, pos, tex_descs, pixels, samplers);
            float3 cz = eval_sampler_leaf(s->child_a, pz, pos, tex_descs, pixels, samplers);
            float wx = pow(fabs(pos.x), sp);
            float wy = pow(fabs(pos.y), sp);
            float wz = pow(fabs(pos.z), sp);
            float wsum = wx + wy + wz + 1e-8f;
            return (cx * wx + cy * wy + cz * wz) / wsum;
        }
        default:
            return eval_sampler_leaf(idx, uv, pos, tex_descs, pixels, samplers);
    }
}
