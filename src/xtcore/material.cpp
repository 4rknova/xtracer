#include "material.hpp"
#include "brdf.h"

Material::Material()
	: ambient(ColorRGBf(1.0, 1.0, 1.0)),
	  diffuse(ColorRGBf(1.0, 1.0, 1.0)),
	  specular(ColorRGBf(1.0, 1.0, 1.0)),
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

ColorRGBf Material::shade(NMath::Vector3f cam_position, const ILight *light, ColorRGBf &texcolor, const IntInfo &info)
{
	switch(type)
	{
		case MATERIAL_LAMBERT:
			return lambert(
				light->point_sample(),
				&info,
				light->intensity(),
				diffuse * texcolor);

		case MATERIAL_PHONG:
			return phong(
				cam_position,
				light->point_sample(),
				&info,
				light->intensity(),
				kspec, kdiff, ksexp,
				diffuse * texcolor,
				specular);

		case MATERIAL_BLINNPHONG:
			return blinn(
				cam_position,
				light->point_sample(),
				&info,
				light->intensity(),
				kspec, kdiff, ksexp,
				diffuse * texcolor,
				specular);
	}

	// This should never happen
	return ColorRGBf(0, 0, 0);
}
