#include <nmath/sample.h>
#include "macro.h"
#include "blinnphong.h"

namespace xtcore {
    namespace asset {
        namespace material {

bool BlinnPhong::shade(
            ColorRGBf    &intensity
    , const ICamera      *camera
    , const emitter_t    *emitter
    , const hit_record_t &hit_record) const
{
    Vector3f light_dir = (emitter->position - hit_record.point).normalized();

    NMath::scalar_t d = NMath::max(dot(light_dir, hit_record.normal), 0);

    Vector3f ray = camera->position - hit_record.point;
    ray.normalize();

    Vector3f r = light_dir + ray;
    r.normalize();

    NMath::scalar_t rmv = NMath::max(dot(r, hit_record.normal), 0);

    intensity = emitter->intensity;
    intensity *= (d * get_sample(MAT_SAMPLER_DIFFUSE, hit_record.texcoord))
            + (get_sample(MAT_SAMPLER_SPECULAR, hit_record.texcoord) * pow((long double)rmv, (long double)get_scalar(MAT_SCALART_EXPONENT)));

    return true;
}

bool BlinnPhong::sample_path(
            hit_result_t &hit_result
    , const hit_record_t &hit_record
) const
{
    hit_result.ray.origin = hit_record.point + hit_record.normal * EPSILON;

    scalar_t s = get_scalar("reflectance");
    scalar_t k = NMath::prng_c(0.0f, 1.0f); // Reflection probability

    if (k > s) {
        hit_result.intensity = get_sample("diffuse", hit_record.texcoord);
        hit_result.ray.direction = NMath::Sample::diffuse(hit_record.normal);
    }
    else {
        hit_result.intensity = get_sample("specular", hit_record.texcoord);
        scalar_t exp = get_scalar("exponent");
        hit_result.ray.direction = NMath::Sample::lobe(hit_record.normal, -hit_record.incident_direction, exp).normalized();
/*
        hit_result.ray.direction = (hit_result.ray.direction + 1.0) * 0.5;
        nimg::ColorRGBf resc(hit_result.ray.direction.x, hit_result.ray.direction.y, hit_result.ray.direction.z);
        hit_result.intensity = resc;
        return false;
*/
    }

    return true;
}
        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
