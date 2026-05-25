/*
 * pathtracer.cl — xtracer OpenCL megakernel path tracer
 *
 * One work item = one pixel per sample pass.
 * The accumulation buffer is summed across passes; the host averages on readback.
 */

/* ---- GPU types (C99-compatible, mirrors gpu_types.h) ---- */

typedef struct { float x, y, z;    } gpu_float3;
typedef struct { float x, y, z, w; } gpu_float4;
typedef unsigned int  gpu_uint;
typedef int           gpu_int;
typedef unsigned long gpu_ulong;

typedef struct { gpu_float3 mn; float _p0; gpu_float3 mx; float _p1; } gpu_aabb_t;

typedef struct { gpu_aabb_t aabb; gpu_int left, right; gpu_uint first, count; } gpu_tlas_node_t;
typedef struct { gpu_aabb_t aabb; gpu_ulong object_id; gpu_uint object_index, _pad; } gpu_tlas_item_t;
typedef struct { gpu_uint object_index; gpu_uint _pad[3]; } gpu_infinite_obj_t;

typedef struct { gpu_aabb_t aabb; gpu_uint left, right, first, count; } gpu_bvh_node_t;

typedef struct {
    gpu_float3 v0, v1, v2;
    gpu_float3 n0, n1, n2;
    float uv0[2], uv1[2], uv2[2];
    float _pad[2];
} gpu_triangle_t;

typedef struct { gpu_float3 origin; float radius; } gpu_sphere_t;
typedef struct { gpu_float3 normal; float offset; } gpu_plane_t;

#define GPU_FRACTAL_MANDELBULB      0
#define GPU_FRACTAL_JULIA           1
#define GPU_FRACTAL_MANDELBOX       2
#define GPU_FRACTAL_QUAT_JULIA      3
#define GPU_FRACTAL_MENGER          4
#define GPU_FRACTAL_SIERPINSKI_TET  5
#define GPU_FRACTAL_BURNING_SHIP    6
#define GPU_FRACTAL_CANTOR_DUST     7
#define GPU_FRACTAL_ICOSAHEDRAL_IFS 8

typedef struct {
    gpu_int    type;
    gpu_float3 origin;
    gpu_float3 param_vec;
    float      param_w, param_w0;
    float      radius, power, bailout, fold_size, min_r, scale_f;
    gpu_int    iterations, orbit_trap_channel;
    float      _pad[2];
} gpu_fractal_t;

#define GPU_GEOM_MESH    0
#define GPU_GEOM_SPHERE  1
#define GPU_GEOM_PLANE   2
#define GPU_GEOM_FRACTAL 3

typedef struct {
    gpu_int  geom_type;
    gpu_uint geom_index, mat_index, tri_base, bvh_base;
    gpu_uint _pad[3];
} gpu_object_t;

#define GPU_MAT_LAMBERT          0
#define GPU_MAT_EMISSIVE         1
#define GPU_MAT_DIELECTRIC       2
#define GPU_MAT_PRINCIPLED       3
#define GPU_MAT_SUBSURFACE       4
#define GPU_MAT_ROUGH_DIELECTRIC 5
#define GPU_MAT_PHONG            6
#define GPU_MAT_BLINNPHONG       7
#define GPU_MAT_SHEEN            8
#define GPU_MAT_THIN_DIELECTRIC  9
#define GPU_MAT_THIN_TRANSLUCENT 10
#define GPU_MAT_BOUNDARY         11

typedef struct {
    gpu_int    type;
    gpu_float3 albedo;
    float      roughness, ior;
    gpu_int    albedo_tex;
    gpu_int    nee_emissive;
    float      metallic;
    float      anisotropy;
    float      anisotropy_rotation;
    float      clearcoat;
    float      clearcoat_roughness;
    gpu_int    normal_tex;
    gpu_int    roughness_tex;
    gpu_int    metallic_tex;
} gpu_material_t;

typedef struct { gpu_uint width, height, pixel_offset, _pad; } gpu_tex_desc_t;

#define GPU_SAMPLER_SOLID_COLOR    0
#define GPU_SAMPLER_TEXTURE        1
#define GPU_SAMPLER_CHECKER        2
#define GPU_SAMPLER_BRICK          3
#define GPU_SAMPLER_DOTS           4
#define GPU_SAMPLER_GRAPHPAPER     5
#define GPU_SAMPLER_STARS          6
#define GPU_SAMPLER_WEAVE          7
#define GPU_SAMPLER_VORONOI_NORMAL 8
#define GPU_SAMPLER_FBM_MARBLE     9
#define GPU_SAMPLER_FBM_WOOD      10
#define GPU_SAMPLER_CURL_NOISE    11
#define GPU_SAMPLER_SCRATCHES     12
#define GPU_SAMPLER_EDGE_WEAR     13
#define GPU_SAMPLER_BLEND         14
#define GPU_SAMPLER_MIX_MASKED    15
#define GPU_SAMPLER_TRIPLANAR     16
#define GPU_SAMPLER_GRADIENT      17

typedef struct {
    gpu_int    type;
    gpu_int    child_a, child_b, child_c;
    gpu_float4 color_a, color_b, color_c;
    float      scale_u, scale_v, offset_u, offset_v;
    float      param0, param1, param2, param3, param4, param5;
    gpu_int    i_param0, i_param1;
    gpu_int    tex_index;
    gpu_int    _pad0, _pad1, _pad2;
} gpu_sampler_t;

typedef struct {
    gpu_float3 position; float _p0;
    gpu_float3 forward;  float _p1;
    gpu_float3 right;    float _p2;
    gpu_float3 up;       float _p3;
    float half_w_over_d, half_h_over_d;
    float aperture, focal_length;
} gpu_camera_t;

#define GPU_EMISSIVE_SPHERE   0
#define GPU_EMISSIVE_TRIANGLE 1

typedef struct {
    gpu_uint  object_index;
    gpu_uint  mat_index;
    gpu_int   type;
    gpu_uint  geom_index;     /* sphere: spheres[] index */
    gpu_uint  tri_base;       /* triangle/mesh: first triangle */
    gpu_uint  tri_count;      /* triangle/mesh: triangle count */
    float     area;
    float     select_weight;
} gpu_emissive_t;

typedef struct {
    gpu_uint  tlas_root, tlas_node_count, tlas_item_count;
    gpu_uint  object_count, infinite_count;
    gpu_uint  triangle_count, bvh_node_count;
    gpu_uint  sphere_count, plane_count, fractal_count, material_count;
    gpu_uint  emissive_count;
    float     total_select_weight;
    gpu_int   env_tex;
    float     env_intensity;
    float     env_sky_r, env_sky_g, env_sky_b;
    float     env_gnd_r, env_gnd_g, env_gnd_b;
} gpu_scene_header_t;

/* ---- PCG32 RNG ---- */

/* Canonical PCG32 — 64-bit state, 32-bit output.
 * Ref: O'Neill, "PCG: A Family of Simple Fast Space-Efficient Statistically
 * Good Algorithms for Random Number Generation", 2014. */
typedef struct { ulong state; ulong inc; } pcg32_t;

/* MurmurHash3 64-bit finalizer — every input bit flips ~half the output bits.
   Used to convert a (pixel, sample) pair into an uncorrelated 64-bit seed so
   that spatially adjacent pixels get completely independent RNG streams. */
static ulong rng_hash(ulong x)
{
    x ^= x >> 33u;
    x *= 0xFF51AFD7ED558CCDuL;
    x ^= x >> 33u;
    x *= 0xC4CEB9FE1A85EC53uL;
    x ^= x >> 33u;
    return x;
}

static void pcg32_seed(pcg32_t *rng, uint pixel_id, uint sample_index)
{
    ulong seed = rng_hash((ulong)pixel_id | ((ulong)sample_index << 32u));
    ulong seq  = rng_hash((ulong)sample_index | ((ulong)pixel_id  << 32u));
    rng->state = 0UL;
    rng->inc   = (seq << 1UL) | 1UL;
    rng->state = rng->state * 6364136223846793005UL + rng->inc;
    rng->state += seed;
    rng->state = rng->state * 6364136223846793005UL + rng->inc;
}

static uint pcg32_next(pcg32_t *rng)
{
    ulong old  = rng->state;
    rng->state = old * 6364136223846793005UL + rng->inc;
    uint xsh   = (uint)(((old >> 18u) ^ old) >> 27u);
    uint rot   = (uint)(old >> 59u);
    return (xsh >> rot) | (xsh << ((-rot) & 31u));
}

static float pcg32_f(pcg32_t *rng)
{
    return (float)(pcg32_next(rng) >> 8) * (1.f / 16777216.f);
}

/* ---- float3 math ---- */

#define F3(a,b,c) ((float3)(a,b,c))

static float f3dot(float3 a, float3 b)   { return dot(a, b); }
static float f3len(float3 v)             { return length(v); }
static float3 f3norm(float3 v)           { return normalize(v); }
static float3 f3reflect(float3 i, float3 n) { return i - 2.f*dot(i,n)*n; }

static float3 f3refract(float3 i, float3 n, float eta)
{
    float cos_i = -dot(i, n);
    float sin2t = eta*eta*(1.f - cos_i*cos_i);
    if (sin2t > 1.f) return F3(0,0,0); // TIR
    return eta*i + (eta*cos_i - sqrt(1.f - sin2t))*n;
}

static float schlick(float cos_theta, float ior)
{
    float r0 = (1.f - ior)/(1.f + ior);
    r0 *= r0;
    float x = 1.f - cos_theta;
    return r0 + (1.f - r0)*x*x*x*x*x;
}

static float3 tangent_basis(float3 n, float3 *t, float3 *b)
{
    float3 up = fabs(n.y) < 0.999f ? F3(0,1,0) : F3(1,0,0);
    *t = f3norm(cross(up, n));
    *b = cross(n, *t);
    return n;
}

static float3 cosine_sample_hemisphere(float3 n, pcg32_t *rng)
{
    float u1 = pcg32_f(rng), u2 = pcg32_f(rng);
    float r   = sqrt(u1);
    float phi = 6.28318530718f * u2;
    float3 t, b;
    tangent_basis(n, &t, &b);
    return f3norm(t*(r*cos(phi)) + b*(r*sin(phi)) + n*sqrt(1.f-u1));
}

/* Sample a power cosine lobe around axis `r` (Phong specular). */
static float3 power_cosine_sample(float3 r, float exponent, pcg32_t *rng)
{
    float u1 = pcg32_f(rng), u2 = pcg32_f(rng);
    float cos_t = pow(fmax(u1, 1e-7f), 1.f / (exponent + 1.f));
    float sin_t = sqrt(fmax(0.f, 1.f - cos_t*cos_t));
    float phi   = 6.28318530718f * u2;
    float3 t, b;
    tangent_basis(r, &t, &b);
    return f3norm(t*(sin_t*cos(phi)) + b*(sin_t*sin(phi)) + r*cos_t);
}

/* ---- Ray-AABB slab test ---- */

static bool aabb_hit(float3 mn, float3 mx, float3 ro, float3 rd, float tmin, float tmax)
{
    float3 inv = (float3)(1.f/rd.x, 1.f/rd.y, 1.f/rd.z);
    float3 t0  = (mn - ro)*inv;
    float3 t1  = (mx - ro)*inv;
    float3 ta  = fmin(t0, t1);
    float3 tb  = fmax(t0, t1);
    tmin = fmax(tmin, fmax(ta.x, fmax(ta.y, ta.z)));
    tmax = fmin(tmax, fmin(tb.x, fmin(tb.y, tb.z)));
    return tmin <= tmax;
}

/* ---- Triangle intersection (Möller–Trumbore) ---- */

#define EPSILON 1e-6f
#define T_MIN   1e-4f
#define T_MAX   1e20f

typedef struct {
    float t;
    float3 pos, normal;
    float2 uv;
    int    hit;
    int    mat_index;
    uint   object_index;
} hit_t;

static void tri_intersect(__global const gpu_triangle_t *tri,
                          float3 ro, float3 rd, float tmax,
                          float *out_t, float *out_u, float *out_v)
{
    float3 v0 = (float3)(tri->v0.x, tri->v0.y, tri->v0.z);
    float3 v1 = (float3)(tri->v1.x, tri->v1.y, tri->v1.z);
    float3 v2 = (float3)(tri->v2.x, tri->v2.y, tri->v2.z);
    float3 e1 = v1 - v0, e2 = v2 - v0;
    float3 h  = cross(rd, e2);
    float  a  = dot(e1, h);
    if (fabs(a) < EPSILON) return;
    float  f  = 1.f / a;
    float3 s  = ro - v0;
    float  u  = f * dot(s, h);
    if (u < 0.f || u > 1.f) return;
    float3 q  = cross(s, e1);
    float  v  = f * dot(rd, q);
    if (v < 0.f || u+v > 1.f) return;
    float  t  = f * dot(e2, q);
    if (t < T_MIN || t >= tmax) return;
    *out_t = t; *out_u = u; *out_v = v;
}

/* ---- Mesh BVH traversal ---- */

static bool mesh_intersect(__global const gpu_bvh_node_t *bvh_nodes,
                           __global const gpu_triangle_t  *triangles,
                           uint bvh_base, uint tri_base,
                           float3 ro, float3 rd,
                           hit_t *best, int mat_index)
{
    int stack[64]; int sp = 0;
    stack[sp++] = (int)bvh_base;
    bool found = false;

    while (sp > 0) {
        int ni = stack[--sp];
        __global const gpu_bvh_node_t *node = &bvh_nodes[ni];

        if (!aabb_hit((float3)(node->aabb.mn.x, node->aabb.mn.y, node->aabb.mn.z),
                      (float3)(node->aabb.mx.x, node->aabb.mx.y, node->aabb.mx.z),
                      ro, rd, T_MIN, best->t > 0.f ? best->t : T_MAX)) continue;

        if (node->count > 0) {
            for (uint i = 0; i < node->count; ++i) {
                float t = T_MAX, u = 0.f, v = 0.f;
                __global const gpu_triangle_t *tri = &triangles[node->first + i];
                tri_intersect(tri, ro, rd, best->t > 0.f ? best->t : T_MAX, &t, &u, &v);
                if (t < T_MAX && (best->t <= 0.f || t < best->t)) {
                    best->t = t;
                    best->pos = ro + rd*t;
                    float w = 1.f - u - v;
                    float3 n0 = (float3)(tri->n0.x, tri->n0.y, tri->n0.z);
                    float3 n1 = (float3)(tri->n1.x, tri->n1.y, tri->n1.z);
                    float3 n2 = (float3)(tri->n2.x, tri->n2.y, tri->n2.z);
                    best->normal = f3norm(w*n0 + u*n1 + v*n2);
                    best->uv     = (float2)(w*tri->uv0[0] + u*tri->uv1[0] + v*tri->uv2[0],
                                           w*tri->uv0[1] + u*tri->uv1[1] + v*tri->uv2[1]);
                    best->mat_index = mat_index;
                    best->hit = 1;
                    found = true;
                }
            }
        } else {
            if (sp + 2 <= 64) {
                stack[sp++] = (int)node->right;
                stack[sp++] = (int)node->left;
            }
        }
    }
    return found;
}

/* ---- Analytic geometry ---- */

static void sphere_intersect(float3 center, float radius,
                             float3 ro, float3 rd,
                             hit_t *best, int mat_index)
{
    float3 oc = ro - center;
    float  b  = dot(oc, rd);
    float  c  = dot(oc, oc) - radius*radius;
    float  d  = b*b - c;
    if (d < 0.f) return;
    float sq = sqrt(d);
    float t  = -b - sq;
    if (t < T_MIN) t = -b + sq;
    if (t < T_MIN || (best->t > 0.f && t >= best->t)) return;
    best->t       = t;
    best->pos     = ro + rd*t;
    best->normal  = f3norm(best->pos - center);
    best->uv      = (float2)(0,0);
    best->mat_index = mat_index;
    best->hit     = 1;
}

static void plane_intersect(float3 normal, float offset,
                            float3 ro, float3 rd,
                            hit_t *best, int mat_index)
{
    float denom = dot(rd, normal);
    if (fabs(denom) < EPSILON) return;
    float t = (offset - dot(ro, normal)) / denom;
    if (t < T_MIN || (best->t > 0.f && t >= best->t)) return;
    best->t       = t;
    best->pos     = ro + rd*t;
    best->normal  = denom < 0.f ? normal : -normal;
    best->uv      = (float2)(0,0);
    best->mat_index = mat_index;
    best->hit     = 1;
}

/* ---- Fractal SDFs ---- */

static float sdf_mandelbulb(float3 p, __global const gpu_fractal_t *f)
{
    float3 z = p - (float3)(f->origin.x, f->origin.y, f->origin.z);
    float dr = 1.f, r = 0.f;
    float pw = f->power;
    for (int i = 0; i < f->iterations; ++i) {
        r = length(z);
        if (r > f->bailout) break;
        float theta = acos(z.z / r) * pw;
        float phi   = atan2(z.y, z.x) * pw;
        float rn_1  = pow(r, pw - 1.f);  /* r^(power-1) — matches CPU fractal_power_distance_estimator */
        float rn    = rn_1 * r;
        dr = rn_1 * pw * dr + 1.f;
        float st = sin(theta), ct = cos(theta);
        float sp = sin(phi),   cp = cos(phi);
        z = rn*(float3)(st*cp, st*sp, ct) + p - (float3)(f->origin.x,f->origin.y,f->origin.z);
    }
    if (r <= 0.f) return 0.f;
    return 0.5f * log(r) * r / dr;
}

static float sdf_julia(float3 p, __global const gpu_fractal_t *f)
{
    /* Mirrors CPU fractal_power_distance_estimator(p, julia_c, iters, power, bailout, use_seed_c=true).
     * No escaped flag — the CPU always computes the DE from the final z/dr regardless of escape. */
    float3 z = p - (float3)(f->origin.x, f->origin.y, f->origin.z);
    float3 c = (float3)(f->param_vec.x, f->param_vec.y, f->param_vec.z);
    float dr = 1.f;
    for (int i = 0; i < f->iterations; ++i) {
        float r = length(z);
        if (r > f->bailout) break;
        if (r < 1e-9f) { z = c; continue; }
        float theta = acos(clamp(z.z / r, -1.f, 1.f)) * f->power;
        float phi   = atan2(z.y, z.x) * f->power;
        float rn_1  = pow(r, f->power - 1.f);  /* r^(power-1) — matches CPU */
        float rn    = rn_1 * r;
        dr = rn_1 * f->power * dr + 1.f;
        z  = rn*(float3)(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta)) + c;
    }
    float r = length(z);
    if (dr <= EPSILON || r <= EPSILON) return 1e20f;
    return 0.5f * log(r) * r / dr;
}

static float sdf_mandelbox(float3 p, __global const gpu_fractal_t *f)
{
    float3 z   = p - (float3)(f->origin.x, f->origin.y, f->origin.z);
    float  dr  = 1.f;
    float  fold = f->fold_size;
    float  min2 = f->min_r * f->min_r;
    float  sc   = f->scale_f;
    for (int i = 0; i < f->iterations; ++i) {
        // Box fold
        z = clamp(z, -fold, fold)*2.f - z;
        // Sphere fold
        float r2 = dot(z, z);
        if      (r2 < min2)       { z *= (sc / min2); dr *= (sc / min2); }
        else if (r2 < 1.f)        { z /= r2;           dr /= r2; }
        z = z*sc + (p - (float3)(f->origin.x, f->origin.y, f->origin.z));
        dr = dr * fabs(sc) + 1.f;
    }
    return (length(z) - (fabs(sc)-1.f)) / fabs(dr);
}

static float sdf_menger(float3 p, __global const gpu_fractal_t *f)
{
    float3 q = fabs(p - (float3)(f->origin.x, f->origin.y, f->origin.z)) / f->radius;
    float d = -1e20f;
    float s = 1.f;
    for (int i = 0; i < f->iterations; ++i) {
        float3 r = fabs(q * s) - 1.f;
        float  c = fmax(r.x, fmax(r.y, r.z));
        // Cross-tube SDF (approximation)
        float c2 = fmin(fmax(r.x, r.y), fmin(fmax(r.x, r.z), fmax(r.y, r.z)));
        d = fmax(d, c2 / s);
        s *= 3.f;
    }
    return d * f->radius;
}

static float sdf_sierpinski(float3 p, __global const gpu_fractal_t *f)
{
    float3 q  = p - (float3)(f->origin.x, f->origin.y, f->origin.z);
    float  scale = 2.f;
    for (int i = 0; i < f->iterations; ++i) {
        if (q.x + q.y < 0.f) { float t=q.x; q.x=-q.y; q.y=-t; }
        if (q.x + q.z < 0.f) { float t=q.x; q.x=-q.z; q.z=-t; }
        if (q.y + q.z < 0.f) { float t=q.y; q.y=-q.z; q.z=-t; }
        q = q * scale - (float3)(1.f, 1.f, 1.f) * (scale - 1.f);
    }
    return (length(q) - 0.5f) / pow(scale, (float)f->iterations);
}

static float sdf_quat_julia(float3 p, __global const gpu_fractal_t *f)
{
    // z = (real=px, imag1=py, imag2=pz, imag3=w0); c = (real=param_w, imag=param_vec)
    float4 z = (float4)(p.x - f->origin.x,
                        p.y - f->origin.y,
                        p.z - f->origin.z,
                        f->param_w0);
    // c layout: c.w = quat_cw (real), c.xyz = quat_c.xyz (imaginary)
    float4 c = (float4)(f->param_vec.x, f->param_vec.y, f->param_vec.z, f->param_w);
    float  dr   = 1.f;
    float  bail2 = f->bailout * f->bailout;
    for (int i = 0; i < f->iterations; ++i) {
        float r2 = dot(z, z);
        if (r2 > bail2) {
            float r = sqrt(r2);
            if (dr <= 1e-10f || r <= 1.f) return 0.f;
            return 0.5f * r * log(r) / dr;
        }
        dr = 2.f * sqrt(r2) * dr;  // no +1 for Julia (c is a constant, not position)
        float4 zz;
        zz.x = z.x*z.x - z.y*z.y - z.z*z.z - z.w*z.w + c.w;  // real part gets c.w = quat_cw
        zz.y = 2.f*(z.x*z.y) + c.x;  // c.x = quat_c.x
        zz.z = 2.f*(z.x*z.z) + c.y;  // c.y = quat_c.y
        zz.w = 2.f*(z.x*z.w) + c.z;  // c.z = quat_c.z
        z = zz;
    }
    return 0.f;  // non-escaped: interior of attractor
}

static float sdf_burning_ship(float3 p, __global const gpu_fractal_t *f)
{
    float3 z   = p - (float3)(f->origin.x, f->origin.y, f->origin.z);
    float  dr  = 1.f;
    for (int i = 0; i < f->iterations; ++i) {
        float r = length(z);
        if (r > f->bailout) break;
        z = fabs(z);
        float theta = acos(z.z/r) * f->power;
        float phi   = atan2(z.y, z.x) * f->power;
        float rn_1  = pow(r, f->power - 1.f);  /* r^(power-1) — matches CPU */
        float rn    = rn_1 * r;
        dr = rn_1 * f->power * dr + 1.f;
        z  = rn*(float3)(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));
        z += p - (float3)(f->origin.x,f->origin.y,f->origin.z);
    }
    float r = length(z);
    if (r <= 0.f) return 0.f;
    return 0.5f * log(r) * r / dr;
}

static float sdf_cantor_dust(float3 p, __global const gpu_fractal_t *f)
{
    float3 q = fabs(p - (float3)(f->origin.x, f->origin.y, f->origin.z));
    float  d = fmax(q.x, fmax(q.y, q.z)) - f->radius;
    float  s = 1.f;
    for (int i = 0; i < f->iterations; ++i) {
        s  /= 3.f;
        float3 r = q / s;
        r = fmod(r, 3.f);
        float3 ld = 1.f - fabs(r - 1.f);
        d = fmax(d, fmin(ld.x, fmin(ld.y, ld.z)) * s);
    }
    return d;
}

static float sdf_icosahedral(float3 p, __global const gpu_fractal_t *f)
{
    float3 q = p - (float3)(f->origin.x, f->origin.y, f->origin.z);
    float s  = f->scale_f;
    float r  = f->radius;
    // Icosahedral fold normals (simplified)
    const float3 n1 = normalize((float3)( 1.f,  1.f, 0.f));
    const float3 n2 = normalize((float3)( 1.f,  0.f, 1.f));
    const float3 n3 = normalize((float3)( 0.f,  1.f, 1.f));
    for (int i = 0; i < f->iterations; ++i) {
        q -= 2.f * fmin(0.f, dot(q, n1)) * n1;
        q -= 2.f * fmin(0.f, dot(q, n2)) * n2;
        q -= 2.f * fmin(0.f, dot(q, n3)) * n3;
        q  = q * s - (float3)(1.f,1.f,1.f) * (s - 1.f);
    }
    return (length(q) - 0.5f) / pow(s, (float)f->iterations) * r;
}

static float eval_sdf(float3 p, __global const gpu_fractal_t *f)
{
    switch (f->type) {
        case GPU_FRACTAL_MANDELBULB:     return sdf_mandelbulb(p, f);
        case GPU_FRACTAL_JULIA:          return sdf_julia(p, f);
        case GPU_FRACTAL_MANDELBOX:      return sdf_mandelbox(p, f);
        case GPU_FRACTAL_QUAT_JULIA:     return sdf_quat_julia(p, f);
        case GPU_FRACTAL_MENGER:         return sdf_menger(p, f);
        case GPU_FRACTAL_SIERPINSKI_TET: return sdf_sierpinski(p, f);
        case GPU_FRACTAL_BURNING_SHIP:   return sdf_burning_ship(p, f);
        case GPU_FRACTAL_CANTOR_DUST:    return sdf_cantor_dust(p, f);
        case GPU_FRACTAL_ICOSAHEDRAL_IFS:return sdf_icosahedral(p, f);
        default: return 1e20f;
    }
}

static float3 sdf_normal(float3 p, __global const gpu_fractal_t *f)
{
    const float h = 0.001f;
    float dx = eval_sdf(p + (float3)(h,0,0), f) - eval_sdf(p - (float3)(h,0,0), f);
    float dy = eval_sdf(p + (float3)(0,h,0), f) - eval_sdf(p - (float3)(0,h,0), f);
    float dz = eval_sdf(p + (float3)(0,0,h), f) - eval_sdf(p - (float3)(0,0,h), f);
    return f3norm((float3)(dx, dy, dz));
}

static void fractal_intersect(__global const gpu_fractal_t *frac,
                              float3 ro, float3 rd,
                              hit_t *best, int mat_index)
{
    const int   MAX_STEPS  = 256;
    const float HIT_DIST   = 0.001f;

    float t = T_MIN;
    float tmax = best->t > 0.f ? best->t : T_MAX;

    // Bound by sphere of radius = fractal.radius * 2 for entry
    float3 origin = (float3)(frac->origin.x, frac->origin.y, frac->origin.z);
    float3 oc = ro - origin;
    float  b  = dot(oc, rd);
    float  c  = dot(oc, oc) - (frac->radius*frac->radius*4.f);
    float  d  = b*b - c;
    if (d < 0.f) return;
    float sq = sqrt(d);
    float t0 = fmax(T_MIN, -b - sq);
    float t1 = -b + sq;
    if (t1 < T_MIN) return;
    t = t0;
    tmax = fmin(tmax, t1);

    for (int s = 0; s < MAX_STEPS && t < tmax; ++s) {
        float3 p   = ro + rd*t;
        float  dist = fabs(eval_sdf(p, frac));  // abs matches CPU raymarcher behavior
        if (dist < HIT_DIST) {
            if (best->t <= 0.f || t < best->t) {
                best->t           = t;
                best->pos         = p;
                best->normal      = sdf_normal(p, frac);
                best->uv          = (float2)(0,0);
                best->mat_index   = mat_index;
                best->hit         = 1;
            }
            return;
        }
        t += fmax(dist * 0.5f, HIT_DIST);
    }
}

/* ---- Scene intersection (object dispatch) ---- */

static void intersect_object(
    uint obj_idx,
    __global const gpu_object_t   *objects,
    __global const gpu_bvh_node_t *bvh_nodes,
    __global const gpu_triangle_t *triangles,
    __global const gpu_sphere_t   *spheres,
    __global const gpu_plane_t    *planes,
    __global const gpu_fractal_t  *fractals,
    float3 ro, float3 rd, hit_t *best)
{
    __global const gpu_object_t *obj = &objects[obj_idx];
    int mat = (int)obj->mat_index;

    switch (obj->geom_type) {
        case GPU_GEOM_MESH:
            mesh_intersect(bvh_nodes, triangles,
                           obj->bvh_base, obj->tri_base,
                           ro, rd, best, mat);
            break;
        case GPU_GEOM_SPHERE: {
            __global const gpu_sphere_t *sp = &spheres[obj->geom_index];
            sphere_intersect((float3)(sp->origin.x, sp->origin.y, sp->origin.z),
                             sp->radius, ro, rd, best, mat);
            break;
        }
        case GPU_GEOM_PLANE: {
            __global const gpu_plane_t *pl = &planes[obj->geom_index];
            plane_intersect((float3)(pl->normal.x, pl->normal.y, pl->normal.z),
                            pl->offset, ro, rd, best, mat);
            break;
        }
        case GPU_GEOM_FRACTAL:
            fractal_intersect(&fractals[obj->geom_index], ro, rd, best, mat);
            break;
    }
}

/* ---- TLAS traversal ---- */

static hit_t scene_intersect(
    float3 ro, float3 rd,
    gpu_scene_header_t hdr,
    __global const gpu_tlas_node_t    *tlas_nodes,
    __global const gpu_tlas_item_t    *tlas_items,
    __global const gpu_infinite_obj_t *infinite,
    __global const gpu_object_t       *objects,
    __global const gpu_bvh_node_t     *bvh_nodes,
    __global const gpu_triangle_t     *triangles,
    __global const gpu_sphere_t       *spheres,
    __global const gpu_plane_t        *planes,
    __global const gpu_fractal_t      *fractals)
{
    hit_t best; best.t = -1.f; best.hit = 0; best.mat_index = -1; best.object_index = ~0u;

    // Infinite objects (planes)
    for (uint i = 0; i < hdr.infinite_count; ++i) {
        uint oi = infinite[i].object_index;
        if (oi < hdr.object_count) {
            float prev_t = best.t;
            intersect_object(oi, objects, bvh_nodes, triangles,
                             spheres, planes, fractals, ro, rd, &best);
            if (best.t != prev_t) best.object_index = oi;
        }
    }

    // TLAS BVH
    if (hdr.tlas_node_count == 0) return best;

    int stack[32]; int sp = 0;
    stack[sp++] = (int)hdr.tlas_root;

    while (sp > 0) {
        int ni = stack[--sp];
        __global const gpu_tlas_node_t *node = &tlas_nodes[ni];

        if (!aabb_hit((float3)(node->aabb.mn.x, node->aabb.mn.y, node->aabb.mn.z),
                      (float3)(node->aabb.mx.x, node->aabb.mx.y, node->aabb.mx.z),
                      ro, rd, T_MIN, best.t > 0.f ? best.t : T_MAX)) continue;

        if (node->count > 0) {
            for (uint i = 0; i < node->count; ++i) {
                uint item_idx = node->first + i;
                uint oi = tlas_items[item_idx].object_index;
                if (oi < hdr.object_count) {
                    float prev_t = best.t;
                    intersect_object(oi, objects, bvh_nodes, triangles,
                                     spheres, planes, fractals, ro, rd, &best);
                    if (best.t != prev_t) best.object_index = oi;
                }
            }
        } else {
            if (sp + 2 <= 32) {
                stack[sp++] = (int)node->right;
                stack[sp++] = (int)node->left;
            }
        }
    }
    return best;
}

/* ---- Sky evaluation ---- */

static float3 eval_sky(float3 dir, gpu_scene_header_t hdr)
{
    /* Gradient remap: t = 0.5*(dir.y+1) maps dir.y in [-1,+1] to t in [0,1].
     * t=0 (looking straight down)  → colour a (env_sky)
     * t=1 (looking straight up)    → colour b (env_gnd)
     * This matches the intended behaviour of CPU Gradient::sample(). */
    float t = 0.5f * (dir.y + 1.0f);
    float3 a = F3(hdr.env_sky_r, hdr.env_sky_g, hdr.env_sky_b);
    float3 b = F3(hdr.env_gnd_r, hdr.env_gnd_g, hdr.env_gnd_b);
    return ((1.f - t) * a + t * b) * hdr.env_intensity;
}

/* ---- Shadow occlusion test ---- */

static bool shadow_occluded(
    float3 ro, float3 rd,
    gpu_scene_header_t hdr,
    __global const gpu_tlas_node_t    *tlas_nodes,
    __global const gpu_tlas_item_t    *tlas_items,
    __global const gpu_infinite_obj_t *infinite,
    __global const gpu_object_t       *objects,
    __global const gpu_bvh_node_t     *bvh_nodes,
    __global const gpu_triangle_t     *triangles,
    __global const gpu_sphere_t       *spheres,
    __global const gpu_plane_t        *planes,
    __global const gpu_fractal_t      *fractals)
{
    hit_t h = scene_intersect(ro, rd, hdr, tlas_nodes, tlas_items, infinite,
                              objects, bvh_nodes, triangles, spheres, planes, fractals);
    return h.hit && h.t > 0.f;
}

/* Shadow ray limited to tmax — for area light NEE so the light surface itself
   is not counted as an occluder.  Uses the same full traversal as scene_intersect
   but rejects hits at t >= tmax. */
static bool shadow_occluded_tmax(
    float3 ro, float3 rd, float tmax,
    gpu_scene_header_t hdr,
    __global const gpu_tlas_node_t    *tlas_nodes,
    __global const gpu_tlas_item_t    *tlas_items,
    __global const gpu_infinite_obj_t *infinite,
    __global const gpu_object_t       *objects,
    __global const gpu_bvh_node_t     *bvh_nodes,
    __global const gpu_triangle_t     *triangles,
    __global const gpu_sphere_t       *spheres,
    __global const gpu_plane_t        *planes,
    __global const gpu_fractal_t      *fractals)
{
    hit_t h = scene_intersect(ro, rd, hdr, tlas_nodes, tlas_items, infinite,
                              objects, bvh_nodes, triangles, spheres, planes, fractals);
    return h.hit && h.t > 0.f && h.t < tmax;
}

/* Power heuristic (β=2) for MIS between two sampling strategies. */
static float power_heuristic(float p_a, float p_b)
{
    float a2 = p_a * p_a;
    float b2 = p_b * p_b;
    float d  = a2 + b2;
    return (d > 0.f) ? (a2 / d) : 0.f;
}

/* ---- Area light sampling ---- */

/* Result of one NEE light sample. */
typedef struct {
    float3 wi;          /* unit direction from shading point toward sampled light point */
    float3 light_n;     /* outward normal at sampled point                              */
    float3 le;          /* emitted radiance                                             */
    float  dist;        /* distance from shading point to sampled point                */
    float  p_nee;       /* solid-angle PDF of the combined selection+point sample      */
    int    valid;
} light_sample_t;

/* Sample one point on the emissive light pool using area×luminance weighted selection.
 *
 * Selection: linear CDF walk over select_weight (matches CPU prepare_light_distribution).
 * Sphere lights: uniform surface sample → p_area = 1/area.
 * Triangle/mesh lights: area-weighted triangle selection + uniform barycentric sample
 *   → p_area = 1/total_area (constant, same as CPU uniform area sampling result).
 * Combined PDF: p_nee = (select_weight/total_select_weight) / area * dist² / cos_l
 *   — matches CPU p_light = p_select * p_area * dist² / cos_l.
 */
static light_sample_t sample_area_light(
    float3 shadow_orig,
    gpu_scene_header_t header,
    __global const gpu_emissive_t  *emissives,
    __global const gpu_sphere_t    *spheres,
    __global const gpu_triangle_t  *triangles,
    __global const gpu_material_t  *materials,
    pcg32_t *rng)
{
    light_sample_t ls; ls.valid = 0;
    if (header.emissive_count == 0 || header.total_select_weight <= 0.f) return ls;

    /* Weighted CDF selection. */
    float u_sel = pcg32_f(rng) * header.total_select_weight;
    float sel_accum = 0.f;
    uint li = header.emissive_count - 1;
    for (uint i = 0; i < header.emissive_count; ++i) {
        sel_accum += emissives[i].select_weight;
        if (sel_accum >= u_sel) { li = i; break; }
    }
    __global const gpu_emissive_t *em = &emissives[li];
    if (em->area <= 0.f || em->select_weight <= 0.f) return ls;

    float p_select = em->select_weight / header.total_select_weight;

    float3 p_light, light_n;

    if (em->type == GPU_EMISSIVE_SPHERE) {
        float3 lc = (float3)(spheres[em->geom_index].origin.x,
                             spheres[em->geom_index].origin.y,
                             spheres[em->geom_index].origin.z);
        float lr = spheres[em->geom_index].radius;
        float u1 = pcg32_f(rng), u2 = pcg32_f(rng);
        float cost = 1.f - 2.f * u1;
        float sint = sqrt(fmax(0.f, 1.f - cost*cost));
        float phi  = 6.28318530718f * u2;
        light_n = (float3)(sint*cos(phi), sint*sin(phi), cost);
        p_light = lc + light_n * lr;
    } else {
        /* Area-weighted triangle selection within the mesh. */
        float u_tri = pcg32_f(rng) * em->area;
        float tri_accum = 0.f;
        uint sel_tri = em->tri_base;
        for (uint ti = em->tri_base; ti < em->tri_base + em->tri_count; ++ti) {
            float3 tv0 = (float3)(triangles[ti].v0.x, triangles[ti].v0.y, triangles[ti].v0.z);
            float3 tv1 = (float3)(triangles[ti].v1.x, triangles[ti].v1.y, triangles[ti].v1.z);
            float3 tv2 = (float3)(triangles[ti].v2.x, triangles[ti].v2.y, triangles[ti].v2.z);
            tri_accum += 0.5f * length(cross(tv1 - tv0, tv2 - tv0));
            sel_tri = ti;
            if (tri_accum >= u_tri) break;
        }
        /* Uniform barycentric sample (Osada et al.). */
        float r1 = pcg32_f(rng), r2 = pcg32_f(rng);
        float su = sqrt(r1);
        float b0 = 1.f - su, b1 = su*(1.f - r2), b2 = su*r2;
        float3 tv0 = (float3)(triangles[sel_tri].v0.x, triangles[sel_tri].v0.y, triangles[sel_tri].v0.z);
        float3 tv1 = (float3)(triangles[sel_tri].v1.x, triangles[sel_tri].v1.y, triangles[sel_tri].v1.z);
        float3 tv2 = (float3)(triangles[sel_tri].v2.x, triangles[sel_tri].v2.y, triangles[sel_tri].v2.z);
        p_light = tv0*b0 + tv1*b1 + tv2*b2;
        float3 tn0 = (float3)(triangles[sel_tri].n0.x, triangles[sel_tri].n0.y, triangles[sel_tri].n0.z);
        float3 tn1 = (float3)(triangles[sel_tri].n1.x, triangles[sel_tri].n1.y, triangles[sel_tri].n1.z);
        float3 tn2 = (float3)(triangles[sel_tri].n2.x, triangles[sel_tri].n2.y, triangles[sel_tri].n2.z);
        float3 interp_n = tn0*b0 + tn1*b1 + tn2*b2;
        float nl = length(interp_n);
        light_n = nl > 1e-6f ? interp_n/nl
                             : f3norm(cross(tv1 - tv0, tv2 - tv0));
    }

    float3 L_vec = p_light - shadow_orig;
    float  dist2 = dot(L_vec, L_vec);
    if (dist2 < 1e-8f) return ls;
    float dist  = sqrt(dist2);
    float3 wi   = L_vec / dist;
    float cos_l = -dot(wi, light_n);
    if (cos_l <= 0.f) return ls;

    ls.wi      = wi;
    ls.light_n = light_n;
    ls.dist    = dist;
    ls.p_nee   = p_select / em->area * dist2 / cos_l;
    ls.le      = (float3)(materials[em->mat_index].albedo.x,
                          materials[em->mat_index].albedo.y,
                          materials[em->mat_index].albedo.z);
    ls.valid   = 1;
    return ls;
}

/* ---- Texture sampling ---- */

static float3 sample_texture(uint tex_idx,
                             float2 uv,
                             __global const gpu_tex_desc_t *tex_descs,
                             __global const float4         *pixels)
{
    __global const gpu_tex_desc_t *td = &tex_descs[tex_idx];
    if (td->width == 0 || td->height == 0) return (float3)(1,1,1);
    float fx = uv.x * (float)td->width  - 0.5f;
    float fy = uv.y * (float)td->height - 0.5f;
    int   x0 = (int)floor(fx);
    int   y0 = (int)floor(fy);
    float tx = fx - (float)x0;
    float ty = fy - (float)y0;
    int   w  = (int)td->width;
    int   h  = (int)td->height;
    /* Clamp addressing — matches CPU Texture2D default wrap mode. */
    int x1 = clamp(x0 + 1, 0, w - 1); x0 = clamp(x0, 0, w - 1);
    int y1 = clamp(y0 + 1, 0, h - 1); y0 = clamp(y0, 0, h - 1);
    uint off = td->pixel_offset;
    float4 p00 = pixels[off + y0 * w + x0];
    float4 p10 = pixels[off + y0 * w + x1];
    float4 p01 = pixels[off + y1 * w + x0];
    float4 p11 = pixels[off + y1 * w + x1];
    float4 px = mix(mix(p00, p10, tx), mix(p01, p11, tx), ty);
    return (float3)(px.x, px.y, px.z);
}

/* Sample single-channel (luminance) from a texture — for roughness/metallic maps. */
static float sample_texture_luma(uint tex_idx,
                                 float2 uv,
                                 __global const gpu_tex_desc_t *tex_descs,
                                 __global const float4         *pixels)
{
    float3 c = sample_texture(tex_idx, uv, tex_descs, pixels);
    return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
}

/* eval_sampler() is defined in sampler.cl; it must come after sample_texture (above)
 * and before the material helpers below that call it. */
#include "sampler.cl"

/* ---- Material eval ---- */
/* mat->albedo_tex / roughness_tex / metallic_tex / normal_tex are now sampler
 * indices into samplers[] rather than tex_descs[] indices.  eval_sampler()
 * dispatches to the appropriate procedural or texture type. */

static float3 material_albedo(__global const gpu_material_t *mat,
                              float2 uv, float3 pos,
                              __global const gpu_tex_desc_t *tex_descs,
                              __global const float4         *pixels,
                              __global const gpu_sampler_t  *samplers)
{
    float3 base = (float3)(mat->albedo.x, mat->albedo.y, mat->albedo.z);
    if (mat->albedo_tex >= 0)
        base *= eval_sampler(mat->albedo_tex, uv, pos, tex_descs, pixels, samplers);
    return base;
}

/* Decode tangent-space normal map and rotate into world space. */
static float3 apply_normal_map(float3 n, float2 uv, float3 pos, int normal_tex,
                               __global const gpu_tex_desc_t *tex_descs,
                               __global const float4         *pixels,
                               __global const gpu_sampler_t  *samplers)
{
    if (normal_tex < 0) return n;
    float3 tc  = eval_sampler(normal_tex, uv, pos, tex_descs, pixels, samplers);
    float3 ts  = (float3)(tc.x * 2.f - 1.f, tc.y * 2.f - 1.f, tc.z * 2.f - 1.f);
    float  tsl = length(ts);
    if (tsl < 1e-6f) return n;
    ts /= tsl;
    float3 t, b;
    tangent_basis(n, &t, &b);
    float bl = length(b);
    if (bl < 1e-6f) return n;
    b /= bl;
    float3 mapped = t * ts.x + b * ts.y + n * ts.z;
    float  ml = length(mapped);
    if (ml < 1e-6f || dot(mapped, n) <= 0.f) return n;
    return mapped / ml;
}

/* Principled roughness — sampler luma overrides scalar when present. */
static float material_roughness(__global const gpu_material_t *mat, float2 uv, float3 pos,
                                __global const gpu_tex_desc_t *tex_descs,
                                __global const float4         *pixels,
                                __global const gpu_sampler_t  *samplers)
{
    float r = mat->roughness;
    if (mat->roughness_tex >= 0) {
        float3 c = eval_sampler(mat->roughness_tex, uv, pos, tex_descs, pixels, samplers);
        r = 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
    }
    return clamp(r, 0.02f, 1.f);
}

/* Principled metallic — sampler luma overrides scalar when present. */
static float material_metallic(__global const gpu_material_t *mat, float2 uv, float3 pos,
                               __global const gpu_tex_desc_t *tex_descs,
                               __global const float4         *pixels,
                               __global const gpu_sampler_t  *samplers)
{
    float m = mat->metallic;
    if (mat->metallic_tex >= 0) {
        float3 c = eval_sampler(mat->metallic_tex, uv, pos, tex_descs, pixels, samplers);
        m = 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
    }
    return clamp(m, 0.f, 1.f);
}

/* ---- Camera ray generation ----
 *
 * Mirrors CPU Perspective::get_primary_ray() for both pinhole and thin-lens.
 *
 * Camera-space pinhole direction for pixel (px, py) with jitter:
 *   dir = (dx, dy, dz)  where dz = 1/tan(fov/2) = cam.half_h_over_d
 *
 * Thin-lens DoF (aperture > 0 && focal_length > 0):
 *   1. Sample lens disk: lens_pt = (lx, ly, 0) in camera space
 *   2. Focus point on focal plane: fp = pinhole_dir * (focal_length / dz)
 *   3. New camera-space dir = fp - lens_pt
 *   4. New camera-space origin = lens_pt
 *   Both are then rotated to world space by the camera basis (right/up/fwd).
 */
static void generate_ray(float3 *out_ro, float3 *out_rd,
                         gpu_camera_t cam, uint px, uint py,
                         uint width, uint height,
                         uint stratum, uint aa,
                         pcg32_t *rng)
{
    /* Stratified jitter: subdivide pixel into aa×aa cells; stratum selects cell. */
    float jx = ((float)(stratum % aa) + pcg32_f(rng)) / (float)aa - 0.5f;
    float jy = ((float)(stratum / aa) + pcg32_f(rng)) / (float)aa - 0.5f;
    float aspect = (float)width / (float)height;
    float dx = 2.f * ((float)px + 0.5f + jx) / (float)width  - 1.f;
    float dy = (2.f * ((float)py + 0.5f + jy) / (float)height - 1.f) / aspect;
    float dz = cam.half_h_over_d; /* = 1/tan(fov/2) */

    float3 right = (float3)(cam.right.x,   cam.right.y,   cam.right.z);
    float3 up    = (float3)(cam.up.x,      cam.up.y,      cam.up.z);
    float3 fwd   = (float3)(cam.forward.x, cam.forward.y, cam.forward.z);
    float3 pos   = (float3)(cam.position.x, cam.position.y, cam.position.z);

    if (cam.aperture > 0.f && cam.focal_length > 0.f) {
        /* Thin-lens: uniform disk sample on the aperture. */
        float u1 = pcg32_f(rng), u2 = pcg32_f(rng);
        float lr  = sqrt(u1) * cam.aperture * 0.5f;
        float lt  = 6.28318530718f * u2;
        float lx  = lr * cos(lt);
        float ly  = lr * sin(lt);

        /* Focus point: pinhole ray evaluated at the focal plane (z = focal_length). */
        float t_focus = cam.focal_length / dz;
        float fx = dx * t_focus;
        float fy = dy * t_focus;
        float fz = cam.focal_length;  /* dz * t_focus */

        /* New camera-space direction from lens point to focus point. */
        float ndx = fx - lx;
        float ndy = fy - ly;
        float ndz = fz;

        *out_ro = pos + right * lx + up * ly;
        *out_rd = f3norm(right * ndx + up * ndy + fwd * ndz);
    } else {
        /* Pinhole */
        *out_ro = pos;
        *out_rd = f3norm(right * dx + up * dy + fwd * dz);
    }
}

/* ---- Material shader interface ----
 *
 * Each shade_XYZ() function implements one material type.
 * MAT_SCENE_PARAMS / MAT_SCENE_ARGS pass the full scene context without repeating
 * the 14-argument list at every call site and function declaration.
 */
#define MAT_SCENE_PARAMS \
    gpu_scene_header_t                     header,   \
    __global const gpu_tlas_node_t    *tlas_nodes,  \
    __global const gpu_tlas_item_t    *tlas_items,  \
    __global const gpu_infinite_obj_t *infinite,    \
    __global const gpu_object_t       *objects,     \
    __global const gpu_bvh_node_t     *bvh_nodes,   \
    __global const gpu_triangle_t     *triangles,   \
    __global const gpu_sphere_t       *spheres,     \
    __global const gpu_plane_t        *planes,      \
    __global const gpu_fractal_t      *fractals,    \
    __global const gpu_emissive_t     *emissives,   \
    __global const gpu_material_t     *materials,   \
    __global const gpu_tex_desc_t     *tex_descs,   \
    __global const float4             *pixels,      \
    __global const gpu_sampler_t      *samplers

#define MAT_SCENE_ARGS \
    header, tlas_nodes, tlas_items, infinite, objects, bvh_nodes, \
    triangles, spheres, planes, fractals, emissives, materials, tex_descs, pixels, samplers

#include "mat_lambert.cl"
#include "mat_dielectric.cl"
#include "mat_principled.cl"
#include "mat_subsurface.cl"
#include "mat_rough_dielectric.cl"
#include "mat_phong.cl"
#include "mat_sheen.cl"
#include "mat_thin_dielectric.cl"
#include "mat_thin_translucent.cl"
#include "mat_boundary.cl"

/* ---- Main kernel ---- */

__kernel void pathtrace(
    gpu_scene_header_t                     header,
    __global const gpu_tlas_node_t    *restrict tlas_nodes,
    __global const gpu_tlas_item_t    *restrict tlas_items,
    __global const gpu_infinite_obj_t *restrict infinite,
    __global const gpu_bvh_node_t     *restrict bvh_nodes,
    __global const gpu_triangle_t     *restrict triangles,
    __global const gpu_sphere_t       *restrict spheres,
    __global const gpu_plane_t        *restrict planes,
    __global const gpu_fractal_t      *restrict fractals,
    __global const gpu_object_t       *restrict objects,
    __global const gpu_material_t     *restrict materials,
    __global const gpu_tex_desc_t     *restrict tex_descs,
    __global const float4             *restrict pixels,
    gpu_camera_t                               camera,
    uint                                       sample_offset,
    uint                                       max_depth,
    __global       float4             *restrict accum,
    uint                                       width,
    uint                                       height,
    uint                                       aa,
    __global const gpu_emissive_t     *restrict emissives,
    __global const gpu_sampler_t      *restrict samplers,
    uint                                       num_samples)
{
    uint px = get_global_id(0);
    uint py = get_global_id(1);
    if (px >= width || py >= height) return;

    uint pixel_id = py * width + px;
    pcg32_t rng;
    uint aa_sq = aa * aa;

    float3 colour_sum = (float3)(0.f, 0.f, 0.f);
    for (uint si = 0; si < num_samples; ++si) {
    for (uint s = 0; s < aa_sq; ++s) {

    /* Each stratum gets its own independent RNG stream. */
    pcg32_seed(&rng, pixel_id, (sample_offset + si) * aa_sq + s);

    float3 ro, rd;
    generate_ray(&ro, &rd, camera, px, py, width, height, s, aa, &rng);

    float3 throughput  = (float3)(1,1,1);
    float3 colour      = (float3)(0,0,0);
    bool   prev_delta  = true;  /* primary ray: no NEE at previous bounce */
    float  prev_bsdf_pdf = 0.f; /* cosine-hemisphere PDF from previous diffuse scatter */

    for (uint depth = 0; depth < max_depth; ++depth) {
        hit_t hit = scene_intersect(ro, rd,
            header, tlas_nodes, tlas_items, infinite,
            objects, bvh_nodes, triangles, spheres, planes, fractals);

        if (!hit.hit) {
            /* Sky MIS: both NEE-sky and BSDF use cosine hemisphere ⇒ PDFs cancel ⇒ weight=0.5.
               Primary/specular rays have no complementary NEE sample ⇒ weight=1. */
            float mis_w = prev_delta ? 1.f : 0.5f;
            colour += throughput * eval_sky(rd, header) * mis_w;
            break;
        }

        if (hit.mat_index < 0 || hit.mat_index >= (int)header.material_count) break;
        __global const gpu_material_t *mat = &materials[hit.mat_index];

        if (mat->type == GPU_MAT_EMISSIVE) {
            float3 le = (float3)(mat->albedo.x, mat->albedo.y, mat->albedo.z);
            float  mis_w = 1.f;

            /* MIS correction: find this object in the emissive list and compute the NEE
               PDF that would have sampled this hit point, then apply power heuristic.
               Matches by object_index (not mat_index) so distinct mesh lights sharing a
               material are handled correctly — same as CPU m_light_select_pdf lookup. */
            if (!prev_delta && mat->nee_emissive && header.emissive_count > 0
                && prev_bsdf_pdf > 0.f && header.total_select_weight > 0.f) {
                for (uint li = 0; li < header.emissive_count; ++li) {
                    if (emissives[li].object_index == hit.object_index) {
                        float cos_l = fmax(0.f, -dot(rd, hit.normal));
                        if (cos_l > 1e-6f && emissives[li].area > 0.f) {
                            float dist2    = hit.t * hit.t;
                            float p_select = emissives[li].select_weight / header.total_select_weight;
                            float p_nee    = p_select / emissives[li].area * dist2 / cos_l;
                            mis_w = power_heuristic(prev_bsdf_pdf, p_nee);
                        }
                        break;
                    }
                }
            }

            colour += throughput * le * mis_w;
            break;
        }

        float3 albedo = material_albedo(mat, hit.uv, hit.pos, tex_descs, pixels, samplers);

        bool mat_handled = true;
        switch (mat->type) {
            case GPU_MAT_LAMBERT:
                shade_lambert(MAT_SCENE_ARGS, mat, hit, albedo,
                              &colour, &throughput, &ro, &rd, &prev_bsdf_pdf, &prev_delta, &rng);
                break;
            case GPU_MAT_DIELECTRIC:
                shade_dielectric(mat, hit,
                                 &throughput, &ro, &rd, &prev_bsdf_pdf, &prev_delta, &rng);
                break;
            case GPU_MAT_PRINCIPLED:
                shade_principled(MAT_SCENE_ARGS, mat, hit, albedo,
                                 &colour, &throughput, &ro, &rd, &prev_bsdf_pdf, &prev_delta, &rng);
                break;
            case GPU_MAT_SUBSURFACE:
                shade_subsurface(MAT_SCENE_ARGS, mat, hit, albedo,
                                 &colour, &throughput, &ro, &rd, &prev_bsdf_pdf, &prev_delta, &rng);
                break;
            case GPU_MAT_ROUGH_DIELECTRIC:
                shade_rough_dielectric(MAT_SCENE_ARGS, mat, hit,
                                       &throughput, &ro, &rd, &prev_bsdf_pdf, &prev_delta, &rng);
                break;
            case GPU_MAT_PHONG:
            case GPU_MAT_BLINNPHONG:
                shade_phong(MAT_SCENE_ARGS, mat, hit, albedo,
                            &colour, &throughput, &ro, &rd, &prev_bsdf_pdf, &prev_delta, &rng);
                break;
            case GPU_MAT_SHEEN:
                shade_sheen(MAT_SCENE_ARGS, mat, hit, albedo,
                            &colour, &throughput, &ro, &rd, &prev_bsdf_pdf, &prev_delta, &rng);
                break;
            case GPU_MAT_THIN_DIELECTRIC:
                shade_thin_dielectric(MAT_SCENE_ARGS, mat, hit,
                                      &throughput, &ro, &rd, &prev_bsdf_pdf, &prev_delta, &rng);
                break;
            case GPU_MAT_THIN_TRANSLUCENT:
                shade_thin_translucent(MAT_SCENE_ARGS, mat, hit, albedo,
                                       &colour, &throughput, &ro, &rd, &prev_bsdf_pdf, &prev_delta, &rng);
                break;
            case GPU_MAT_BOUNDARY:
                shade_boundary(mat, hit, &throughput, &ro, &rd, &prev_bsdf_pdf, &prev_delta);
                break;
            default:
                mat_handled = false;
                break;
        }
        if (!mat_handled) break;

        /* Russian roulette after 3 bounces. */
        if (depth > 2) {
            float p = fmax(throughput.x, fmax(throughput.y, throughput.z));
            if (pcg32_f(&rng) > p) break;
            throughput /= p;
        }
    }

    colour_sum += colour;
    } /* end stratum loop */
    } /* end sample loop */

    float inv = 1.f / (float)(num_samples * aa_sq);
    accum[pixel_id] = (float4)(colour_sum.x * inv, colour_sum.y * inv, colour_sum.z * inv, 0.f);
}
