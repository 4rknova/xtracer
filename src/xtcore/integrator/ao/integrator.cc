#include <nmath/precision.h>
#include <nmath/sample.h>
#include <xtcore/aa.h>
#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace ao {

#define MAX_DISTANCE 100.0f

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->scene.get_camera(ctx->params.camera);

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t sample;
        tile->samples.pop(sample);

	    xtcore::hit_record_t hit_record;

        nimg::ColorRGBAf color_pixel;

        tile->read(sample.pixel.x, sample.pixel.y, color_pixel);

        NMath::Vector3f  acc_ao = NMath::Vector3f(color_pixel.r(), color_pixel.g(), color_pixel.b());

        xtcore::Ray ray = cam->get_primary_ray(
              sample.coords.x, sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height));

        float w = 1.0f;

        if (ctx->scene.intersection(ray, hit_record)) {
            Ray ao_ray;
            ao_ray.direction = NMath::Sample::hemisphere(hit_record.normal, hit_record.normal);
            ao_ray.origin    = hit_record.point + hit_record.normal * EPSILON;

            xtcore::hit_record_t ao_hit_record;

            if (ctx->scene.intersection(ao_ray, ao_hit_record)) {
                float dist = (ao_hit_record.point - ao_ray.origin).length();
                w = NMath::min(dist, MAX_DISTANCE) / MAX_DISTANCE;
            }
        }

        acc_ao += w * sample.weight;

        color_pixel = nimg::ColorRGBAf(acc_ao.x, acc_ao.y, acc_ao.z, 1.f);
        tile->write(floor(sample.pixel.x), floor(sample.pixel.y), color_pixel);
    }
}

        } /* namespace ao */
    } /* namespace integrator */
} /* namespace xtcore */
