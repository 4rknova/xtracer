#include <nmath/sample.h>
#include <nmath/prng.h>
#include <xtcore/math/sampling_util.h>
#include "macro.h"
#include "lambert.h"

namespace xtcore {
    namespace asset {
        namespace material {

bool Dielectric::shade(
            ColorRGBf    &intensity
    , const ICamera      *camera
    , const emitter_t    *emitter
    , const hit_record_t &hit_record) const
{
    UNUSED(camera)
    UNUSED(emitter)
    UNUSED(hit_record)
    intensity = ColorRGBf(0.f,0.f,0.f);
    return true;
}

bool Dielectric::sample_path(
            hit_result_t &hit_result
    , const hit_record_t &hit_record
) const
{
    float mat_ior = get_scalar("ior");
    if (mat_ior <= EPSILON) mat_ior = 1.5f;

    float transparency = get_scalar("transparency");
    if (transparency <= 0.0f) transparency = 1.0f;
    if (transparency > 1.0f) transparency = 1.0f;

    float reflectance = get_scalar("reflectance");
    if (reflectance < 0.0f) reflectance = 0.0f;
    if (reflectance > 1.0f) reflectance = 1.0f;

    const float ior_src = (hit_record.ior > EPSILON) ? hit_record.ior : 1.0f;
    float ior_dst = mat_ior;

    nmath::Vector3f n = hit_record.normal.normalized();
    const nmath::Vector3f wi = hit_record.incident_direction.normalized();

    // If the ray is leaving the medium, invert normal and target air IOR.
    if (dot(wi, n) > 0.0f) {
        n = -n;
        ior_dst = 1.0f;
    }

    const float cos_i = nmath::max(0.0f, (float)dot(-wi, n));
    const float fresnel = (float)xtcore::math::sampling::fresnel_dielectric(cos_i, ior_src, ior_dst);
    const bool choose_reflection = (nmath::prng_c(0.0f, 1.0f) < fresnel);
    if (choose_reflection) {
        hit_result.ray.direction = wi.reflected(n).normalized();
        hit_result.ior = ior_src;
        hit_result.intensity = ColorRGBf(1.0f, 1.0f, 1.0f);
    } else {
        hit_result.ray.direction = wi.refracted(n, ior_src, ior_dst).normalized();
        hit_result.ior = ior_dst;
        hit_result.intensity = ColorRGBf(transparency, transparency, transparency);
    }

    hit_result.ray.origin = hit_record.point + hit_result.ray.direction * EPSILON;
    return true;
}

bool Dielectric::bsdf_is_delta() const
{
    return true;
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
