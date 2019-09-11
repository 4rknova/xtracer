#include <nmath/sample.h>
#include <nmath/prng.h>
#include <nimg/luminance.h>
#include "macro.h"
#include "phong.h"

namespace xtcore {
    namespace asset {
        namespace material {

bool Phong::shade(
            ColorRGBf    &intensity
    , const ICamera      *camera
    , const emitter_t    *emitter
    , const hit_record_t &hit_record) const
{
    Vector3f light_dir = (emitter->position - hit_record.point).normalized();

    NMath::scalar_t d = NMath::max(0, dot(light_dir, hit_record.normal));

    Vector3f ray = (camera->position - hit_record.point).normalized();

    Vector3f r = (light_dir.reflected(hit_record.normal)).normalized();

    NMath::scalar_t rmv = NMath::max(0, dot(r, ray));

    intensity += emitter->intensity *
            (   (d * get_sample(MAT_SAMPLER_DIFFUSE, hit_record.texcoord))
              + (get_sample(MAT_SAMPLER_SPECULAR, hit_record.texcoord) * pow((long double)rmv, (long double)get_scalar(MAT_SCALART_EXPONENT)))
            );

    return true;
}

bool Phong::sample_path(
            hit_result_t &hit_result
    , const hit_record_t &hit_record
) const
{
    hit_result.ray.origin    = hit_record.point + hit_record.normal * EPSILON;
    scalar_t s = get_scalar("reflectance");
    scalar_t k = NMath::prng_c(0.0f, 1.0f);

    if (k > s) {
        hit_result.intensity = get_sample("diffuse", hit_record.texcoord);
        hit_result.ray.direction = NMath::Sample::diffuse(hit_record.normal);
    }
    else {
        hit_result.intensity = get_sample("specular", hit_record.texcoord);
        scalar_t exp = get_scalar("exponent");
        hit_result.ray.direction = NMath::Sample::lobe(hit_record.normal, -hit_record.incident_direction, exp);
    }

    return true;
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
