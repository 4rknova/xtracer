#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <nmath/precision.h>
#include <xtcore/aa.h>
#include "integrator.h"
#include "util/raygraph.h"

namespace xtcore {
    namespace integrator {
        namespace depth {

namespace {

inline nmath::scalar_t clamp01(nmath::scalar_t v)
{
    return std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, v));
}

} // namespace

Integrator::Integrator()
    : m_encoding(DEPTH_ENCODING_LEGACY)
    , m_max_distance(1000.0)
{}

void Integrator::configure(const std::map<std::string, std::string> &options)
{
    auto enc_it = options.find("depth_encoding");
    if (enc_it != options.end()) {
        const std::string &v = enc_it->second;
        if (v == "linear") m_encoding = DEPTH_ENCODING_LINEAR;
        else if (v == "log") m_encoding = DEPTH_ENCODING_LOG;
        else if (v == "inverse") m_encoding = DEPTH_ENCODING_INVERSE;
        else m_encoding = DEPTH_ENCODING_LEGACY;
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

	    xtcore::hit_record_t hit_record;

        xtcore::Ray ray = cam->get_primary_ray(
              sample.coords.x, sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height)
        );

        float depth = 0.f;

        bool found_hit = ctx->scene.intersection(ray, hit_record);

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
            depth = color_pixel.r() + encoded * sample.weight;
            color_pixel = nimg::ColorRGBAf(depth, depth, depth, 1);
        }

        color_pixel.a(1);

        raygraph::path_t path;
        raygraph::sample_t sample0, sample1;

        sample0.position = ray.origin;
        sample1.position = found_hit
            ? hit_record.point
            : (ray.origin + ray.direction * m_max_distance);
        sample1.color = color_pixel;
        path.samples.emplace_back(std::move(sample0));
        path.samples.emplace_back(std::move(sample1));
        tile->raygraph_bundle.paths.emplace_back(std::move(path));

        tile->write(floor(sample.pixel.x), floor(sample.pixel.y), color_pixel);
    }
}

        } /* namespace depth */
    } /* namespace integrator */
} /* namespace xtcore */
