#include <iostream>
#include <vector>
#include <iomanip>

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
        namespace pathtracer {

nimg::ColorRGBf Integrator::eval(size_t depth, const xtcore::Ray &ray)
{
    if (depth == 0) return nimg::ColorRGBf(0,0,0);

    xtcore::HitRecord info;
    bool hit = ctx->scene.intersection(ray, info);

    if (hit) {
        const xtcore::asset::IMaterial *m = ctx->scene.get_material(info.id_object);

        Ray r;
        ColorRGBf c;
        bool path_continues = m->sample_path(r, c, info);
        if (path_continues) c = c * eval(--depth, r);
        return c;
    }

    return ctx->scene.sample_environment(ray.direction);
}

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera  *cam = ctx->scene.get_camera(ctx->params.camera);

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t aa_sample;
        tile->samples.pop(aa_sample);

        nimg::ColorRGBAf color_pixel;

        tile->read(aa_sample.pixel.x, aa_sample.pixel.y, color_pixel);

        xtcore::Ray ray = cam->get_primary_ray(
              aa_sample.coords.x, aa_sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height)
        );

        color_pixel += eval(ctx->params.rdepth, ray) * aa_sample.weight;

        color_pixel.a(1);
        tile->write(floor(aa_sample.pixel.x), floor(aa_sample.pixel.y), color_pixel);
    }
}

        } /* namespace pathtracer */
    } /* namespace integrator */
} /* namespace xtcore */
