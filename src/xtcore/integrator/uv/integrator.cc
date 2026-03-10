#include <nmath/precision.h>
#include <nmath/sample.h>
#include <cmath>
#include <xtcore/aa.h>
#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace uv {

namespace {

inline nmath::scalar_t wrap01(nmath::scalar_t v)
{
    v = v - (nmath::scalar_t)std::floor((double)v);
    if (v < (nmath::scalar_t)0.0) v += (nmath::scalar_t)1.0;
    return v;
}

} // namespace

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->scene.get_camera(ctx->params.camera);

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t sample;
        tile->samples.pop(sample);

	    xtcore::hit_record_t hit_record;

        nimg::ColorRGBAf color_pixel;

        tile->read(sample.pixel.x, sample.pixel.y, color_pixel);

        nmath::Vector3f  acc_uv = nmath::Vector3f(color_pixel.r(), color_pixel.g(), color_pixel.b());
        nmath::scalar_t  alpha  = color_pixel.a();

        nmath::scalar_t alpha_sample = 0.f;

        xtcore::Ray ray = cam->get_primary_ray(
              sample.coords.x, sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height));

        if (ctx->scene.intersection(ray, hit_record)) {
            const nmath::Vector2f uv(wrap01(hit_record.texcoord.x), wrap01(hit_record.texcoord.y));
            acc_uv += nmath::Vector3f(uv.x, uv.y, 0.0f) * sample.weight;
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
