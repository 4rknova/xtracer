#include "ods.h"

namespace xtcore {
    namespace camera {

ODS::ODS()
    : ipd(XT_CAM_DEFAULT_IPD)
{}

const char* ODS::get_type() const
{
    return "Omni-Directional Stereo";
}

Ray ODS::get_primary_ray(float x, float y, float width, float height)
{
    Ray ray;
    float h = height * .5f;
    float scale  = ipd * .5f * ((y < h) ? -1 : 1);
    if (y > h) y = y - h;

    // Calculate theta & phi angles
    float theta = (x / width) * 2.f * nmath::PI;
    float phi   = nmath::PI * .5f - (y / h) * nmath::PI;

    ray.origin    = nmath::Vector3f(cos(theta), 0, sin(theta)) * scale + position;
    ray.direction = nmath::Vector3f(sin(theta) * cos(phi), sin(phi), -cos(theta) * cos(phi));

    nmath::Matrix4x4f m;
    m.rotate(orientation);
    ray.direction.transform(m);

    return ray;
}

    } /* namespace camera */
} /* namespace xtcore */
