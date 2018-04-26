#include "ods.h"

namespace xtcore {
    namespace camera {

ODS::ODS()
    : ipd(XT_CAM_DEFAULT_IPD)
{}

Ray ODS::get_primary_ray(float x, float y, float width, float height)
{
    Ray ray;
    float h = height * .5f;
    float scale  = ipd * .5f * ((y < h) ? -1 : 1);
    if (y > h) y = y - h;

    // Calculate theta & phi angles
    float theta = (x / width) * 2.f * NMath::PI;
    float phi   = NMath::PI * .5f - (y/h) * NMath::PI;

    ray.origin    = NMath::Vector3f(cos(theta), 0, sin(theta)) * scale + position;
    ray.direction = NMath::Vector3f(sin(theta) * cos(phi), sin(phi), -cos(theta) * cos(phi));

    NMath::Matrix4x4f m;
    m.rotate(orientation);
    ray.direction.transform(m);

    return ray;
}

    } /* namespace camera */
} /* namespace xtcore */