#include <algorithm>
#include <cmath>
#include <cstdlib>

#include <nmath/precision.h>
#include <xtcore/aa.h>
#include <xtcore/material.h>
#include <xtcore/math/sampling_util.h>
#include "util/raygraph.h"
#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace debug_views {

namespace {

inline nmath::scalar_t clamp01(nmath::scalar_t v)
{
    return std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, v));
}

inline nmath::Vector3f resolve_shading_normal(
      xtcore::render::context_t *ctx
    , const xtcore::hit_record_t &hit_record)
{
    nmath::Vector3f n = hit_record.normal.normalized();
    if (!ctx) return n;

    const xtcore::asset::IMaterial *mat = ctx->scene.get_material(hit_record.id_object);
    if (!mat || !mat->has_sampler(MAT_SAMPLER_NORMAL)) return n;

    const nimg::ColorRGBf tex = mat->get_sample(MAT_SAMPLER_NORMAL, hit_record.texcoord);
    nmath::Vector3f tangent_space_n(
        (nmath::scalar_t)(tex.r() * 2.0f - 1.0f),
        (nmath::scalar_t)(tex.g() * 2.0f - 1.0f),
        (nmath::scalar_t)(tex.b() * 2.0f - 1.0f)
    );
    if (tangent_space_n.length() <= (nmath::scalar_t)EPSILON) return n;
    tangent_space_n.normalize();

    const nmath::Vector3f t = xtcore::math::sampling::build_tangent(n);
    nmath::Vector3f b = nmath::cross(n, t);
    if (b.length() <= (nmath::scalar_t)EPSILON) return n;
    b.normalize();

    nmath::Vector3f mapped = (t * tangent_space_n.x) + (b * tangent_space_n.y) + (n * tangent_space_n.z);
    if (mapped.length() <= (nmath::scalar_t)EPSILON) return n;
    mapped.normalize();

    if (nmath::dot(mapped, n) <= (nmath::scalar_t)0.0) return n;
    return mapped;
}

} // namespace

Integrator::Integrator(view_mode_t mode)
    : m_mode(mode)
    , m_encoding(DEPTH_ENCODING_LEGACY)
    , m_max_distance(1000.0)
{}

void Integrator::configure(const std::map<std::string, std::string> &options)
{
    auto mode_it = options.find("mode");
    if (mode_it != options.end()) {
        const std::string &v = mode_it->second;
        if      (v == "depth")    m_mode = VIEW_DEPTH;
        else if (v == "stencil")  m_mode = VIEW_STENCIL;
        else if (v == "normal")   m_mode = VIEW_NORMAL;
        else if (v == "uv")       m_mode = VIEW_UV;
        else if (v == "emission") m_mode = VIEW_EMISSION;
    }

    auto enc_it = options.find("depth_encoding");
    if (enc_it != options.end()) {
        const std::string &v = enc_it->second;
        if      (v == "linear")  m_encoding = DEPTH_ENCODING_LINEAR;
        else if (v == "log")     m_encoding = DEPTH_ENCODING_LOG;
        else if (v == "inverse") m_encoding = DEPTH_ENCODING_INVERSE;
        else                     m_encoding = DEPTH_ENCODING_LEGACY;
    }

    auto dist_it = options.find("max_distance");
    if (dist_it != options.end()) {
        char *end = nullptr;
        double v = std::strtod(dist_it->second.c_str(), &end);
        if (!(end == dist_it->second.c_str() || !end || *end != '\0') && v > 0.0) {
            m_max_distance = (nmath::scalar_t)v;
        }
    }
}

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->scene.get_camera(ctx->params.camera);

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t sample;
        tile->samples.pop(sample);

        nimg::ColorRGBAf color_pixel;
        tile->read(sample.pixel.x, sample.pixel.y, color_pixel);

        xtcore::Ray ray = cam->get_primary_ray(
              sample.coords.x, sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height));

        xtcore::hit_record_t hit_record;
        const bool found_hit = ctx->scene.intersection(ray, hit_record);

        switch (m_mode) {
            case VIEW_DEPTH: {
                nmath::scalar_t depth = color_pixel.r();
                if (found_hit) {
                    const nmath::scalar_t dist = (ray.origin - hit_record.point).length();
                    nmath::scalar_t encoded = 0.0;

                    switch (m_encoding) {
                        case DEPTH_ENCODING_LINEAR: {
                            const nmath::scalar_t nd = std::min(dist, m_max_distance) / m_max_distance;
                            encoded = 1.0 - nd;
                            break;
                        }
                        case DEPTH_ENCODING_LOG: {
                            const nmath::scalar_t d = std::min(dist, m_max_distance);
                            const nmath::scalar_t den = std::log((nmath::scalar_t)1.0 + m_max_distance);
                            encoded = den > (nmath::scalar_t)0.0
                                ? (nmath::scalar_t)1.0 - std::log((nmath::scalar_t)1.0 + d) / den
                                : (nmath::scalar_t)0.0;
                            break;
                        }
                        case DEPTH_ENCODING_INVERSE:
                            encoded = (nmath::scalar_t)1.0 / ((nmath::scalar_t)1.0 + dist);
                            break;
                        case DEPTH_ENCODING_LEGACY:
                        default: {
                            const nmath::scalar_t safe = std::max((nmath::scalar_t)1.000001, dist);
                            encoded = (nmath::scalar_t)1.0 / std::log(safe);
                            break;
                        }
                    }

                    encoded = clamp01(encoded);
                    depth += encoded * sample.weight;
                    color_pixel = nimg::ColorRGBAf(depth, depth, depth, 1);
                }

                color_pixel.a(1);

                raygraph::path_t path;
                raygraph::sample_t sample0;
                raygraph::sample_t sample1;
                sample0.position = ray.origin;
                sample1.position = found_hit
                    ? hit_record.point
                    : (ray.origin + ray.direction * m_max_distance);
                sample1.color = color_pixel;
                path.samples.emplace_back(std::move(sample0));
                path.samples.emplace_back(std::move(sample1));
                tile->raygraph_bundle.paths.emplace_back(std::move(path));
                break;
            }

            case VIEW_STENCIL: {
                if (found_hit) color_pixel = nimg::ColorRGBAf(1, 1, 1, color_pixel.a() + sample.weight);
                break;
            }

            case VIEW_NORMAL: {
                nmath::Vector3f acc_normal = nmath::Vector3f(color_pixel.r(), color_pixel.g(), color_pixel.b());
                acc_normal = acc_normal * 2.0f - 1.0f;
                if (found_hit) {
                    const nmath::Vector3f shading_normal = resolve_shading_normal(ctx, hit_record);
                    nmath::Vector3f new_normal = (shading_normal + acc_normal) * 0.5f;
                    acc_normal = (new_normal.normalized() + 1.0f) * 0.5f;
                }
                color_pixel = nimg::ColorRGBAf(acc_normal.x, acc_normal.y, acc_normal.z, 1.0f);
                break;
            }

            case VIEW_UV: {
                nmath::Vector3f acc_uv = nmath::Vector3f(color_pixel.r(), color_pixel.g(), color_pixel.b());
                nmath::scalar_t alpha = color_pixel.a();
                nmath::scalar_t alpha_sample = 0.f;

                if (found_hit) {
                    const nmath::Vector2f uv(hit_record.texcoord.x, hit_record.texcoord.y);
                    acc_uv += nmath::Vector3f(uv.x, uv.y, 0.0f) * sample.weight;
                    alpha_sample += sample.weight;
                }

                color_pixel = nimg::ColorRGBAf(acc_uv.x, acc_uv.y, acc_uv.z, 0.);
                color_pixel.a(alpha + sample.weight * alpha_sample);
                break;
            }

            case VIEW_EMISSION: {
                nimg::ColorRGBf acc_emission = nimg::ColorRGBf(color_pixel.r(), color_pixel.g(), color_pixel.b());
                if (found_hit) {
                    HASH_UINT64 matid = ctx->scene.m_objects[hit_record.id_object]->material;
                    xtcore::asset::IMaterial *mat = ctx->scene.m_materials[matid];
                    if (mat) acc_emission += mat->get_sample("emissive", hit_record.texcoord) * sample.weight;
                }
                color_pixel = acc_emission;
                break;
            }
        }

        tile->write(floor(sample.pixel.x), floor(sample.pixel.y), color_pixel);
    }
}

        } /* namespace debug_views */
    } /* namespace integrator */
} /* namespace xtcore */
