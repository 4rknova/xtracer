#include "erp.h"

namespace xtcore {
    namespace camera {

Ray ERP::get_primary_ray(float x, float y, float width, float height)
{
    Ray ray;
    // Calculate theta & phi angles
    float phi   = (x / width ) * NMath::PI * 2.f;
    float theta = (y / height) * NMath::PI;

    ray.origin    = position;
    ray.direction = NMath::Vector3f(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));

    NMath::Matrix4x4f m;
    m.rotate(orientation);
    ray.direction.transform(m);

    return ray;
}

    } /* namespace camera */
} /* namespace xtcore */
