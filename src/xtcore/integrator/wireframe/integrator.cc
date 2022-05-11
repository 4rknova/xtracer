#include <nmath/precision.h>
#include <xtcore/aa.h>
#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace wireframe {

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera  *cam = ctx->scene.get_camera(ctx->params.camera);

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t aa_sample;
        tile->samples.pop(aa_sample);

        nimg::ColorRGBAf color_pixel;
        tile->read(aa_sample.pixel.x, aa_sample.pixel.y, color_pixel);

        Ray ray = cam->get_primary_ray(
              aa_sample.coords.x
            , aa_sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height)
        );

        xtcore::hit_record_t hit_record;
        bool found_hit = ctx->scene.intersection(ray, hit_record);

        if (found_hit) color_pixel = nimg::ColorRGBAf(1, 1, 1, color_pixel.a() + aa_sample.weight);

        tile->write(floor(aa_sample.pixel.x), floor(aa_sample.pixel.y), color_pixel);
    }
}

        } /* namespace wireframe */
    } /* namespace integrator */
} /* namespace xtcore */
