#ifndef XTRACER_MATERIAL_HPP_INCLUDED
#define XTRACER_MATERIAL_HPP_INCLUDED

#include <nimg/color.hpp>
#include <nmath/intinfo.h>

#include "camera.h"
#include "light.hpp"

using NMath::IntInfo;
using NImg::ColorRGBf;

enum MATERIAL_TYPE
{
	MATERIAL_LAMBERT,		// Lambert
	MATERIAL_PHONG,			// Phong
	MATERIAL_BLINNPHONG		// Blinn-Phong
};

class Material
{
	public:
		Material();
		~Material();

		// shader
		ColorRGBf shade(const Camera *cam, const Light *light, ColorRGBf &texcolor, const IntInfo &info);

		// properties
		ColorRGBf ambient;		// ambient intensity
		ColorRGBf diffuse;		// diffuse intensity
		ColorRGBf specular;		// specular intensity

		scalar_t kspec;			// specular constant
		scalar_t kdiff;			// diffuse constant
		scalar_t ksexp;			// specular exponential
		scalar_t roughness;		// roughness (Ideally equal to exponent)

		scalar_t reflectance;	// reflectance
		scalar_t transparency;	// transparency
		scalar_t ior;			// index of refraction

		MATERIAL_TYPE type;
};

#endif /* XTRACER_MATERIAL_HPP_INCLUDED */
