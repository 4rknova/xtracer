#include <nmath/precision.h>
#include <nmath/sample.h>
#include <xtcore/aa.h>
#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace normal {

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->scene.get_camera(ctx->params.camera);

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t sample;
        tile->samples.pop(sample);

	    xtcore::hit_record_t hit_record;

        nimg::ColorRGBAf color_pixel;

        tile->read(sample.pixel.x, sample.pixel.y, color_pixel);

        NMath::Vector3f  acc_normal = NMath::Vector3f(color_pixel.r(), color_pixel.g(), color_pixel.b());
        NMath::scalar_t  alpha  = color_pixel.a();

        NMath::scalar_t alpha_sample = 0.f;

        xtcore::Ray ray = cam->get_primary_ray(
              sample.coords.x, sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height));

        if (ctx->scene.intersection(ray, hit_record)) {
            acc_normal += (hit_record.normal + 1.0f) * 0.5f * sample.weight;
            alpha_sample += sample.weight;
        }

        color_pixel = nimg::ColorRGBAf(acc_normal.x, acc_normal.y, acc_normal.z, 0.);
        color_pixel.a(alpha + sample.weight * alpha_sample);
        tile->write(floor(sample.pixel.x), floor(sample.pixel.y), color_pixel);
    }
}

        } /* namespace normal */
    } /* namespace integrator */
} /* namespace xtcore */
