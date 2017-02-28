#include "mat_lambert.h"

namespace xtracer {
    namespace assets {

nimg::ColorRGBf MaterialLambert::shade(
       const NMath::Vector3f &cam_position
     , const NMath::Vector3f &light_position
     , const nimg::ColorRGBf &light_intensity
     , const nimg::ColorRGBf &texcolor
     , const NMath::IntInfo &info) const
{
    NMath::Vector3f light_dir = light_position - info.point;
    light_dir.normalize();

    NMath::scalar_t d = dot(light_dir, info.normal);

    return emissive + (d > 0 ? d * diffuse * light_intensity : nimg::ColorRGBf(0,0,0));
}

    } /* namespace assets */
} /* namespace xtracer */
