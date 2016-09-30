#ifndef XTCORE_BRDF_BLINN_INL_INCLUDED
#define XTCORE_BRDF_BLINN_INL_INCLUDED

#include <iostream>
#include <math.h>

#ifndef XTCORE_BRDF_BLINN_H_INCLUDED
    #error "brdf_blinn.h must be included before brdf_blinn.inl"
#endif /* XTCORE_BRDF_BLINN_H_INCLUDED */

inline ColorRGBf blinn(const Vector3f &campos, const Vector3f &lightpos, const IntInfo *info,
					   const ColorRGBf &light, scalar_t ks, scalar_t kd, scalar_t specexp,
					   const ColorRGBf &diffuse, const ColorRGBf &specular)
{
	// calculate the light direction vector
	Vector3f lightdir = lightpos - info->point;
	lightdir.normalize();

	// calculate the normal - light dot product
	scalar_t d = dot(lightdir, info->normal);

	if (d < 0.0)
		d = 0;

	Vector3f ray = campos - info->point;
	ray.normalize();

	Vector3f r = lightdir + ray;
	r.normalize();

	scalar_t rmv = dot(r, info->normal);

	if (rmv < 0.0)
		rmv = 0;

	return ((kd * d * diffuse) + (ks * pow(rmv, specexp) * specular)) * light;
}

#endif /* XTCORE_BRDF_BLINN_INL_INCLUDED */
