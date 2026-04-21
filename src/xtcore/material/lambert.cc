#include <algorithm>
#include <nmath/sample.h>
#include <xtcore/math/sampling_util.h>
#include "macro.h"
#include "lambert.h"

namespace xtcore {
    namespace asset {
        namespace material {

namespace {

inline Vector3f lambert_shading_normal(const Lambert *mat, const hit_record_t &hit_record)
{
    Vector3f n = hit_record.normal.normalized();
    if (!mat || !mat->has_sampler(MAT_SAMPLER_NORMAL)) return n;

    const ColorRGBf tex = mat->get_sample(MAT_SAMPLER_NORMAL, hit_record.texcoord);
    Vector3f tangent_space_n(
        (nmath::scalar_t)(tex.r() * 2.0f - 1.0f),
        (nmath::scalar_t)(tex.g() * 2.0f - 1.0f),
        (nmath::scalar_t)(tex.b() * 2.0f - 1.0f)
    );
    if (tangent_space_n.length() <= (nmath::scalar_t)EPSILON) return n;
    tangent_space_n.normalize();

    const Vector3f t = xtcore::math::sampling::build_tangent(n);
    Vector3f b = nmath::cross(n, t);
    if (b.length() <= (nmath::scalar_t)EPSILON) return n;
    b.normalize();

    Vector3f mapped = (t * tangent_space_n.x) + (b * tangent_space_n.y) + (n * tangent_space_n.z);
    if (mapped.length() <= (nmath::scalar_t)EPSILON) return n;
    mapped.normalize();

    if (nmath::dot(mapped, n) <= (nmath::scalar_t)0.0) return n;
    return mapped;
}

} // namespace

bool Lambert::shade(
            ColorRGBf    &intensity
    , const ICamera      *camera
    , const emitter_t    *emitter
    , const hit_record_t &hit_record) const
{
    UNUSED(camera)

    Vector3f light_dir = emitter->position - hit_record.point;
    light_dir.normalize();

    const Vector3f n = lambert_shading_normal(this, hit_record);
    nmath::scalar_t d = dot(light_dir, n);

    if (d > 0) {
        intensity += emitter->intensity *
               (d / nmath::PI) * get_sample(MAT_SAMPLER_DIFFUSE , hit_record.texcoord);
    }

    return true;
}

bool Lambert::sample_path(
            hit_result_t &hit_result
    , const hit_record_t &hit_record
) const
{
    nmath::Vector3f n = lambert_shading_normal(this, hit_record);
    const nmath::Vector3f wo = (-hit_record.incident_direction).normalized();
    if (nmath::dot(n, wo) < (nmath::scalar_t)0.0) n = -n;
    hit_result.ray.origin    = hit_record.point + n * EPSILON;
    hit_result.ray.direction = nmath::sample::diffuse(n).normalized();
    hit_result.intensity     = get_sample("diffuse", hit_record.texcoord);
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
    nmath::Vector3f n = lambert_shading_normal(this, hit_record);
    // Two-sided: flip shading normal to match the side wo arrives from
    if (nmath::dot(n, wo.normalized()) < (nmath::scalar_t)0.0) n = -n;
    const nmath::scalar_t cos_i = std::max((nmath::scalar_t)0.0, nmath::dot(n, wi.normalized()));
    if (cos_i <= (nmath::scalar_t)EPSILON) {
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
    nmath::Vector3f n = lambert_shading_normal(this, hit_record);
    if (nmath::dot(n, wo.normalized()) < (nmath::scalar_t)0.0) n = -n;
    wi = xtcore::math::sampling::sample_cosine_hemisphere(n, pdf);
    return bsdf_eval(hit_record, wo, wi, f, pdf);
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
