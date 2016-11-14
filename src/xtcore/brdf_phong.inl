#ifndef XTCORE_BRDF_PHONG_INL_INCLUDED
#define XTCORE_BRDF_PHONG_INL_INCLUDED

#include <iostream>
#include <math.h>

#ifndef XTCORE_BRDF_PHONG_H_INCLUDED
    #error "brdf_phong.h must be included before brdf_phong.inl"
#endif /* XTCORE_BRDF_PHONG_H_INCLUDED */

inline nimg::ColorRGBf phong(const NMath::Vector3f &campos
                           , const NMath::Vector3f &lightpos
                           , const NMath::IntInfo *info
                           , const nimg::ColorRGBf &light
                           , const NMath::scalar_t ks
                           , const NMath::scalar_t kd
                           , const NMath::scalar_t specexp
                           , const nimg::ColorRGBf &diffuse
                           , const nimg::ColorRGBf &specular)
{
	// calculate the light direction vector
	NMath::Vector3f lightdir = lightpos - info->point;
	lightdir.normalize();

	// calculate the normal - light dot product
	NMath::scalar_t d = dot(lightdir, info->normal);

	if (d < 0.0) d = 0;

	NMath::Vector3f ray = campos - info->point;
	ray.normalize();

	NMath::Vector3f r = lightdir.reflected(info->normal);
	r.normalize();

	NMath::scalar_t rmv = dot(r, ray);

	if (rmv < 0.0) rmv = 0;

	return ((kd * d * diffuse) + (ks * pow((long double)rmv, (long double)specexp) * specular)) * light;
}

#endif /* XTCORE_BRDF_PHONG_INL_INCLUDED */
