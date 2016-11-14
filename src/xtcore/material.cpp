#include <nimg/luminance.h>
#include "brdf.h"
#include "material.hpp"

Material::Material()
	: ambient(nimg::ColorRGBf(1, 1, 1)),
	  diffuse(nimg::ColorRGBf(1, 1, 1)),
	  specular(nimg::ColorRGBf(1, 1, 1)),
	  emissive(nimg::ColorRGBf(0, 0, 0)),
	  kspec(0.0),
	  kdiff(1.0),
	  ksexp(60),
	  roughness(0),
	  reflectance(0.0),
	  transparency(0.0),
	  ior(1.5),
	  type(MATERIAL_LAMBERT)
{}

Material::~Material()
{}

bool Material::is_emissive() const
{
    return (type == MATERIAL_EMISSIVE);
//    return nimg::eval::luminance(emissive) > 0.f;
}

nimg::ColorRGBf Material::shade(const NMath::Vector3f &cam_position
                              , const NMath::Vector3f &light_pos
                              , const nimg::ColorRGBf &light_intensity
                              , const nimg::ColorRGBf &texcolor
                              , const NMath::IntInfo &info)
{
	switch(type)
	{
		case MATERIAL_LAMBERT:
			return lambert(
				light_pos,
				&info,
				light_intensity,
				diffuse * texcolor);

		case MATERIAL_PHONG:
			return phong(
				cam_position,
				light_pos,
				&info,
				light_intensity,
				kspec, kdiff, ksexp,
				diffuse * texcolor,
				specular);

		case MATERIAL_BLINNPHONG:
			return blinn(
				cam_position,
				light_pos,
				&info,
				light_intensity,
				kspec, kdiff, ksexp,
				diffuse * texcolor,
				specular);
        case MATERIAL_EMISSIVE:
            return emission(emissive);
	}

	// This should never happen
	return nimg::ColorRGBf(0, 0, 0);
}
