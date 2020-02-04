#include <nmath/precision.h>
#include <nmath/sample.h>
#include <xtcore/aa.h>
#include <xtcore/material.h>
#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace emission {

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->scene.get_camera(ctx->params.camera);

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t sample;
        tile->samples.pop(sample);

	    xtcore::hit_record_t hit_record;

        nimg::ColorRGBAf color_pixel;

        tile->read(sample.pixel.x, sample.pixel.y, color_pixel);

        nimg::ColorRGBf acc_emission = nimg::ColorRGBf(color_pixel.r(), color_pixel.g(), color_pixel.b());

        xtcore::Ray ray = cam->get_primary_ray(
              sample.coords.x, sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height));

        if (ctx->scene.intersection(ray, hit_record)) {
            HASH_UINT64 matid = ctx->scene.m_objects[hit_record.id_object]->material;
            xtcore::asset::IMaterial *mat = ctx->scene.m_materials[matid];
            if (mat) {
                acc_emission += mat->get_sample("emissive", hit_record.texcoord) * sample.weight;
            }
        }

        color_pixel = acc_emission;
        tile->write(floor(sample.pixel.x), floor(sample.pixel.y), color_pixel);
    }
}

        } /* namespace emission */
    } /* namespace integrator */
} /* namespace xtcore */
