#include "blinnphong.h"

namespace xtcore {
    namespace material {

ColorRGBf BlinnPhong::shade(
       const Vector3f  &cam_position
     , const Vector3f  &light_position
     , const ColorRGBf &light_intensity
     , const HitRecord &info) const
{
    Vector3f light_dir = light_position - info.point;
    light_dir.normalize();

    NMath::scalar_t d = dot(light_dir, info.normal);

    if (d < 0) d = 0;

    Vector3f ray = cam_position - info.point;
    ray.normalize();

    Vector3f r = light_dir + ray;
    r.normalize();

    NMath::scalar_t rmv = dot(r, info.normal);

    if (rmv < 0) rmv = 0;

    ColorRGBf res = light_intensity;
    res *=    (d * get_sample(MAT_SAMPLER_DIFFUSE, info.texcoord))
            + (get_sample(MAT_SAMPLER_SPECULAR, info.texcoord) * pow((long double)rmv, (long double)get_scalar(MAT_SCALART_EXPONENT)));
    return res;
}

    } /* namespace material */
} /* namespace xtcore */
