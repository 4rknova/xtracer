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

nimg::ColorRGBf Integrator::eval(size_t depth, hit_result_t &in)
{
    if (depth == 0) return nimg::ColorRGBf(0,0,0);

    xtcore::hit_record_t hit_record;
    bool hit = ctx->scene.intersection(in.ray, hit_record);
    hit_record.ior = in.ior;

    if (hit) {
        const xtcore::asset::IMaterial *m = ctx->scene.get_material(hit_record.id_object);

        xtcore::hit_result_t hit_result;
        bool path_continues = m->sample_path(hit_result, hit_record);
        if (path_continues) {
            in.intensity  = hit_result.intensity * eval(--depth, hit_result);
        }
        return in.intensity;
    }

    return ctx->scene.sample_environment(in.ray.direction);
}

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->scene.get_camera(ctx->params.camera);

    size_t sample_count = tile->samples.count();

    while (sample_count > 0) {
        xtcore::antialiasing::sample_rgba_t aa_sample;
        tile->samples.pop(aa_sample);
        --sample_count;

        nimg::ColorRGBAf color_pixel;
        tile->read(aa_sample.pixel.x, aa_sample.pixel.y, color_pixel);

        hit_result_t hit_result;

        hit_result.intensity = ColorRGBf(1,1,1);
        hit_result.ior = 1.f;
        hit_result.ray = cam->get_primary_ray(
              aa_sample.coords.x, aa_sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height)
        );

        color_pixel += eval(ctx->params.rdepth, hit_result) * aa_sample.weight;
        color_pixel.a(1);
        tile->write(floor(aa_sample.pixel.x), floor(aa_sample.pixel.y), color_pixel);
    }
}

        } /* namespace pathtracer */
    } /* namespace integrator */
} /* namespace xtcore */
