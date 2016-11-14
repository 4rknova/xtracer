#ifndef XTCORE_BRDF_LAMBERT_INL_INCLUDED
#define XTCORE_BRDF_LAMBERT_INL_INCLUDED

#ifndef XTCORE_BRDF_LAMBERT_H_INCLUDED
    #error "brdf_lambert.h must be included before brdf_lambert.inl"
#endif /* XTCORE_BRDF_LAMBERT_H_INCLUDED */

inline nimg::ColorRGBf lambert(const NMath::Vector3f &lightpos
                             , const NMath::IntInfo *info
				   	         , const nimg::ColorRGBf &light
                             , const nimg::ColorRGBf &diffuse)
{
	// calculate the light vector
	NMath::Vector3f lightdir = lightpos - info->point;
	lightdir.normalize();

	// calculate the normal - light dot product
	NMath::scalar_t d = dot(lightdir, info->normal);

	return d > 0 ? d * diffuse * light : nimg::ColorRGBf(0, 0 ,0);
}

#endif /* XTCORE_BRDF_LAMBERT_INL_INCLUDED */
