#include "mat_lambert.h"

void MatLambert::shade(ColorRGBf &res, NMath::Vector3f cam_position, const ILight *light, ColorRGBf &texcolor, const IntInfo &info)
{
    ColorRGBf def = ColorRGBf(0,0,0);

    if (!light) {
        res = def;
        return;
    }

	Vector3f lightdir = (light->position() - info.point).normalized();
	scalar_t d = dot(lightdir, info.normal);

	res = d > 0
        ? d * texcolor * colors["diffuse"] * light->intensity()
        : def;
}
