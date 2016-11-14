#ifndef XTCORE_BRDF_LAMBERT_H_INCLUDED
#define XTCORE_BRDF_LAMBERT_H_INCLUDED

#include <nmath/vector.h>
#include <nmath/intinfo.h>
#include <nimg/color.h>

inline nimg::ColorRGBf lambert(const NMath::Vector3f &lightpos
                             , const NMath::IntInfo *info
				             , const nimg::ColorRGBf &light
                             , const nimg::ColorRGBf &diffuse);

#include "brdf_lambert.inl"

#endif /* XTCORE_BRDF_LAMBERT_H_INCLUDED */
