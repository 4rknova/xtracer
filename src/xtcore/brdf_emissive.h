#ifndef XTCORE_BRDF_EMISSIVE_H_INCLUDED
#define XTCORE_BRDF_EMISSIVE_H_INCLUDED

#include <nmath/vector.h>
#include <nmath/intinfo.h>
#include <nimg/color.h>

inline nimg::ColorRGBf emission(const nimg::ColorRGBf &intensity);

#include "brdf_emissive.inl"

#endif /* XTCORE_BRDF_EMISSIVE_H_INCLUDED */
