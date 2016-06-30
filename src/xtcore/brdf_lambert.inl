#ifndef XTCORE_BRDF_LAMBERT_INL_INCLUDED
#define XTCORE_BRDF_LAMBERT_INL_INCLUDED

#ifndef XTCORE_BRDF_LAMBERT_H_INCLUDED
    #error "brdf_lambert.h must be included before brdf_lambert.inl"
#endif /* XTCORE_BRDF_LAMBERT_H_INCLUDED */

inline ColorRGBf lambert(const Vector3f &lightpos, const IntInfo *info,
						 const ColorRGBf &light, const ColorRGBf &diffuse)
{
	// calculate the light vector
	Vector3f lightdir = lightpos - info->point;
	lightdir.normalize();

	// calculate the normal - light dot product
	scalar_t d = dot(lightdir, info->normal);

	return d > 0 ? d * diffuse * light : ColorRGBf(0, 0 ,0);
}

#endif /* XTCORE_BRDF_LAMBERT_INL_INCLUDED */
