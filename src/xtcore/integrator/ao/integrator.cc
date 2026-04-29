#include <cstdlib>
#include <nmath/precision.h>
#include <nmath/sample.h>
#include <xtcore/aa.h>
#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace ao {

Integrator::Integrator()
    : m_max_distance(100.0)
{}

void Integrator::configure(const std::map<std::string, std::string> &options)
{
    auto it = options.find("max_distance");
    if (it == options.end()) return;

    char *end = nullptr;
    double v = std::strtod(it->second.c_str(), &end);
    if (end == it->second.c_str() || !end || *end != '\0') return;
    if (v <= 0.0) return;
    m_max_distance = (nmath::scalar_t)v;
}

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->active_camera();

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t sample;
        tile->samples.pop(sample);

	    xtcore::hit_record_t hit_record;

        nimg::ColorRGBAf color_pixel;

        tile->read(sample.pixel.x, sample.pixel.y, color_pixel);

        nmath::Vector3f  acc_ao = nmath::Vector3f(color_pixel.r(), color_pixel.g(), color_pixel.b());

        xtcore::Ray ray = cam->get_primary_ray(
              sample.coords.x, sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height));

        float w = 1.0f;

        if (ctx->scene->intersection(ray, hit_record)) {
            Ray ao_ray;
            ao_ray.direction = nmath::sample::hemisphere(hit_record.normal, hit_record.normal);
            ao_ray.origin    = hit_record.point + hit_record.normal * EPSILON;

            xtcore::hit_record_t ao_hit_record;

            if (ctx->scene->intersection(ao_ray, ao_hit_record)) {
                float dist = (ao_hit_record.point - ao_ray.origin).length();
                w = nmath::min((nmath::scalar_t)dist, m_max_distance) / m_max_distance;
            }
        }

        acc_ao += w * sample.weight;

        color_pixel = nimg::ColorRGBAf(acc_ao.x, acc_ao.y, acc_ao.z, 1.f);
        tile->write(floor(sample.pixel.x), floor(sample.pixel.y), color_pixel);
    }
}

        } /* namespace ao */
    } /* namespace integrator */
} /* namespace xtcore */
