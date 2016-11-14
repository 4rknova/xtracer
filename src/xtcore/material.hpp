#ifndef XTRACER_MATERIAL_HPP_INCLUDED
#define XTRACER_MATERIAL_HPP_INCLUDED

#include <nimg/color.h>
#include <nmath/intinfo.h>
#include "camera.h"

enum MATERIAL_TYPE
{
	  MATERIAL_LAMBERT		// Lambert
	, MATERIAL_PHONG    	// Phong
	, MATERIAL_BLINNPHONG	// Blinn-Phong
	, MATERIAL_EMISSIVE		// Emissive
};

class Material
{
	public:
		Material();
		~Material();

		// shader
		nimg::ColorRGBf shade(const NMath::Vector3f &cam_position
                            , const NMath::Vector3f &light_pos
                            , const nimg::ColorRGBf &light_intensity
                            , const nimg::ColorRGBf &texcolor
                            , const NMath::IntInfo &info);

		// properties
		nimg::ColorRGBf ambient;		// ambient intensity
		nimg::ColorRGBf diffuse;		// diffuse intensity
		nimg::ColorRGBf specular;		// specular intensity
		nimg::ColorRGBf emissive;		// emissive intensity

		NMath::scalar_t kspec;			// specular constant
		NMath::scalar_t kdiff;			// diffuse constant
		NMath::scalar_t ksexp;			// specular exponential
		NMath::scalar_t roughness;		// roughness (Ideally equal to exponent)

		NMath::scalar_t reflectance;	// reflectance
		NMath::scalar_t transparency;	// transparency
		NMath::scalar_t ior;			// index of refraction

		MATERIAL_TYPE type;

        bool is_emissive() const;
};

#endif /* XTRACER_MATERIAL_HPP_INCLUDED */
