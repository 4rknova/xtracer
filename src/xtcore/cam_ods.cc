#include "cam_ods.h"

CamODS::CamODS()
    : ipd(XT_CAM_DEFAULT_IPD)
{}

CamODS::~CamODS()
{}

NMath::Ray CamODS::get_primary_ray(float x, float y, float width, float height)
{
    NMath::Ray ray;

    float h = height / 2;

    if (y > h) y = y - h;

    // Calculate theta & phi angles
    float theta = (x / width ) * 2.f  * NMath::PI - NMath::PI;
    float phi   = NMath::PI * .5f - (y/h) * NMath::PI;

    float offset = ipd * .5f;
    float scale  = offset * ((y < h) ? -1 : 1);

    ray.origin    = NMath::Vector3f(cos(theta), 0, sin(theta)) * scale + position;
    ray.direction = NMath::Vector3f(sin(theta) * cos(phi), sin(phi), cos(theta) * cos(phi));

    return ray;
}
