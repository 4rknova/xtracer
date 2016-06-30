#ifndef XTCORE_BRDF_LAMBERT_H_INCLUDED
#define XTCORE_BRDF_LAMBERT_H_INCLUDED

#include <nmath/vector.h>
#include <nmath/intinfo.h>
#include <nimg/color.hpp>

using NImg::ColorRGBf;

inline ColorRGBf lambert(const Vector3f &lightpos, const IntInfo *info,
						 const ColorRGBf &light, const ColorRGBf &diffuse);

#include "brdf_lambert.inl"

#endif /* XTCORE_BRDF_LAMBERT_H_INCLUDED */
