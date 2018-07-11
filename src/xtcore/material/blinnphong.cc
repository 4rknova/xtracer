#include <nmath/sample.h>
#include "blinnphong.h"

namespace xtcore {
    namespace asset {
        namespace material {

bool BlinnPhong::shade(
            ColorRGBf &intensity
    , const ICamera   *camera
    , const emitter_t *emitter
    , const HitRecord &info) const
{
    Vector3f light_dir = (emitter->position - info.point).normalized();

    NMath::scalar_t d = nmath_max(dot(light_dir, info.normal), 0);

    Vector3f ray = camera->position - info.point;
    ray.normalize();

    Vector3f r = light_dir + ray;
    r.normalize();

    NMath::scalar_t rmv = nmath_max(dot(r, info.normal), 0);

    intensity = emitter->intensity;
    intensity *= (d * get_sample(MAT_SAMPLER_DIFFUSE, info.texcoord))
            + (get_sample(MAT_SAMPLER_SPECULAR, info.texcoord) * pow((long double)rmv, (long double)get_scalar(MAT_SCALART_EXPONENT)));

    return true;
}

bool BlinnPhong::sample_path(
            Ray       &ray
    ,       ColorRGBf &color
    , const HitRecord &info) const
{
    ray.origin    = info.point + info.normal * EPSILON;

    ColorRGBf diff = get_sample("diffuse", info.texcoord);
    ColorRGBf spec = get_sample("specular", info.texcoord);
    scalar_t s = get_scalar("reflectance");
    scalar_t k = NMath::prng_c(0.0f, 1.0f);
//    float lumdiff = nimg::eval::luminance(diff);

    if (k > s) {
        color = diff;
        ray.direction = NMath::Sample::diffuse(info.normal);
    }
    else {
        color = spec;
        scalar_t exp = get_scalar("exponent");
        ray.direction = NMath::Sample::lobe(info.normal, -ray.direction, exp);
    }

    return true;
}
        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
