#include "mat_phong.h"

namespace xtcore {
    namespace assets {

nimg::ColorRGBf MaterialPhong::shade(
       const NMath::Vector3f &cam_position
     , const NMath::Vector3f &light_position
     , const nimg::ColorRGBf &light_intensity
     , const NMath::IntInfo  &info) const
{
    NMath::Vector3f light_dir = light_position - info.point;
    light_dir.normalize();

    NMath::scalar_t d = dot(light_dir, info.normal);

    if (d < 0) d = 0;

    NMath::Vector3f ray = cam_position - info.point;
    ray.normalize();

    NMath::Vector3f r = light_dir.reflected(info.normal);
    r.normalize();

    NMath::scalar_t rmv = dot(r, ray);

    if (rmv < 0) rmv = 0;

    nimg::ColorRGBf res;

    res +=  light_intensity *
            (   (d * get_sample(MAT_SAMPLER_DIFFUSE, info.texcoord))
              + (get_sample(MAT_SAMPLER_SPECULAR, info.texcoord) * pow((long double)rmv, (long double)get_scalar(MAT_SCALART_EXPONENT)))
            );
    return res;
}

    } /* namespace assets */
} /* namespace xtcore */
