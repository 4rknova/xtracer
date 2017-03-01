#include "mat_lambert.h"
#include "log.h"

namespace xtracer {
    namespace assets {

nimg::ColorRGBf MaterialLambert::shade(
       const NMath::Vector3f &cam_position
     , const NMath::Vector3f &light_position
     , const nimg::ColorRGBf &light_intensity
     , const NMath::IntInfo  &info) const
{
    NMath::Vector3f light_dir = light_position - info.point;
    light_dir.normalize();

    NMath::scalar_t d = dot(light_dir, info.normal);

    nimg::ColorRGBf res = get_sample(MAT_SAMPLER_EMISSIVE, info.texcoord);

    if (d < 0) return res;

    res +=  light_intensity *
            d * get_sample(MAT_SAMPLER_DIFFUSE , info.texcoord);
    return res;
}

    } /* namespace assets */
} /* namespace xtracer */
