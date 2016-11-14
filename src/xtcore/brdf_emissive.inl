#ifndef XTCORE_BRDF_EMISSIVE_INL_INCLUDED
#define XTCORE_BRDF_EMISSIVE_INL_INCLUDED

#ifndef XTCORE_BRDF_EMISSIVE_H_INCLUDED
    #error "brdf_emissive.h must be included before brdf_emissive.inl"
#endif /* XTCORE_BRDF_EMISSIVE_H_INCLUDED */

inline nimg::ColorRGBf emission(const nimg::ColorRGBf &intensity)
{
    return intensity;
}

#endif /* XTCORE_BRDF_EMISSIVE_INL_INCLUDED */
