#include "phong.h"

namespace xtcore {
    namespace asset {
        namespace material {

bool Phong::shade(
                ColorRGBf &intensity
        , const ICamera   *camera
        , const emitter_t *emitter
        , const HitRecord &info) const
{
    Vector3f light_dir = (emitter->position - info.point).normalized();

    NMath::scalar_t d = nmath_max(0, dot(light_dir, info.normal));

    Vector3f ray = (camera->position - info.point).normalized();

    Vector3f r = (light_dir.reflected(info.normal)).normalized();

    NMath::scalar_t rmv = nmath_max(0, dot(r, ray));

    intensity += emitter->intensity *
            (   (d * get_sample(MAT_SAMPLER_DIFFUSE, info.texcoord))
              + (get_sample(MAT_SAMPLER_SPECULAR, info.texcoord) * pow((long double)rmv, (long double)get_scalar(MAT_SCALART_EXPONENT)))
            );

    return true;
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
