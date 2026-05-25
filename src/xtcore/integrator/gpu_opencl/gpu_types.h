/*
 * gpu_types.h — POD structs shared between the host serializer and the OpenCL kernel.
 *
 * Rules:
 *  - All types are C99-compatible (no C++ classes, no templates, no namespaces).
 *  - All fields use fixed-width base types so the host and device layouts match.
 *  - Structs are explicitly padded to 16-byte boundaries where needed to avoid
 *    OpenCL implementation-defined padding differences.
 */

#ifndef XTCORE_GPU_OPENCL_TYPES_H_INCLUDED
#define XTCORE_GPU_OPENCL_TYPES_H_INCLUDED

#ifdef __OPENCL_VERSION__
/* Inside the kernel — use OpenCL built-in types */
typedef float3  gpu_float3;
typedef float4  gpu_float4;
typedef uint    gpu_uint;
typedef int     gpu_int;
typedef ulong   gpu_ulong;
#else
/* Host side — define matching POD types */
#include <stdint.h>
typedef struct { float x, y, z;       } gpu_float3;
typedef struct { float x, y, z, w;    } gpu_float4;
typedef uint32_t  gpu_uint;
typedef int32_t   gpu_int;
typedef uint64_t  gpu_ulong;
#endif

/* ---- AABB ---- */
typedef struct {
    gpu_float3 mn;
    float      _pad0;
    gpu_float3 mx;
    float      _pad1;
} gpu_aabb_t;

/* ---- TLAS ---- */
typedef struct {
    gpu_aabb_t aabb;
    gpu_int    left;
    gpu_int    right;
    gpu_uint   first;
    gpu_uint   count;
} gpu_tlas_node_t;

typedef struct {
    gpu_aabb_t aabb;
    gpu_ulong  object_id;
    gpu_uint   object_index; /* index into gpu_object_t array */
    gpu_uint   _pad;
} gpu_tlas_item_t;

/* Infinite objects (planes, etc.) stored separately */
typedef struct {
    gpu_uint   object_index;
    gpu_uint   _pad[3];
} gpu_infinite_obj_t;

/* ---- Mesh BVH ---- */
typedef struct {
    gpu_aabb_t aabb;
    gpu_uint   left;
    gpu_uint   right;
    gpu_uint   first; /* index into triangle array (already permuted) */
    gpu_uint   count;
} gpu_bvh_node_t;

/* ---- Geometry ---- */
typedef struct {
    gpu_float3 v0, v1, v2;
    gpu_float3 n0, n1, n2;
    float      uv0[2], uv1[2], uv2[2];
    float      _pad[2];
} gpu_triangle_t;

typedef struct {
    gpu_float3 origin;
    float      radius;
} gpu_sphere_t;

typedef struct {
    gpu_float3 normal;
    float      offset;
} gpu_plane_t;

/* ---- Fractal geometry ---- */
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
    gpu_float3 param_vec;  /* julia_c / orientation / quat_c */
    float      param_w;    /* quat_cw for QuaternionJulia    */
    float      param_w0;   /* quat_w0 for QuaternionJulia    */
    float      radius;
    float      power;
    float      bailout;
    float      fold_size;  /* MandelBox                      */
    float      min_r;      /* MandelBox                      */
    float      scale_f;    /* MandelBox / IcosahedralIFS     */
    gpu_int    iterations;
    gpu_int    orbit_trap_channel; /* -1 = none              */
    float      _pad[2];
} gpu_fractal_t;

/* ---- Object (maps TLAS leaf → geometry + material) ---- */
#define GPU_GEOM_MESH    0
#define GPU_GEOM_SPHERE  1
#define GPU_GEOM_PLANE   2
#define GPU_GEOM_FRACTAL 3

typedef struct {
    gpu_int  geom_type;
    gpu_uint geom_index;  /* index into sphere/plane/fractal buffer; mesh uses tri/bvh bases */
    gpu_uint mat_index;
    gpu_uint tri_base;    /* meshes: first triangle in the global triangle buffer */
    gpu_uint bvh_base;    /* meshes: first bvh_node in the global bvh_node buffer */
    gpu_uint _pad[3];
} gpu_object_t;

/* ---- Materials ---- */
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
    gpu_float3 albedo;           /* base colour (all types) or emission colour */
    float      roughness;        /* Principled / RoughDielectric */
    float      ior;              /* Dielectric / Principled */
    gpu_int    albedo_tex;       /* index into tex_desc array; -1 = use albedo directly */
    gpu_int    nee_emissive;     /* 1 if this emissive has a sphere NEE entry */
    /* Principled extra — packed to keep struct at 64 bytes */
    float      metallic;
    float      anisotropy;
    float      anisotropy_rotation; /* degrees */
    float      clearcoat;
    float      clearcoat_roughness;
    gpu_int    normal_tex;          /* -1 = no normal map          */
    gpu_int    roughness_tex;       /* -1 = use scalar roughness   */
    gpu_int    metallic_tex;        /* -1 = use scalar metallic    */
} gpu_material_t;

/* ---- Textures ---- */
typedef struct {
    gpu_uint width;
    gpu_uint height;
    gpu_uint pixel_offset; /* offset (in float4 units) into the global pixel buffer */
    gpu_uint _pad;
} gpu_tex_desc_t;

/* ---- Samplers ---- */
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

/*
 * gpu_sampler_t — 128-byte unified sampler descriptor.
 *
 * Field semantics per type (fields not listed are unused):
 *   SOLID_COLOR:     color_a
 *   TEXTURE:         tex_index
 *   CHECKER:         color_a, color_b, scale_u, scale_v, offset_u, offset_v; i_param0=swap_colors
 *   BRICK:           color_a=brick, color_b=mortar; scale_u, scale_v;
 *                    param0=mortar_u, param1=mortar_v, param2=color_variation; i_param0=seed
 *   DOTS:            color_a=bg, color_b=dot; scale_u=scale, param0=radius, param1=softness
 *   GRAPHPAPER:      color_a=base, color_b=minor, color_c=major; scale_u=scale;
 *                    param0=minor_width, param1=major_width; i_param0=major_every
 *   STARS:           color_a=background; scale_u=density;
 *                    param0=min_brightness, param1=max_brightness, param2=star_size; i_param0=seed
 *   WEAVE:           color_a=base, color_b=warp, color_c=weft; scale_u=scale, param0=band_width
 *   VORONOI_NORMAL:  param0=max_deviation; i_param0=cells, i_param1=seed
 *   FBM_MARBLE:      color_a, color_b, color_c=vein_color; scale_u=scale;
 *                    param0=vein_frequency, param1=turbulence, param2=lacunarity, param3=gain,
 *                    param4=vein_strength, param5=vein_sharpness; i_param0=octaves
 *   FBM_WOOD:        color_a, color_b; scale_u=scale;
 *                    param0=ring_frequency, param1=turbulence, param2=lacunarity, param3=gain;
 *                    i_param0=octaves
 *   CURL_NOISE:      color_a, color_b; scale_u=scale;
 *                    param0=strength, param1=lacunarity, param2=gain; i_param0=octaves
 *   SCRATCHES:       color_a=base, color_b=scratch; scale_u=scale;
 *                    param0=width, param1=angle, param2=angle_jitter; i_param0=density, i_param1=seed
 *   EDGE_WEAR:       color_a=base, color_b=worn; scale_u=scale;
 *                    param0=sharpness, param1=coverage; i_param0=seed
 *   BLEND:           child_a, child_b; param0=t
 *   MIX_MASKED:      child_a=base, child_b=overlay, child_c=mask; param0=t; i_param0=mode
 *                    (mode: 0=LERP, 1=MULTIPLY, 2=ADD, 3=SCREEN)
 *   TRIPLANAR:       child_a=child; scale_u=scale, param0=blend_sharpness
 *   GRADIENT:        color_a, color_b  (lerps on uv.y in [0,1])
 */
typedef struct {
    gpu_int    type;       /* GPU_SAMPLER_* */
    gpu_int    child_a;    /* composite: sampler index or -1 */
    gpu_int    child_b;
    gpu_int    child_c;    /* MIX_MASKED mask; -1 = no mask */
    gpu_float4 color_a;    /* primary color */
    gpu_float4 color_b;    /* secondary color */
    gpu_float4 color_c;    /* tertiary color (Weave weft, GraphPaper major, FBMMarble vein) */
    float      scale_u;
    float      scale_v;
    float      offset_u;
    float      offset_v;
    float      param0;
    float      param1;
    float      param2;
    float      param3;
    float      param4;
    float      param5;
    gpu_int    i_param0;
    gpu_int    i_param1;
    gpu_int    tex_index;  /* GPU_SAMPLER_TEXTURE: index into tex_descs[] */
    gpu_int    _pad0;
    gpu_int    _pad1;
    gpu_int    _pad2;
} gpu_sampler_t;
/* static_assert: sizeof(gpu_sampler_t) == 128 */

/* ---- Camera (perspective only for phase 1) ---- */
typedef struct {
    gpu_float3 position;
    float      _pad0;
    gpu_float3 forward;
    float      _pad1;
    gpu_float3 right;
    float      _pad2;
    gpu_float3 up;
    float      _pad3;
    float      half_w_over_d; /* unused (reserved)                  */
    float      half_h_over_d; /* focal distance D = 1/tan(fov_h/2)  */
    float      aperture;
    float      focal_length;
} gpu_camera_t;

/* ---- Emissive area light (GPU NEE — sphere or triangle/mesh) ---- */
#define GPU_EMISSIVE_SPHERE   0
#define GPU_EMISSIVE_TRIANGLE 1  /* standalone Triangle or Mesh (both stored as tri list) */

typedef struct {
    gpu_uint  object_index;    /* index into objects[]                              */
    gpu_uint  mat_index;       /* index into materials[]                            */
    gpu_int   type;            /* GPU_EMISSIVE_SPHERE or GPU_EMISSIVE_TRIANGLE       */
    gpu_uint  geom_index;      /* sphere only: index into spheres[]                 */
    gpu_uint  tri_base;        /* triangle/mesh only: first triangle in triangles[] */
    gpu_uint  tri_count;       /* triangle/mesh only: number of triangles           */
    float     area;            /* total surface area (4πr² or sum of tri areas)     */
    float     select_weight;   /* area × luminance — used for weighted CDF selection */
} gpu_emissive_t;

/* ---- Scene header ---- */
typedef struct {
    gpu_uint  tlas_root;
    gpu_uint  tlas_node_count;
    gpu_uint  tlas_item_count;
    gpu_uint  object_count;
    gpu_uint  infinite_count;
    gpu_uint  triangle_count;
    gpu_uint  bvh_node_count;
    gpu_uint  sphere_count;
    gpu_uint  plane_count;
    gpu_uint  fractal_count;
    gpu_uint  material_count;
    gpu_uint  emissive_count;        /* total emissive NEE lights (all types) */
    float     total_select_weight;   /* sum of emissive select_weights        */
    gpu_int   env_tex;           /* -1 = gradient, >= 0 = texture index */
    float     env_intensity;
    float     env_sky_r,  env_sky_g,  env_sky_b;   /* upper-hemisphere gradient colour */
    float     env_gnd_r,  env_gnd_g,  env_gnd_b;   /* lower-hemisphere gradient colour */
} gpu_scene_header_t;

#endif /* XTCORE_GPU_OPENCL_TYPES_H_INCLUDED */
