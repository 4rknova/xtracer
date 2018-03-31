#include "lambert.h"

namespace xtcore {
    namespace material {

ColorRGBf Lambert::shade(
       const Vector3f  &cam_position
     , const Vector3f  &light_position
     , const ColorRGBf &light_intensity
     , const HitRecord &info) const
{
    Vector3f light_dir = light_position - info.point;
    light_dir.normalize();

    NMath::scalar_t d = dot(light_dir, info.normal);

    ColorRGBf res;

    if (d < 0) return res;

    res += light_intensity *
           d * get_sample(MAT_SAMPLER_DIFFUSE , info.texcoord);
    return res;
}

    } /* namespace material */
} /* namespace xtcore */
