#include <iostream>
#include <vector>
#include <iomanip>
#include <omp.h>

#include <nmath/precision.h>
#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nmath/sample.h>
#include <xtcore/math/plane.h>
#include <nimg/luminance.h>
#include <ncf/util.h>
#include <xtcore/tile.h>
#include <xtcore/aa.h>

#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace depth {

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->scene.get_camera(ctx->params.camera);

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t sample;
        tile->samples.pop(sample);

		nimg::ColorRGBAf color_pixel;
        nimg::ColorRGBf  color_sample;

        tile->read(sample.pixel.x, sample.pixel.y, color_pixel);

	    xtcore::HitRecord info;

        xtcore::Ray ray = cam->get_primary_ray(
              sample.coords.x, sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height)
        );

        float depth = 0.f;

        if (ctx->scene.intersection(ray, info)) {
            depth = 1. / log((ray.origin - info.point).length());
        }

        color_sample += nimg::ColorRGBf(depth, depth, depth) * (1. / ctx->params.samples);
        tile->write(floor(sample.pixel.x), floor(sample.pixel.y), nimg::ColorRGBf(color_pixel) + color_sample * sample.weight);
    }
}

        } /* namespace depth */
    } /* namespace integrator */
} /* namespace xtcore */
