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

        nmath::Vector3f  acc_normal = nmath::Vector3f(color_pixel.r(), color_pixel.g(), color_pixel.b());
        acc_normal = acc_normal * 2.0f - 1.0f;

        xtcore::Ray ray = cam->get_primary_ray(
              sample.coords.x, sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height));

        if (ctx->scene.intersection(ray, hit_record)) {
            nmath::Vector3f new_normal = (hit_record.normal + acc_normal) * 0.5f;
            acc_normal = (new_normal.normalized() + 1.0f) * 0.5f;
        }

        color_pixel = nimg::ColorRGBAf(acc_normal.x, acc_normal.y, acc_normal.z, 1.0f);
        tile->write(floor(sample.pixel.x), floor(sample.pixel.y), color_pixel);
    }
}

        } /* namespace normal */
    } /* namespace integrator */
} /* namespace xtcore */
