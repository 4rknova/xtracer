#include "mat_blinnphong.h"

void MatBlinnPhong::shade(ColorRGBf &res, NMath::Vector3f cam_position, const ILight *light, ColorRGBf &texcolor, const IntInfo &info)
{
    ColorRGBf def = ColorRGBf(0,0,0);

    if (!light) {
        res = def;
        return;
    }

	Vector3f lightdir = (light->position() - info.point).normalized();
	scalar_t d = dot(lightdir, info.normal);


	if (d < 0.0) d = 0;

    Vector3f ray = (cam_position - info.point).normalized();
	Vector3f r = (lightdir + ray).normalized();

	scalar_t rmv = dot(r, info.normal);

    if (rmv < 0.0) rmv = 0;


    res = (( d * texcolor * colors["diffuse"]) + (pow(rmv, scalars["exponent"] )* colors["specular"])) * light->intensity();
}
