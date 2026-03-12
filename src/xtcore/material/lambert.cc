#include <algorithm>
#include <nmath/sample.h>
#include <xtcore/math/sampling_util.h>
#include "macro.h"
#include "lambert.h"

namespace xtcore {
    namespace asset {
        namespace material {

bool Lambert::shade(
            ColorRGBf    &intensity
    , const ICamera      *camera
    , const emitter_t    *emitter
    , const hit_record_t &hit_record) const
{
    UNUSED(camera)

    Vector3f light_dir = emitter->position - hit_record.point;
    light_dir.normalize();

    nmath::scalar_t d = dot(light_dir, hit_record.normal);

    if (d > 0) {
        intensity += emitter->intensity *
               d * get_sample(MAT_SAMPLER_DIFFUSE , hit_record.texcoord);
    }

    return true;
}

bool Lambert::sample_path(
            hit_result_t &hit_result
    , const hit_record_t &hit_record
) const
{
    hit_result.ray.origin    = hit_record.point + hit_record.normal * EPSILON;
    hit_result.ray.direction = nmath::sample::diffuse(hit_record.normal).normalized();
    hit_result.intensity     = get_sample("diffuse", hit_record.texcoord)
                             * dot(hit_record.normal, hit_result.ray.direction);
    return true;
}

bool Lambert::bsdf_eval(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , const Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    const nmath::Vector3f n = hit_record.normal.normalized();
    const nmath::scalar_t cos_i = std::max((nmath::scalar_t)0.0, nmath::dot(n, wi.normalized()));
    const nmath::scalar_t cos_o = std::max((nmath::scalar_t)0.0, nmath::dot(n, wo.normalized()));
    if (cos_i <= (nmath::scalar_t)EPSILON || cos_o <= (nmath::scalar_t)EPSILON) {
        f = ColorRGBf(0.0f, 0.0f, 0.0f);
        pdf = 0.0f;
        return false;
    }

    f = get_sample(MAT_SAMPLER_DIFFUSE, hit_record.texcoord) * ((nmath::scalar_t)1.0 / nmath::PI);
    pdf = cos_i / nmath::PI;
    return true;
}

bool Lambert::bsdf_sample(
            const hit_record_t &hit_record
    , const Vector3f &wo
    , Vector3f &wi
    , ColorRGBf &f
    , scalar_t &pdf
) const
{
    wi = xtcore::math::sampling::sample_cosine_hemisphere(hit_record.normal, pdf);
    return bsdf_eval(hit_record, wo, wi, f, pdf);
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
