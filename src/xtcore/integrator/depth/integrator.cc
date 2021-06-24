#include <iostream>
#include <nmath/precision.h>
#include <xtcore/aa.h>
#include "integrator.h"
#include "util/raygraph.h"

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
            depth = 1. / log((ray.origin - hit_record.point).length());
            depth = color_pixel.r() + depth * sample.weight;
            color_pixel = nimg::ColorRGBAf(depth, depth, depth, 1);
        }

        color_pixel.a(1);

        raygraph::path_t path;
        raygraph::sample_t sample0, sample1;

        sample0.position = ray.origin;
        sample1.position = hit_record.point;
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
