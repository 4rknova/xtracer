#include <nmath/mutil.h>
#include <cmath>
#include <xtcore/aa.h>
#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace realtime_gl {

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->scene.get_camera(ctx->params.camera);
    if (!cam) return;

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
        nimg::ColorRGBf color_sample;

        if (ctx->scene.intersection(ray, hit_record)) {
            /* Fast primary-visibility preview shading:
            ** normal in view-independent RGB space.
            */
            nmath::Vector3f n = hit_record.normal;
            color_sample = nimg::ColorRGBf(
                std::abs(n.x),
                std::abs(n.y),
                std::abs(n.z)
            );
        }
        else {
            color_sample = ctx->scene.sample_environment(ray.direction);
        }

        color_pixel += color_sample * sample.weight;
        color_pixel.a(1.0f);
        tile->write(floor(sample.pixel.x), floor(sample.pixel.y), color_pixel);
    }
}

        } /* namespace realtime_gl */
    } /* namespace integrator */
} /* namespace xtcore */
