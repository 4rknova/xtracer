#include <nmath/precision.h>
#include <nmath/sample.h>
#include <xtcore/aa.h>
#include <xtcore/material.h>
#include <xtcore/math/sampling_util.h>
#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace normal {

namespace {

inline nmath::Vector3f resolve_shading_normal(
      xtcore::render::context_t *ctx
    , const xtcore::hit_record_t &hit_record)
{
    nmath::Vector3f n = hit_record.normal.normalized();
    if (!ctx) return n;

    const xtcore::asset::IMaterial *mat = ctx->scene.get_material(hit_record.id_object);
    if (!mat || !mat->has_sampler(MAT_SAMPLER_NORMAL)) return n;

    const nimg::ColorRGBf tex = mat->get_sample(MAT_SAMPLER_NORMAL, hit_record.texcoord);
    nmath::Vector3f tangent_space_n(
        (nmath::scalar_t)(tex.r() * 2.0f - 1.0f),
        (nmath::scalar_t)(tex.g() * 2.0f - 1.0f),
        (nmath::scalar_t)(tex.b() * 2.0f - 1.0f)
    );
    if (tangent_space_n.length() <= (nmath::scalar_t)EPSILON) return n;
    tangent_space_n.normalize();

    const nmath::Vector3f t = xtcore::math::sampling::build_tangent(n);
    nmath::Vector3f b = nmath::cross(n, t);
    if (b.length() <= (nmath::scalar_t)EPSILON) return n;
    b.normalize();

    nmath::Vector3f mapped = (t * tangent_space_n.x) + (b * tangent_space_n.y) + (n * tangent_space_n.z);
    if (mapped.length() <= (nmath::scalar_t)EPSILON) return n;
    mapped.normalize();

    if (nmath::dot(mapped, n) <= (nmath::scalar_t)0.0) return n;
    return mapped;
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

        nmath::Vector3f  acc_normal = nmath::Vector3f(color_pixel.r(), color_pixel.g(), color_pixel.b());
        acc_normal = acc_normal * 2.0f - 1.0f;

        xtcore::Ray ray = cam->get_primary_ray(
              sample.coords.x, sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height));

        if (ctx->scene.intersection(ray, hit_record)) {
            const nmath::Vector3f shading_normal = resolve_shading_normal(ctx, hit_record);
            nmath::Vector3f new_normal = (shading_normal + acc_normal) * 0.5f;
            acc_normal = (new_normal.normalized() + 1.0f) * 0.5f;
        }

        color_pixel = nimg::ColorRGBAf(acc_normal.x, acc_normal.y, acc_normal.z, 1.0f);
        tile->write(floor(sample.pixel.x), floor(sample.pixel.y), color_pixel);
    }
}

        } /* namespace normal */
    } /* namespace integrator */
} /* namespace xtcore */
