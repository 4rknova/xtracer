#include "erp.h"

namespace xtcore {
    namespace camera {

const char* ERP::get_type() const
{
    return "Equirectangular Projection";
}

Ray ERP::get_primary_ray(float x, float y, float width, float height)
{
    Ray ray;
    // Calculate theta & phi angles
    float phi   = (x / width ) * nmath::PI * 2.f;
    float theta = (y / height) * nmath::PI;

    ray.origin    = position;
    ray.direction = nmath::Vector3f(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));

    nmath::Matrix4x4f m;
    m.rotate(orientation);
    ray.direction.transform(m);

    return ray;
}

    } /* namespace camera */
} /* namespace xtcore */
