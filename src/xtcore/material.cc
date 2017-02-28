#include <nimg/luminance.h>
#include "material.h"

namespace xtracer {
    namespace assets {

IMaterial::IMaterial()
	: ambient(nimg::ColorRGBf(1, 1, 1))
	, diffuse(nimg::ColorRGBf(1, 1, 1))
	, specular(nimg::ColorRGBf(1, 1, 1))
	, emissive(nimg::ColorRGBf(0, 0, 0))
	, kspec(0.0)
	, kdiff(1.0)
	, ksexp(60)
	, roughness(0)
	, reflectance(0.0)
	, transparency(0.0)
	, ior(1.5)
{}

IMaterial::~IMaterial()
{}

bool IMaterial::is_emissive() const
{
    return nimg::eval::luminance(emissive) > 0;
}

    } /* namespace assets */
} /* namespace xtracer */
