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
        namespace uv {

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->scene.get_camera(ctx->params.camera);

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t sample;
        tile->samples.pop(sample);

	    xtcore::HitRecord info;

        nimg::ColorRGBAf color_pixel;

        tile->read(sample.pixel.x, sample.pixel.y, color_pixel);

        NMath::Vector3f  acc_uv = NMath::Vector3f(color_pixel.r(), color_pixel.g(), color_pixel.b());
        NMath::scalar_t  alpha  = color_pixel.a();

        NMath::scalar_t alpha_sample = 0.f;

        xtcore::Ray ray = cam->get_primary_ray(
              sample.coords.x, sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height));

        if (ctx->scene.intersection(ray, info)) {
            acc_uv += (info.texcoord * 0.5f + 0.5f) * sample.weight;
            alpha_sample += sample.weight;
        }

        color_pixel = nimg::ColorRGBAf(acc_uv.x, acc_uv.y, acc_uv.z, 0.);
        color_pixel.a(alpha + sample.weight * alpha_sample);
        tile->write(floor(sample.pixel.x), floor(sample.pixel.y), color_pixel);
    }
}

        } /* namespace uv */
    } /* namespace integrator */
} /* namespace xtcore */
