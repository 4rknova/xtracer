#ifdef XTCORE_ENABLE_OPENCL

#include <cstring>
#include <cmath>
#include <typeinfo>
#include <nimg/luminance.h>
#include <xtcore/log.h>

#include <xtcore/scene.h>
#include <xtcore/mesh.h>
#include <xtcore/math/sphere.h>
#include <xtcore/math/plane.h>
#include <xtcore/math/fractal.h>
#include <xtcore/math/triangle.h>
#include <xtcore/material.h>
#include <xtcore/material/lambert.h>
#include <xtcore/material/emissive.h>
#include <xtcore/material/dielectric.h>
#include <xtcore/material/rough_dielectric.h>
#include <xtcore/material/principled.h>
#include <xtcore/material/subsurface.h>
#include <xtcore/material/phong.h>
#include <xtcore/material/blinnphong.h>
#include <xtcore/material/sheen.h>
#include <xtcore/material/thin_dielectric.h>
#include <xtcore/material/thin_translucent.h>
#include <xtcore/material/boundary.h>
#include <xtcore/sampler/sampler_tex.h>
#include <xtcore/sampler/sampler_col.h>
#include <xtcore/sampler/sampler_gradient.h>
#include <xtcore/sampler/sampler_checker.h>
#include <xtcore/sampler/sampler_brick.h>
#include <xtcore/sampler/sampler_dots.h>
#include <xtcore/sampler/sampler_graphpaper.h>
#include <xtcore/sampler/sampler_stars.h>
#include <xtcore/sampler/sampler_weave.h>
#include <xtcore/sampler/sampler_voronoi_normal.h>
#include <xtcore/sampler/sampler_fbm_marble.h>
#include <xtcore/sampler/sampler_fbm_wood.h>
#include <xtcore/sampler/sampler_curl_noise.h>
#include <xtcore/sampler/sampler_scratches.h>
#include <xtcore/sampler/sampler_edge_wear.h>
#include <xtcore/sampler/sampler_blend.h>
#include <xtcore/sampler/sampler_mix_masked.h>
#include <xtcore/sampler/sampler_triplanar.h>
#include <xtcore/camera/perspective.h>
#include <xtcore/matdefs.h>

#include "scene_serializer.h"

namespace xtcore {
    namespace integrator {
        namespace gpu_opencl {

/* ---- helpers ---- */

static gpu_float3 v3(float x, float y, float z) { gpu_float3 r; r.x=x; r.y=y; r.z=z; return r; }
static gpu_float3 from_vec(const nmath::Vector3f &v) { return v3((float)v.x, (float)v.y, (float)v.z); }

static gpu_aabb_t from_aabb(const AABB3 &a)
{
    gpu_aabb_t r;
    r.mn = from_vec(a.min); r._pad0 = 0.f;
    r.mx = from_vec(a.max); r._pad1 = 0.f;
    return r;
}

/* ---- material / texture ---- */

/* Retrieve a named sampler from a material by iterating the index-based API. */
static const xtcore::sampler::ISampler *get_named_sampler(
    const xtcore::asset::IMaterial *mat, const char *name)
{
    size_t n = mat->get_sampler_count();
    for (size_t i = 0; i < n; ++i) {
        std::string slot;
        xtcore::sampler::ISampler *s =
            const_cast<xtcore::asset::IMaterial*>(mat)->get_sampler_by_index(i, &slot);
        if (slot == name) return s;
    }
    return nullptr;
}

/* Upload a Texture2D to tex_descs/pixels and return its tex_descs index.
 * Returns (uint32_t)-1 if the texture is invalid. */
uint32_t SceneSerializer::add_texture_desc(const xtcore::sampler::Texture2D *tex)
{
    if (!tex || tex->width() == 0 || tex->height() == 0) return (uint32_t)-1;

    gpu_tex_desc_t desc;
    desc.width        = (gpu_uint)tex->width();
    desc.height       = (gpu_uint)tex->height();
    desc.pixel_offset = (gpu_uint)pixels.size();
    desc._pad         = 0;

    for (size_t y = 0; y < tex->height(); ++y) {
        for (size_t x = 0; x < tex->width(); ++x) {
            const nimg::ColorRGBAf &p = tex->pixel_ro(x, y);
            cl_float4 px = {{p.r(), p.g(), p.b(), p.a()}};
            pixels.push_back(px);
        }
    }

    uint32_t idx = (uint32_t)tex_descs.size();
    tex_descs.push_back(desc);
    return idx;
}

/* Helper: make a zero-initialised gpu_sampler_t with a given type. */
static gpu_sampler_t make_sampler(gpu_int type)
{
    gpu_sampler_t s;
    memset(&s, 0, sizeof(s));
    s.type    = type;
    s.child_a = -1; s.child_b = -1; s.child_c = -1;
    s.tex_index = -1;
    return s;
}

static gpu_float4 rgb_to_f4(const nimg::ColorRGBf &c)
{
    gpu_float4 f; f.x = c.r(); f.y = c.g(); f.z = c.b(); f.w = 1.f;
    return f;
}

/* Serialize any ISampler to the samplers[] array. Returns the sampler index,
 * or (uint32_t)-1 if the sampler is null or unrecognised.
 * Composite samplers (Blend, MixMasked, Triplanar) recurse one level deep. */
uint32_t SceneSerializer::add_sampler(const xtcore::sampler::ISampler *s)
{
    if (!s) return (uint32_t)-1;

    /* ---- Texture2D ---- */
    if (const auto *tex = dynamic_cast<const xtcore::sampler::Texture2D *>(s)) {
        uint32_t tex_idx = add_texture_desc(tex);
        if (tex_idx == (uint32_t)-1) return (uint32_t)-1;
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_TEXTURE);
        gs.tex_index = (gpu_int)tex_idx;
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- SolidColor: caller handles inline; we never reach this path for
     *      albedo slots, but support it in case it is used as a child. ---- */
    if (const auto *sc = dynamic_cast<const xtcore::sampler::SolidColor *>(s)) {
        nmath::Vector3f tc(0, 0, 0);
        nimg::ColorRGBf c = const_cast<xtcore::sampler::SolidColor*>(sc)->sample(tc);
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_SOLID_COLOR);
        gs.color_a = rgb_to_f4(c);
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- Gradient ---- */
    if (const auto *g = dynamic_cast<const xtcore::sampler::Gradient *>(s)) {
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_GRADIENT);
        gs.color_a = rgb_to_f4(g->a);
        gs.color_b = rgb_to_f4(g->b);
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- Checker ---- */
    if (const auto *c = dynamic_cast<const xtcore::sampler::Checker *>(s)) {
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_CHECKER);
        gs.color_a  = rgb_to_f4(c->color_a);
        gs.color_b  = rgb_to_f4(c->color_b);
        gs.scale_u  = (float)c->scale_u;
        gs.scale_v  = (float)c->scale_v;
        gs.offset_u = (float)c->offset_u;
        gs.offset_v = (float)c->offset_v;
        gs.i_param0 = c->swap_colors ? 1 : 0;
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- Brick ---- */
    if (const auto *b = dynamic_cast<const xtcore::sampler::Brick *>(s)) {
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_BRICK);
        gs.color_a  = rgb_to_f4(b->color_brick);
        gs.color_b  = rgb_to_f4(b->color_mortar);
        gs.scale_u  = (float)b->scale_u;
        gs.scale_v  = (float)b->scale_v;
        gs.param0   = (float)b->mortar_u;
        gs.param1   = (float)b->mortar_v;
        gs.param2   = (float)b->color_variation;
        gs.i_param0 = b->seed;
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- Dots ---- */
    if (const auto *d = dynamic_cast<const xtcore::sampler::Dots *>(s)) {
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_DOTS);
        gs.color_a  = rgb_to_f4(d->color_bg);
        gs.color_b  = rgb_to_f4(d->color_dot);
        gs.scale_u  = (float)d->scale;
        gs.param0   = (float)d->radius;
        gs.param1   = (float)d->softness;
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- GraphPaper ---- */
    if (const auto *gp = dynamic_cast<const xtcore::sampler::GraphPaper *>(s)) {
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_GRAPHPAPER);
        gs.color_a  = rgb_to_f4(gp->base_color);
        gs.color_b  = rgb_to_f4(gp->minor_color);
        gs.color_c  = rgb_to_f4(gp->major_color);
        gs.scale_u  = (float)gp->scale;
        gs.param0   = (float)gp->minor_width;
        gs.param1   = (float)gp->major_width;
        gs.i_param0 = gp->major_every;
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- Stars ---- */
    if (const auto *st = dynamic_cast<const xtcore::sampler::Stars *>(s)) {
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_STARS);
        gs.color_a  = rgb_to_f4(st->background_color);
        gs.scale_u  = (float)st->density;
        gs.param0   = (float)st->min_brightness;
        gs.param1   = (float)st->max_brightness;
        gs.param2   = (float)st->star_size;
        gs.i_param0 = st->seed;
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- Weave ---- */
    if (const auto *w = dynamic_cast<const xtcore::sampler::Weave *>(s)) {
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_WEAVE);
        gs.color_a  = rgb_to_f4(w->base_color);
        gs.color_b  = rgb_to_f4(w->warp_color);
        gs.color_c  = rgb_to_f4(w->weft_color);
        gs.scale_u  = (float)w->scale;
        gs.param0   = (float)w->band_width;
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- VoronoiNormal ---- */
    if (const auto *vn = dynamic_cast<const xtcore::sampler::VoronoiNormal *>(s)) {
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_VORONOI_NORMAL);
        gs.param0   = (float)vn->max_deviation;
        gs.i_param0 = vn->cells;
        gs.i_param1 = vn->seed;
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- FBMMarble ---- */
    if (const auto *fm = dynamic_cast<const xtcore::sampler::FBMMarble *>(s)) {
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_FBM_MARBLE);
        gs.color_a  = rgb_to_f4(fm->color_a);
        gs.color_b  = rgb_to_f4(fm->color_b);
        gs.color_c  = rgb_to_f4(fm->vein_color);
        gs.scale_u  = (float)fm->scale;
        gs.param0   = (float)fm->vein_frequency;
        gs.param1   = (float)fm->turbulence;
        gs.param2   = (float)fm->lacunarity;
        gs.param3   = (float)fm->gain;
        gs.param4   = (float)fm->vein_strength;
        gs.param5   = (float)fm->vein_sharpness;
        gs.i_param0 = fm->octaves;
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- FBMWood ---- */
    if (const auto *fw = dynamic_cast<const xtcore::sampler::FBMWood *>(s)) {
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_FBM_WOOD);
        gs.color_a  = rgb_to_f4(fw->color_a);
        gs.color_b  = rgb_to_f4(fw->color_b);
        gs.scale_u  = (float)fw->scale;
        gs.param0   = (float)fw->ring_frequency;
        gs.param1   = (float)fw->turbulence;
        gs.param2   = (float)fw->lacunarity;
        gs.param3   = (float)fw->gain;
        gs.i_param0 = fw->octaves;
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- CurlNoise ---- */
    if (const auto *cn = dynamic_cast<const xtcore::sampler::CurlNoise *>(s)) {
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_CURL_NOISE);
        gs.color_a  = rgb_to_f4(cn->color_a);
        gs.color_b  = rgb_to_f4(cn->color_b);
        gs.scale_u  = (float)cn->scale;
        gs.param0   = (float)cn->strength;
        gs.param1   = (float)cn->lacunarity;
        gs.param2   = (float)cn->gain;
        gs.i_param0 = cn->octaves;
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- Scratches ---- */
    if (const auto *sc = dynamic_cast<const xtcore::sampler::Scratches *>(s)) {
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_SCRATCHES);
        gs.color_a  = rgb_to_f4(sc->color_base);
        gs.color_b  = rgb_to_f4(sc->color_scratch);
        gs.scale_u  = (float)sc->scale;
        gs.param0   = (float)sc->width;
        gs.param1   = (float)sc->angle;
        gs.param2   = (float)sc->angle_jitter;
        gs.i_param0 = sc->density;
        gs.i_param1 = sc->seed;
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- EdgeWear ---- */
    if (const auto *ew = dynamic_cast<const xtcore::sampler::EdgeWear *>(s)) {
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_EDGE_WEAR);
        gs.color_a  = rgb_to_f4(ew->color_base);
        gs.color_b  = rgb_to_f4(ew->color_worn);
        gs.scale_u  = (float)ew->scale;
        gs.param0   = (float)ew->sharpness;
        gs.param1   = (float)ew->coverage;
        gs.i_param0 = ew->seed;
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- Blend ---- */
    if (const auto *bl = dynamic_cast<const xtcore::sampler::Blend *>(s)) {
        /* Children must be serialized before the parent so indices are valid. */
        uint32_t ca = add_sampler(bl->a);
        uint32_t cb = add_sampler(bl->b);
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_BLEND);
        gs.child_a  = (ca != (uint32_t)-1) ? (gpu_int)ca : -1;
        gs.child_b  = (cb != (uint32_t)-1) ? (gpu_int)cb : -1;
        gs.param0   = (float)bl->t;
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- MixMasked ---- */
    if (const auto *mm = dynamic_cast<const xtcore::sampler::MixMasked *>(s)) {
        uint32_t ca = add_sampler(mm->base);
        uint32_t cb = add_sampler(mm->overlay);
        uint32_t cc = add_sampler(mm->mask);
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_MIX_MASKED);
        gs.child_a  = (ca != (uint32_t)-1) ? (gpu_int)ca : -1;
        gs.child_b  = (cb != (uint32_t)-1) ? (gpu_int)cb : -1;
        gs.child_c  = (cc != (uint32_t)-1) ? (gpu_int)cc : -1;
        gs.param0   = (float)mm->t;
        gs.i_param0 = (gpu_int)mm->mode; /* MODE_LERP=0, MULTIPLY=1, ADD=2, SCREEN=3 */
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* ---- Triplanar ---- */
    if (const auto *tp = dynamic_cast<const xtcore::sampler::Triplanar *>(s)) {
        uint32_t ca = add_sampler(tp->child);
        gpu_sampler_t gs = make_sampler(GPU_SAMPLER_TRIPLANAR);
        gs.child_a  = (ca != (uint32_t)-1) ? (gpu_int)ca : -1;
        gs.scale_u  = (float)tp->scale;
        gs.param0   = (float)tp->blend_sharpness;
        uint32_t idx = (uint32_t)samplers.size();
        samplers.push_back(gs);
        return idx;
    }

    /* Unrecognised sampler type — not representable on GPU. */
    xtcore::Log::handle().post_warning("[gpu_opencl] unrecognised sampler type '%s' — ignored",
                                       typeid(*s).name());
    return (uint32_t)-1;
}

uint32_t SceneSerializer::get_or_add_material(const xtcore::asset::IMaterial *mat)
{
    if (!mat) {
        // Fallback: white Lambert
        gpu_material_t m;
        memset(&m, 0, sizeof(m));
        m.type          = GPU_MAT_LAMBERT;
        m.albedo        = v3(0.8f, 0.8f, 0.8f);
        m.albedo_tex    = -1;
        m.normal_tex    = -1;
        m.roughness_tex = -1;
        m.metallic_tex  = -1;
        uint32_t idx = (uint32_t)materials.size();
        materials.push_back(m);
        return idx;
    }

    auto it = m_mat_index.find(mat);
    if (it != m_mat_index.end()) return it->second;

    gpu_material_t m;
    memset(&m, 0, sizeof(m));
    m.albedo_tex    = -1;
    m.normal_tex    = -1;
    m.roughness_tex = -1;
    m.metallic_tex  = -1;

    if (dynamic_cast<const xtcore::asset::material::Lambert *>(mat)) {
        m.type = GPU_MAT_LAMBERT;
        if (mat->has_sampler(MAT_SAMPLER_DIFFUSE)) {
            const xtcore::sampler::ISampler *s = const_cast<xtcore::asset::IMaterial*>(mat)
                ->get_sampler_by_index(0);
            const xtcore::sampler::SolidColor *sc =
                dynamic_cast<const xtcore::sampler::SolidColor *>(s);
            if (sc) {
                nmath::Vector3f tc(0,0,0);
                nimg::ColorRGBf c = const_cast<xtcore::asset::IMaterial*>(mat)
                    ->get_sample(MAT_SAMPLER_DIFFUSE, tc);
                m.albedo = v3(c.r(), c.g(), c.b());
            } else {
                m.albedo_tex = (gpu_int)add_sampler(s);
                m.albedo     = v3(1.f, 1.f, 1.f);
            }
        } else {
            m.albedo = v3(0.8f, 0.8f, 0.8f);
        }
        if (mat->has_sampler(MAT_SAMPLER_NORMAL))
            m.normal_tex = (gpu_int)add_sampler(get_named_sampler(mat, MAT_SAMPLER_NORMAL));
        /* Lambert with an emissive sampler (e.g. area lights with diffuse≈0): promote to
         * GPU_MAT_EMISSIVE so path rays terminate with the emissive colour and the light
         * is added to the NEE pool below — matches CPU is_emissive() behaviour. */
        if (mat->has_sampler(MAT_SAMPLER_EMISSIVE)) {
            nmath::Vector3f tc(0,0,0);
            float intensity = const_cast<xtcore::asset::IMaterial*>(mat)->get_scalar("intensity");
            if (intensity <= 0.f) intensity = 1.f;
            nimg::ColorRGBf le = const_cast<xtcore::asset::IMaterial*>(mat)
                ->get_sample(MAT_SAMPLER_EMISSIVE, tc);
            float le_luma = (float)nimg::eval::luminance(le);
            if (le_luma > 1e-4f) {
                m.type   = GPU_MAT_EMISSIVE;
                m.albedo = v3(le.r() * intensity, le.g() * intensity, le.b() * intensity);
            }
        }
    } else if (dynamic_cast<const xtcore::asset::material::Emissive *>(mat)) {
        m.type = GPU_MAT_EMISSIVE;
        nmath::Vector3f tc(0,0,0);
        float intensity = const_cast<xtcore::asset::IMaterial*>(mat)
            ->get_scalar("intensity");
        if (intensity <= 0.f) intensity = 1.f;
        nimg::ColorRGBf c = const_cast<xtcore::asset::IMaterial*>(mat)
            ->get_sample(MAT_SAMPLER_EMISSIVE, tc);
        m.albedo = v3(c.r() * intensity, c.g() * intensity, c.b() * intensity);
    } else if (dynamic_cast<const xtcore::asset::material::Dielectric *>(mat)) {
        m.type = GPU_MAT_DIELECTRIC;
        auto *mmat = const_cast<xtcore::asset::IMaterial*>(mat);
        m.ior  = mmat->get_scalar("ior");
        if (m.ior <= 0.f) m.ior = 1.5f;
        /* transparency stored in roughness (unused for delta dielectric) */
        m.roughness = mmat->get_scalar("transparency");
        if (m.roughness <= 0.f) m.roughness = 1.f;
        if (m.roughness >  1.f) m.roughness = 1.f;
        m.albedo = v3(1.f, 1.f, 1.f);
    } else if (dynamic_cast<const xtcore::asset::material::Principled *>(mat)) {
        m.type = GPU_MAT_PRINCIPLED;
        nmath::Vector3f tc(0,0,0);
        auto *mmat = const_cast<xtcore::asset::IMaterial*>(mat);
        /* Base colour: upload as texture if sampler is Texture2D, else constant. */
        {
            const char *bc_slot = mmat->has_sampler(MAT_SAMPLER_BASE_COLOR)
                                    ? MAT_SAMPLER_BASE_COLOR : MAT_SAMPLER_DIFFUSE;
            if (mmat->has_sampler(bc_slot)) {
                const xtcore::sampler::ISampler *s = get_named_sampler(mat, bc_slot);
                const xtcore::sampler::SolidColor *sc =
                    dynamic_cast<const xtcore::sampler::SolidColor *>(s);
                if (sc) {
                    nimg::ColorRGBf c = mmat->get_sample(bc_slot, tc);
                    m.albedo = v3(c.r(), c.g(), c.b());
                } else {
                    m.albedo_tex = (gpu_int)add_sampler(s);
                    m.albedo     = v3(1.f, 1.f, 1.f);
                }
            } else {
                m.albedo = v3(1.f, 1.f, 1.f);
            }
        }
        /* Scalar parameters — mirror CPU helper clamp ranges from principled.cc. */
        float rough = (float)mmat->get_scalar(MAT_SCALART_ROUGHNESS);
        m.roughness = (rough >= 0.02f && rough <= 1.f) ? rough : 0.5f;
        float ior_v = (float)mmat->get_scalar(MAT_SCALART_IOR);
        m.ior = (ior_v > 1.f) ? ior_v : 1.5f;
        float met = (float)mmat->get_scalar(MAT_SCALART_METALLIC);
        m.metallic = fmaxf(0.f, fminf(1.f, met));
        float aniso = (float)mmat->get_scalar(MAT_SCALART_ANISOTROPY);
        m.anisotropy = fmaxf(0.f, fminf(1.f, aniso));
        m.anisotropy_rotation = (float)mmat->get_scalar(MAT_SCALART_ANISOTROPY_ROTATION);
        float cc = (float)mmat->get_scalar(MAT_SCALART_CLEARCOAT);
        m.clearcoat = fmaxf(0.f, fminf(1.f, cc));
        float ccr = (float)mmat->get_scalar(MAT_SCALART_CLEARCOAT_ROUGHNESS);
        m.clearcoat_roughness = (ccr >= 0.02f && ccr <= 0.6f) ? ccr : 0.08f;
        /* Sampler-driven parameters. */
        if (mmat->has_sampler(MAT_SAMPLER_NORMAL))
            m.normal_tex = (gpu_int)add_sampler(get_named_sampler(mat, MAT_SAMPLER_NORMAL));
        if (mmat->has_sampler(MAT_SAMPLER_ROUGHNESS))
            m.roughness_tex = (gpu_int)add_sampler(get_named_sampler(mat, MAT_SAMPLER_ROUGHNESS));
        if (mmat->has_sampler(MAT_SAMPLER_METALLIC))
            m.metallic_tex = (gpu_int)add_sampler(get_named_sampler(mat, MAT_SAMPLER_METALLIC));
    } else if (dynamic_cast<const xtcore::asset::material::RoughDielectric *>(mat)) {
        m.type = GPU_MAT_ROUGH_DIELECTRIC;
        auto *mmat = const_cast<xtcore::asset::IMaterial*>(mat);
        nmath::Vector3f tc(0,0,0);
        /* IOR and roughness. */
        float ior_v = (float)mmat->get_scalar(MAT_SCALART_IOR);
        m.ior = (ior_v > 1.f) ? ior_v : 1.5f;
        float rough = (float)mmat->get_scalar(MAT_SCALART_ROUGHNESS);
        m.roughness = (rough >= 0.02f && rough <= 1.f) ? rough : 0.3f;
        /* Transparency stored in metallic field (unused for this material). */
        float transp = (float)mmat->get_scalar(MAT_SCALART_TRANSPARENCY);
        m.metallic = (transp > 0.f && transp <= 1.f) ? transp : 1.f;
        /* Transmission tint: prefer MAT_SAMPLER_TRANSMISSION, fall back to DIFFUSE. */
        {
            const char *tx_slot = mmat->has_sampler(MAT_SAMPLER_TRANSMISSION)
                                    ? MAT_SAMPLER_TRANSMISSION : MAT_SAMPLER_DIFFUSE;
            if (mmat->has_sampler(tx_slot)) {
                const xtcore::sampler::ISampler *s = get_named_sampler(mat, tx_slot);
                const xtcore::sampler::SolidColor *sc =
                    dynamic_cast<const xtcore::sampler::SolidColor *>(s);
                if (sc) {
                    nimg::ColorRGBf c = mmat->get_sample(tx_slot, tc);
                    m.albedo = v3(c.r(), c.g(), c.b());
                } else {
                    m.albedo_tex = (gpu_int)add_sampler(s);
                    m.albedo     = v3(1.f, 1.f, 1.f);
                }
            } else {
                m.albedo = v3(1.f, 1.f, 1.f);
            }
        }
        if (mmat->has_sampler(MAT_SAMPLER_NORMAL))
            m.normal_tex = (gpu_int)add_sampler(get_named_sampler(mat, MAT_SAMPLER_NORMAL));
        if (mmat->has_sampler(MAT_SAMPLER_ROUGHNESS))
            m.roughness_tex = (gpu_int)add_sampler(get_named_sampler(mat, MAT_SAMPLER_ROUGHNESS));
    } else if (dynamic_cast<const xtcore::asset::material::Subsurface *>(mat)) {
        m.type = GPU_MAT_SUBSURFACE;
        auto *mmat = const_cast<xtcore::asset::IMaterial*>(mat);
        nmath::Vector3f tc(0,0,0);

        /* Base (diffuse) colour — also the default sss_color. */
        {
            const char *bc_slot = mmat->has_sampler(MAT_SAMPLER_BASE_COLOR)
                                    ? MAT_SAMPLER_BASE_COLOR : MAT_SAMPLER_DIFFUSE;
            if (mmat->has_sampler(bc_slot)) {
                const xtcore::sampler::ISampler *s = get_named_sampler(mat, bc_slot);
                const xtcore::sampler::SolidColor *sc =
                    dynamic_cast<const xtcore::sampler::SolidColor *>(s);
                if (sc) {
                    nimg::ColorRGBf c = mmat->get_sample(bc_slot, tc);
                    m.albedo = v3(c.r(), c.g(), c.b());
                } else {
                    m.albedo_tex = (gpu_int)add_sampler(s);
                    m.albedo     = v3(1.f, 1.f, 1.f);
                }
            } else {
                m.albedo = v3(1.f, 1.f, 1.f);
            }
        }

        /* SSS weight (roughness slot) and thickness (ior slot). */
        float sss_w = (float)mmat->get_scalar(MAT_SCALART_SUBSURFACE);
        m.roughness = fmaxf(0.f, fminf(1.f, sss_w));
        m.ior = fmaxf(0.f, (float)mmat->get_scalar(MAT_SCALART_THICKNESS));

        /* Subsurface colour (metallic/anisotropy/anisotropy_rotation slots). */
        nimg::ColorRGBf sss_col(m.albedo.x, m.albedo.y, m.albedo.z);
        if (mmat->has_sampler(MAT_SAMPLER_SUBSURFACE_COLOR))
            sss_col = mmat->get_sample(MAT_SAMPLER_SUBSURFACE_COLOR, tc);
        m.metallic           = sss_col.r();
        m.anisotropy         = sss_col.g();
        m.anisotropy_rotation = sss_col.b();

        /* Subsurface radius per-channel (clearcoat/clearcoat_roughness/roughness_tex slots).
         * roughness_tex is gpu_int — store float bits via memcpy (kernel reads via as_float). */
        nimg::ColorRGBf sss_rad(0.f, 0.f, 0.f);
        if (mmat->has_sampler(MAT_SAMPLER_SUBSURFACE_RADIUS))
            sss_rad = mmat->get_sample(MAT_SAMPLER_SUBSURFACE_RADIUS, tc);
        m.clearcoat           = fmaxf(1e-4f, sss_rad.r());
        m.clearcoat_roughness = fmaxf(1e-4f, sss_rad.g());
        float sss_rad_b = fmaxf(1e-4f, sss_rad.b());
        memcpy(&m.roughness_tex, &sss_rad_b, sizeof(float));

        /* Normal map. */
        if (mmat->has_sampler(MAT_SAMPLER_NORMAL))
            m.normal_tex = (gpu_int)add_sampler(get_named_sampler(mat, MAT_SAMPLER_NORMAL));
    } else if (dynamic_cast<const xtcore::asset::material::Phong *>(mat)
            || dynamic_cast<const xtcore::asset::material::BlinnPhong *>(mat)) {
        m.type = dynamic_cast<const xtcore::asset::material::Phong *>(mat)
               ? GPU_MAT_PHONG : GPU_MAT_BLINNPHONG;
        auto *mmat = const_cast<xtcore::asset::IMaterial*>(mat);
        nmath::Vector3f tc(0,0,0);
        /* Diffuse color → albedo. */
        if (mmat->has_sampler(MAT_SAMPLER_DIFFUSE)) {
            const xtcore::sampler::ISampler *s = get_named_sampler(mat, MAT_SAMPLER_DIFFUSE);
            const xtcore::sampler::SolidColor *sc = dynamic_cast<const xtcore::sampler::SolidColor *>(s);
            if (sc) {
                nimg::ColorRGBf c = mmat->get_sample(MAT_SAMPLER_DIFFUSE, tc);
                m.albedo = v3(c.r(), c.g(), c.b());
            } else {
                m.albedo_tex = (gpu_int)add_sampler(s);
                m.albedo     = v3(1.f, 1.f, 1.f);
            }
        } else {
            m.albedo = v3(0.5f, 0.5f, 0.5f);
        }
        /* Specular color packed into metallic/anisotropy/anisotropy_rotation. */
        if (mmat->has_sampler(MAT_SAMPLER_SPECULAR)) {
            const xtcore::sampler::ISampler *s = get_named_sampler(mat, MAT_SAMPLER_SPECULAR);
            const xtcore::sampler::SolidColor *sc = dynamic_cast<const xtcore::sampler::SolidColor *>(s);
            if (sc) {
                nimg::ColorRGBf ks = mmat->get_sample(MAT_SAMPLER_SPECULAR, tc);
                m.metallic           = ks.r();
                m.anisotropy         = ks.g();
                m.anisotropy_rotation = ks.b();
            } else {
                m.metallic_tex       = (gpu_int)add_sampler(s);
                m.metallic           = 1.f;
                m.anisotropy         = 1.f;
                m.anisotropy_rotation = 1.f;
            }
        }
        /* Exponent → roughness field. */
        float exp_v = (float)mmat->get_scalar(MAT_SCALART_EXPONENT);
        m.roughness = fmaxf(1.f, exp_v);
        /* Reflectance (lobe weight) → clearcoat field. */
        float refl = (float)mmat->get_scalar(MAT_SCALART_REFLECTANCE);
        m.clearcoat = fmaxf(0.f, fminf(1.f, refl));
        /* Normal map. */
        if (mmat->has_sampler(MAT_SAMPLER_NORMAL))
            m.normal_tex = (gpu_int)add_sampler(get_named_sampler(mat, MAT_SAMPLER_NORMAL));
    } else if (dynamic_cast<const xtcore::asset::material::Sheen *>(mat)) {
        m.type = GPU_MAT_SHEEN;
        auto *mmat = const_cast<xtcore::asset::IMaterial*>(mat);
        nmath::Vector3f tc(0,0,0);
        /* Base color → albedo. */
        {
            const char *bc_slot = mmat->has_sampler(MAT_SAMPLER_BASE_COLOR)
                                    ? MAT_SAMPLER_BASE_COLOR : MAT_SAMPLER_DIFFUSE;
            if (mmat->has_sampler(bc_slot)) {
                const xtcore::sampler::ISampler *s = get_named_sampler(mat, bc_slot);
                const xtcore::sampler::SolidColor *sc = dynamic_cast<const xtcore::sampler::SolidColor *>(s);
                if (sc) {
                    nimg::ColorRGBf c = mmat->get_sample(bc_slot, tc);
                    m.albedo = v3(c.r(), c.g(), c.b());
                } else {
                    m.albedo_tex = (gpu_int)add_sampler(s);
                    m.albedo     = v3(1.f, 1.f, 1.f);
                }
            } else {
                m.albedo = v3(1.f, 1.f, 1.f);
            }
        }
        /* Sheen color (packed into metallic/anisotropy/anisotropy_rotation).
         * Falls back to base_color when MAT_SAMPLER_SHEEN_COLOR is absent,
         * matching CPU sheen_color() fallback behaviour. */
        {
            const char *sc_slot = mmat->has_sampler(MAT_SAMPLER_SHEEN_COLOR)
                                    ? MAT_SAMPLER_SHEEN_COLOR : nullptr;
            if (sc_slot) {
                const xtcore::sampler::ISampler *s = get_named_sampler(mat, sc_slot);
                const xtcore::sampler::SolidColor *sc = dynamic_cast<const xtcore::sampler::SolidColor *>(s);
                if (sc) {
                    nimg::ColorRGBf c = mmat->get_sample(sc_slot, tc);
                    m.metallic            = c.r();
                    m.anisotropy          = c.g();
                    m.anisotropy_rotation = c.b();
                } else {
                    m.metallic_tex        = (gpu_int)add_sampler(s);
                    m.metallic            = 1.f;
                    m.anisotropy          = 1.f;
                    m.anisotropy_rotation = 1.f;
                }
            } else {
                /* No sheen_color sampler — use base_color as sheen tint. */
                m.metallic            = m.albedo.x;
                m.anisotropy          = m.albedo.y;
                m.anisotropy_rotation = m.albedo.z;
            }
        }
        /* Sheen strength → roughness field. */
        float sheen_s = (float)mmat->get_scalar(MAT_SCALART_SHEEN);
        m.roughness = fmaxf(0.f, fminf(1.f, sheen_s));
        /* Normal map. */
        if (mmat->has_sampler(MAT_SAMPLER_NORMAL))
            m.normal_tex = (gpu_int)add_sampler(get_named_sampler(mat, MAT_SAMPLER_NORMAL));
    } else if (dynamic_cast<const xtcore::asset::material::ThinDielectric *>(mat)) {
        m.type = GPU_MAT_THIN_DIELECTRIC;
        auto *mmat = const_cast<xtcore::asset::IMaterial*>(mat);
        nmath::Vector3f tc(0,0,0);
        /* Transmission tint → albedo. */
        {
            const char *slot = mmat->has_sampler(MAT_SAMPLER_TRANSMISSION) ? MAT_SAMPLER_TRANSMISSION
                             : mmat->has_sampler(MAT_SAMPLER_DIFFUSE)       ? MAT_SAMPLER_DIFFUSE
                             : nullptr;
            if (slot) {
                const xtcore::sampler::ISampler *s = get_named_sampler(mat, slot);
                const xtcore::sampler::SolidColor *sc = dynamic_cast<const xtcore::sampler::SolidColor *>(s);
                if (sc) {
                    nimg::ColorRGBf c = mmat->get_sample(slot, tc);
                    m.albedo = v3(c.r(), c.g(), c.b());
                } else {
                    m.albedo_tex = (gpu_int)add_sampler(s);
                    m.albedo     = v3(1.f, 1.f, 1.f);
                }
            } else {
                m.albedo = v3(1.f, 1.f, 1.f);
            }
        }
        /* IOR, roughness, transparency. */
        {
            float ior = (float)mmat->get_scalar(MAT_SCALART_IOR);
            m.ior       = (ior > 1.f) ? ior : 1.5f;
            float rgh   = (float)mmat->get_scalar(MAT_SCALART_ROUGHNESS);
            m.roughness = fmaxf(0.f, fminf(1.f, rgh));
            float transp = (float)mmat->get_scalar(MAT_SCALART_TRANSPARENCY);
            m.metallic  = fmaxf(0.f, fminf(1.f, transp > 0.f ? transp : 1.f));
        }
        /* Normal map. */
        if (mmat->has_sampler(MAT_SAMPLER_NORMAL))
            m.normal_tex = (gpu_int)add_sampler(get_named_sampler(mat, MAT_SAMPLER_NORMAL));
    } else if (dynamic_cast<const xtcore::asset::material::ThinTranslucent *>(mat)) {
        m.type = GPU_MAT_THIN_TRANSLUCENT;
        auto *mmat = const_cast<xtcore::asset::IMaterial*>(mat);
        nmath::Vector3f tc(0,0,0);
        /* Base color → albedo. */
        {
            const char *bc_slot = mmat->has_sampler(MAT_SAMPLER_BASE_COLOR) ? MAT_SAMPLER_BASE_COLOR
                                : mmat->has_sampler(MAT_SAMPLER_DIFFUSE)     ? MAT_SAMPLER_DIFFUSE
                                : nullptr;
            if (bc_slot) {
                const xtcore::sampler::ISampler *s = get_named_sampler(mat, bc_slot);
                const xtcore::sampler::SolidColor *sc = dynamic_cast<const xtcore::sampler::SolidColor *>(s);
                if (sc) {
                    nimg::ColorRGBf c = mmat->get_sample(bc_slot, tc);
                    m.albedo = v3(c.r(), c.g(), c.b());
                } else {
                    m.albedo_tex = (gpu_int)add_sampler(s);
                    m.albedo     = v3(1.f, 1.f, 1.f);
                }
            } else {
                m.albedo = v3(1.f, 1.f, 1.f);
            }
        }
        /* Translucency color → metallic/anisotropy/anisotropy_rotation (falls back to base). */
        {
            const char *tc_slot = mmat->has_sampler(MAT_SAMPLER_TRANSLUCENCY_COLOR) ? MAT_SAMPLER_TRANSLUCENCY_COLOR
                                : mmat->has_sampler(MAT_SAMPLER_TRANSMISSION)        ? MAT_SAMPLER_TRANSMISSION
                                : nullptr;
            if (tc_slot) {
                const xtcore::sampler::ISampler *s = get_named_sampler(mat, tc_slot);
                const xtcore::sampler::SolidColor *sc = dynamic_cast<const xtcore::sampler::SolidColor *>(s);
                if (sc) {
                    nimg::ColorRGBf c = mmat->get_sample(tc_slot, tc);
                    m.metallic            = c.r();
                    m.anisotropy          = c.g();
                    m.anisotropy_rotation = c.b();
                } else {
                    m.metallic_tex        = (gpu_int)add_sampler(s);
                    m.metallic            = 1.f;
                    m.anisotropy          = 1.f;
                    m.anisotropy_rotation = 1.f;
                }
            } else {
                /* No translucency_color — use base_col (signaled as all-zero to shader). */
                m.metallic            = 0.f;
                m.anisotropy          = 0.f;
                m.anisotropy_rotation = 0.f;
            }
        }
        /* Translucency scalar → roughness; thickness → ior. */
        {
            float transl = (float)mmat->get_scalar(MAT_SCALART_TRANSLUCENCY);
            m.roughness  = fmaxf(0.f, fminf(1.f, transl));
            float thick  = (float)mmat->get_scalar(MAT_SCALART_THICKNESS);
            m.ior        = fmaxf(0.f, thick);
        }
        /* Normal map. */
        if (mmat->has_sampler(MAT_SAMPLER_NORMAL))
            m.normal_tex = (gpu_int)add_sampler(get_named_sampler(mat, MAT_SAMPLER_NORMAL));
    } else if (dynamic_cast<const xtcore::asset::material::Boundary *>(mat)) {
        m.type = GPU_MAT_BOUNDARY;
    } else {
        xtcore::Log::handle().post_warning("[gpu_opencl] unsupported material type '%s' -- falling back to mid-grey Lambert",
                typeid(*mat).name());
        m.type   = GPU_MAT_LAMBERT;
        m.albedo = v3(0.5f, 0.5f, 0.5f);
    }

    uint32_t idx = (uint32_t)materials.size();
    materials.push_back(m);
    m_mat_index[mat] = idx;
    return idx;
}

/* ---- geometry ---- */

static void serialize_mesh(const xtcore::surface::Mesh *mesh,
                           std::vector<gpu_bvh_node_t>  &out_bvh,
                           std::vector<gpu_triangle_t>  &out_tri,
                           uint32_t &tri_base, uint32_t &bvh_base)
{
    tri_base = (uint32_t)out_tri.size();
    bvh_base = (uint32_t)out_bvh.size();

    const auto &src_tris    = mesh->triangles();
    const auto &src_indices = mesh->bvh_indices();
    const auto &src_bvh     = mesh->bvh_nodes();

    // Permute triangles into BVH order so bvh_node.first indexes directly
    if (!src_indices.empty()) {
        for (uint32_t idx : src_indices) {
            const xtcore::surface::Triangle &t = src_tris[idx];
            gpu_triangle_t g;
            g.v0 = from_vec(t.v[0]); g.v1 = from_vec(t.v[1]); g.v2 = from_vec(t.v[2]);
            g.n0 = from_vec(t.n[0]); g.n1 = from_vec(t.n[1]); g.n2 = from_vec(t.n[2]);
            g.uv0[0]=(float)t.tc[0].x; g.uv0[1]=(float)t.tc[0].y;
            g.uv1[0]=(float)t.tc[1].x; g.uv1[1]=(float)t.tc[1].y;
            g.uv2[0]=(float)t.tc[2].x; g.uv2[1]=(float)t.tc[2].y;
            g._pad[0]=0.f; g._pad[1]=0.f;
            out_tri.push_back(g);
        }
    } else {
        // No BVH — just dump triangles in order
        for (const auto &t : src_tris) {
            gpu_triangle_t g;
            g.v0 = from_vec(t.v[0]); g.v1 = from_vec(t.v[1]); g.v2 = from_vec(t.v[2]);
            g.n0 = from_vec(t.n[0]); g.n1 = from_vec(t.n[1]); g.n2 = from_vec(t.n[2]);
            g.uv0[0]=(float)t.tc[0].x; g.uv0[1]=(float)t.tc[0].y;
            g.uv1[0]=(float)t.tc[1].x; g.uv1[1]=(float)t.tc[1].y;
            g.uv2[0]=(float)t.tc[2].x; g.uv2[1]=(float)t.tc[2].y;
            g._pad[0]=0.f; g._pad[1]=0.f;
            out_tri.push_back(g);
        }
    }

    for (const auto &n : src_bvh) {
        gpu_bvh_node_t g;
        g.aabb  = from_aabb(n.aabb);
        g.left  = n.left  + bvh_base;
        g.right = n.right + bvh_base;
        g.first = n.first + tri_base;
        g.count = n.count;
        out_bvh.push_back(g);
    }
}

void SceneSerializer::serialize_object(uint64_t obj_id, const xtcore::asset::Object *obj)
{
    if (!obj || !obj->ptr_surface) return;

    // Multi-material objects store materials in ptr_material_array; fall back to ptr_material.
    // Use the middle entry so fractal orbit-trap gradients show a representative colour.
    const xtcore::asset::IMaterial *mat_ptr = obj->ptr_material;
    if (!mat_ptr && !obj->ptr_material_array.empty()) {
        size_t mid = obj->ptr_material_array.size() / 2;
        mat_ptr = obj->ptr_material_array[mid];
    }
    uint32_t mat_idx = get_or_add_material(mat_ptr);

    gpu_object_t go;
    memset(&go, 0, sizeof(go));
    go.mat_index = mat_idx;

    const xtcore::asset::ISurface *surf = obj->ptr_surface;

    if (const xtcore::surface::Mesh *mesh = dynamic_cast<const xtcore::surface::Mesh *>(surf)) {
        go.geom_type = GPU_GEOM_MESH;
        go.geom_index = 0;
        serialize_mesh(mesh, bvh_nodes, triangles, go.tri_base, go.bvh_base);
    } else if (const xtcore::surface::Sphere *sp = dynamic_cast<const xtcore::surface::Sphere *>(surf)) {
        go.geom_type  = GPU_GEOM_SPHERE;
        go.geom_index = (uint32_t)spheres.size();
        gpu_sphere_t s;
        s.origin = from_vec(sp->origin);
        s.radius = (float)sp->radius;
        spheres.push_back(s);
    } else if (const xtcore::surface::Plane *pl = dynamic_cast<const xtcore::surface::Plane *>(surf)) {
        go.geom_type  = GPU_GEOM_PLANE;
        go.geom_index = (uint32_t)planes.size();
        gpu_plane_t p;
        p.normal = from_vec(pl->normal);
        p.offset = (float)pl->offset;
        planes.push_back(p);
    } else if (const xtcore::surface::Triangle *tri = dynamic_cast<const xtcore::surface::Triangle *>(surf)) {
        /* Standalone triangle — pack as a single-leaf mesh so the existing mesh path handles it. */
        go.geom_type  = GPU_GEOM_MESH;
        go.geom_index = 0;
        go.tri_base   = (uint32_t)triangles.size();
        go.bvh_base   = (uint32_t)bvh_nodes.size();

        /* Face normal fallback for vertices whose normals were not set. */
        nmath::Vector3f fn = tri->face_normal;
        if (fn.length() < 1e-7f) {
            fn = nmath::cross(tri->v[1] - tri->v[0], tri->v[2] - tri->v[0]);
            if (fn.length() > 1e-7f) fn = fn.normalized();
        }
        auto vn = [&](int i) -> gpu_float3 {
            nmath::Vector3f n = tri->n[i];
            return from_vec(n.length() > 1e-7f ? n.normalized() : fn);
        };

        gpu_triangle_t g;
        g.v0 = from_vec(tri->v[0]); g.v1 = from_vec(tri->v[1]); g.v2 = from_vec(tri->v[2]);
        g.n0 = vn(0);               g.n1 = vn(1);               g.n2 = vn(2);
        g.uv0[0]=(float)tri->tc[0].x; g.uv0[1]=(float)tri->tc[0].y;
        g.uv1[0]=(float)tri->tc[1].x; g.uv1[1]=(float)tri->tc[1].y;
        g.uv2[0]=(float)tri->tc[2].x; g.uv2[1]=(float)tri->tc[2].y;
        g._pad[0]=0.f; g._pad[1]=0.f;
        triangles.push_back(g);

        /* Single leaf BVH node — count>0 means the mesh traversal tests triangles directly. */
        gpu_bvh_node_t bvh;
        bvh.aabb  = from_aabb(tri->aabb);
        bvh.left  = go.bvh_base;
        bvh.right = go.bvh_base;
        bvh.first = go.tri_base;
        bvh.count = 1;
        bvh_nodes.push_back(bvh);

    } else {
        gpu_fractal_t frac;
        memset(&frac, 0, sizeof(frac));
        bool is_fractal = false;

        if (const xtcore::surface::Mandelbulb *f = dynamic_cast<const xtcore::surface::Mandelbulb *>(surf)) {
            frac.type = GPU_FRACTAL_MANDELBULB;
            frac.origin = from_vec(f->origin); frac.radius=(float)f->radius;
            frac.power=(float)f->power; frac.bailout=(float)f->bailout;
            frac.iterations=f->iterations; frac.orbit_trap_channel=f->orbit_trap_channel;
            is_fractal = true;
        } else if (const xtcore::surface::JuliaFractal *f = dynamic_cast<const xtcore::surface::JuliaFractal *>(surf)) {
            frac.type = GPU_FRACTAL_JULIA;
            frac.origin=from_vec(f->origin); frac.param_vec=from_vec(f->julia_c);
            frac.radius=(float)f->radius; frac.power=(float)f->power;
            frac.bailout=(float)f->bailout; frac.iterations=f->iterations;
            frac.orbit_trap_channel=f->orbit_trap_channel;
            is_fractal = true;
        } else if (const xtcore::surface::MandelBox *f = dynamic_cast<const xtcore::surface::MandelBox *>(surf)) {
            frac.type = GPU_FRACTAL_MANDELBOX;
            frac.origin=from_vec(f->origin); frac.radius=(float)f->radius;
            frac.fold_size=(float)f->fold_size; frac.min_r=(float)f->min_r;
            frac.scale_f=(float)f->scale; frac.bailout=(float)f->bailout;
            frac.iterations=f->iterations; frac.orbit_trap_channel=f->orbit_trap_channel;
            is_fractal = true;
        } else if (const xtcore::surface::QuaternionJulia *f = dynamic_cast<const xtcore::surface::QuaternionJulia *>(surf)) {
            frac.type = GPU_FRACTAL_QUAT_JULIA;
            frac.origin=from_vec(f->origin); frac.param_vec=from_vec(f->quat_c);
            frac.param_w=(float)f->quat_cw; frac.param_w0=(float)f->quat_w0;
            frac.radius=(float)f->radius; frac.bailout=(float)f->bailout;
            frac.iterations=f->iterations; frac.orbit_trap_channel=f->orbit_trap_channel;
            is_fractal = true;
        } else if (const xtcore::surface::MengerSponge *f = dynamic_cast<const xtcore::surface::MengerSponge *>(surf)) {
            frac.type = GPU_FRACTAL_MENGER;
            frac.origin=from_vec(f->origin); frac.param_vec=from_vec(f->orientation);
            frac.radius=(float)f->radius; frac.iterations=f->iterations;
            is_fractal = true;
        } else if (const xtcore::surface::SierpinskiTetrahedron *f = dynamic_cast<const xtcore::surface::SierpinskiTetrahedron *>(surf)) {
            frac.type = GPU_FRACTAL_SIERPINSKI_TET;
            frac.origin=from_vec(f->origin); frac.radius=(float)f->radius;
            frac.iterations=f->iterations;
            is_fractal = true;
        } else if (const xtcore::surface::BurningShip3D *f = dynamic_cast<const xtcore::surface::BurningShip3D *>(surf)) {
            frac.type = GPU_FRACTAL_BURNING_SHIP;
            frac.origin=from_vec(f->origin); frac.radius=(float)f->radius;
            frac.power=(float)f->power; frac.bailout=(float)f->bailout;
            frac.iterations=f->iterations; frac.orbit_trap_channel=f->orbit_trap_channel;
            is_fractal = true;
        } else if (const xtcore::surface::CantorDust3D *f = dynamic_cast<const xtcore::surface::CantorDust3D *>(surf)) {
            frac.type = GPU_FRACTAL_CANTOR_DUST;
            frac.origin=from_vec(f->origin); frac.param_vec=from_vec(f->orientation);
            frac.radius=(float)f->radius; frac.iterations=f->iterations;
            is_fractal = true;
        } else if (const xtcore::surface::IcosahedralIFS *f = dynamic_cast<const xtcore::surface::IcosahedralIFS *>(surf)) {
            frac.type = GPU_FRACTAL_ICOSAHEDRAL_IFS;
            frac.origin=from_vec(f->origin); frac.radius=(float)f->radius;
            frac.scale_f=(float)f->scale; frac.iterations=f->iterations;
            is_fractal = true;
        }

        if (is_fractal) {
            go.geom_type  = GPU_GEOM_FRACTAL;
            go.geom_index = (uint32_t)fractals.size();
            fractals.push_back(frac);
        } else {
            return; // unsupported geometry — skip object
        }
    }

    uint32_t obj_index = (uint32_t)objects.size();
    m_obj_index[obj_id] = obj_index;
    objects.push_back(go);

    // Collect emissives for GPU NEE — spheres, standalone triangles, and meshes.
    // Use is_emissive() (same predicate as CPU integrator) so Lambert materials that
    // carry an emissive sampler (area lights with diffuse≈0) are also registered.
    if (mat_ptr && mat_ptr->is_emissive()) {
        nmath::Vector3f tc(0,0,0);
        nimg::ColorRGBf le = const_cast<xtcore::asset::IMaterial*>(mat_ptr)
                                ->get_sample(MAT_SAMPLER_EMISSIVE, tc);
        float luma = (float)nimg::eval::luminance(le);
        if (luma < 1e-4f) luma = 1e-4f;

        gpu_emissive_t e;
        memset(&e, 0, sizeof(e));
        e.object_index = obj_index;
        e.mat_index    = mat_idx;

        if (go.geom_type == GPU_GEOM_SPHERE) {
            const xtcore::surface::Sphere *sp =
                dynamic_cast<const xtcore::surface::Sphere *>(surf);
            if (sp) {
                float r = (float)sp->radius;
                e.type       = GPU_EMISSIVE_SPHERE;
                e.geom_index = go.geom_index;
                e.area       = 4.f * 3.14159265358f * r * r;
                e.select_weight = e.area * luma;
                emissives.push_back(e);
                materials[mat_idx].nee_emissive = 1;
            }
        } else if (go.geom_type == GPU_GEOM_MESH) {
            // tri_count = triangles added by serialize_mesh (or standalone Triangle pack)
            uint32_t tri_count = (uint32_t)triangles.size() - go.tri_base;
            if (tri_count > 0) {
                // Compute total area by summing triangle areas in the global buffer.
                float total_area = 0.f;
                for (uint32_t ti = go.tri_base; ti < go.tri_base + tri_count; ++ti) {
                    nmath::Vector3f v0(triangles[ti].v0.x, triangles[ti].v0.y, triangles[ti].v0.z);
                    nmath::Vector3f v1(triangles[ti].v1.x, triangles[ti].v1.y, triangles[ti].v1.z);
                    nmath::Vector3f v2(triangles[ti].v2.x, triangles[ti].v2.y, triangles[ti].v2.z);
                    total_area += 0.5f * (float)nmath::cross(v1 - v0, v2 - v0).length();
                }
                if (total_area > 1e-8f) {
                    e.type          = GPU_EMISSIVE_TRIANGLE;
                    e.tri_base      = go.tri_base;
                    e.tri_count     = tri_count;
                    e.area          = total_area;
                    e.select_weight = total_area * luma;
                    emissives.push_back(e);
                    materials[mat_idx].nee_emissive = 1;
                }
            }
        }
    }
}

/* ---- camera ---- */

void SceneSerializer::serialize_camera(xtcore::render::context_t &ctx)
{
    memset(&camera, 0, sizeof(camera));

    xtcore::asset::ICamera *cam = ctx.active_camera();
    if (!cam) return;

    const xtcore::camera::Perspective *pcam =
        dynamic_cast<const xtcore::camera::Perspective *>(cam);
    if (!pcam) return;

    camera.position = from_vec(pcam->position);

    // Recompute look-at basis — matches CPU Perspective::calculate_transform() exactly:
    //   rz = normalize(target - position)
    //   rx = normalize(cross(rz, up))
    //   ry = normalize(cross(rz, rx))
    nmath::Vector3f fwd   = (pcam->target - pcam->position).normalized();
    nmath::Vector3f right = nmath::cross(fwd, pcam->up).normalized();
    nmath::Vector3f up    = nmath::cross(fwd, right).normalized(); // cross(rz,rx), NOT cross(rx,rz)

    camera.forward = from_vec(fwd);
    camera.right   = from_vec(right);
    camera.up      = from_vec(up);

    // focal distance D = 1/tan(fov/2) — matches CPU ray.direction.z = 1/tan(fov*RADIAN/2)
    // fov is in degrees; RADIAN = pi/180
    float D = 1.0f / std::tan((float)pcam->fov * 0.5f * (float)(M_PI / 180.0));
    camera.half_h_over_d = D;     // repurposed: stores D = 1/tan(fov/2)
    camera.half_w_over_d = 0.0f;  // unused — aspect handled in kernel
    camera.aperture      = pcam->aperture;
    camera.focal_length  = pcam->flength;
}

/* ---- top-level ---- */

void SceneSerializer::serialize(xtcore::render::context_t &ctx)
{
    if (!ctx.scene) return;

    xtcore::Scene &scene = *ctx.scene;
    scene.rebuild_spatial_index();

    // Serialize all objects.
    // ptr_surface and ptr_material are lazily initialized by CPU code paths;
    // resolve them now so serialize_object doesn't skip null-pointer objects.
    for (auto &kv : scene.m_objects) {
        scene.get_surface(kv.first);
        scene.get_material(kv.first);
        serialize_object(kv.first, kv.second);
    }

    // TLAS nodes
    for (const auto &n : scene.tlas_nodes()) {
        gpu_tlas_node_t g;
        g.aabb  = from_aabb(n.aabb);
        g.left  = n.left;
        g.right = n.right;
        g.first = (gpu_uint)n.first;
        g.count = (gpu_uint)n.count;
        tlas_nodes.push_back(g);
    }

    // TLAS items
    for (const auto &item : scene.tlas_items()) {
        gpu_tlas_item_t g;
        g.aabb = from_aabb(item.aabb);
        g.object_id = item.object_id;
        auto it = m_obj_index.find(item.object_id);
        g.object_index = (it != m_obj_index.end()) ? it->second : (uint32_t)-1;
        g._pad = 0;
        tlas_items.push_back(g);
    }

    // Infinite objects
    for (uint64_t obj_id : scene.infinite_objects()) {
        gpu_infinite_obj_t g;
        auto it = m_obj_index.find(obj_id);
        g.object_index = (it != m_obj_index.end()) ? it->second : (uint32_t)-1;
        g._pad[0]=g._pad[1]=g._pad[2]=0;
        infinite.push_back(g);
    }

    // Camera
    serialize_camera(ctx);

    // Record actual counts before padding (padding is only to avoid zero-size CL buffers)
    memset(&header, 0, sizeof(header));
    header.tlas_root       = 0; // root is always node 0
    header.tlas_node_count = (gpu_uint)tlas_nodes.size();
    header.tlas_item_count = (gpu_uint)tlas_items.size();
    header.object_count    = (gpu_uint)objects.size();
    header.infinite_count  = (gpu_uint)infinite.size();
    header.triangle_count  = (gpu_uint)triangles.size();
    header.bvh_node_count  = (gpu_uint)bvh_nodes.size();
    header.sphere_count    = (gpu_uint)spheres.size();
    header.plane_count     = (gpu_uint)planes.size();
    header.fractal_count   = (gpu_uint)fractals.size();
    header.material_count  = (gpu_uint)materials.size();
    header.emissive_count  = (gpu_uint)emissives.size();
    {
        float sw = 0.f;
        for (const auto &e : emissives) sw += e.select_weight;
        header.total_select_weight = sw;
    }
    header.env_tex         = -1;
    header.env_intensity   = 1.f;
    // Serialize gradient environment colors as raw a/b parameters so the kernel
    // can replicate the CPU formula: t = 0.5*dir.y + 1.0, color = (1-t)*a + t*b.
    // Sampling at extreme directions (0,±1,0) gives extrapolated/negative values —
    // we must use the raw a and b directly.
    {
        const xtcore::sampler::Gradient *grad =
            dynamic_cast<const xtcore::sampler::Gradient *>(scene.m_environment);
        if (grad) {
            // Direct access: store a in env_sky fields, b in env_gnd fields
            header.env_sky_r = (float)grad->a.r(); header.env_sky_g = (float)grad->a.g(); header.env_sky_b = (float)grad->a.b();
            header.env_gnd_r = (float)grad->b.r(); header.env_gnd_g = (float)grad->b.g(); header.env_gnd_b = (float)grad->b.b();
        } else {
            // Non-gradient environment: sample at dir.y=0 (→ b) and dir.y=-1 (→ (a+b)/2),
            // then reconstruct a = 2*(a+b)/2 - b.
            nimg::ColorRGBf c_horiz = scene.sample_environment(nmath::Vector3f(0, 0,-1)); // t=1 → b
            nimg::ColorRGBf c_down  = scene.sample_environment(nmath::Vector3f(0,-1, 0)); // t=0.5 → (a+b)/2
            float ar = 2.f*(float)c_down.r() - (float)c_horiz.r();
            float ag = 2.f*(float)c_down.g() - (float)c_horiz.g();
            float ab = 2.f*(float)c_down.b() - (float)c_horiz.b();
            header.env_sky_r = ar; header.env_sky_g = ag; header.env_sky_b = ab;
            header.env_gnd_r = (float)c_horiz.r(); header.env_gnd_g = (float)c_horiz.g(); header.env_gnd_b = (float)c_horiz.b();
        }
    }

    // Pad any empty vectors to avoid zero-size CL buffers (counts already recorded above)
    if (tlas_nodes.empty())  { gpu_tlas_node_t   g; memset(&g,0,sizeof(g)); tlas_nodes.push_back(g); }
    if (tlas_items.empty())  { gpu_tlas_item_t   g; memset(&g,0,sizeof(g)); tlas_items.push_back(g); }
    if (infinite.empty())    { gpu_infinite_obj_t g; memset(&g,0,sizeof(g)); infinite.push_back(g); }
    if (bvh_nodes.empty())   { gpu_bvh_node_t    g; memset(&g,0,sizeof(g)); bvh_nodes.push_back(g); }
    if (triangles.empty())   { gpu_triangle_t    g; memset(&g,0,sizeof(g)); triangles.push_back(g); }
    if (spheres.empty())     { gpu_sphere_t      g; memset(&g,0,sizeof(g)); spheres.push_back(g); }
    if (planes.empty())      { gpu_plane_t       g; memset(&g,0,sizeof(g)); planes.push_back(g); }
    if (fractals.empty())    { gpu_fractal_t     g; memset(&g,0,sizeof(g)); fractals.push_back(g); }
    if (emissives.empty())   { gpu_emissive_t    g; memset(&g,0,sizeof(g)); emissives.push_back(g); }
    if (objects.empty())     { gpu_object_t      g; memset(&g,0,sizeof(g)); objects.push_back(g); }
    if (materials.empty())   { gpu_material_t    g; memset(&g,0,sizeof(g)); materials.push_back(g); }
    if (tex_descs.empty())   { gpu_tex_desc_t    g; memset(&g,0,sizeof(g)); tex_descs.push_back(g); }
    if (pixels.empty())      { cl_float4 g = {}; pixels.push_back(g); }
    if (samplers.empty())    { gpu_sampler_t     g; memset(&g,0,sizeof(g)); samplers.push_back(g); }
}

        } /* namespace gpu_opencl */
    } /* namespace integrator */
} /* namespace xtcore */

#endif /* XTCORE_ENABLE_OPENCL */
