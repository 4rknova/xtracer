#ifndef XTCORE_BRDF_BLINN_H_INCLUDED
#define XTCORE_BRDF_BLINN_H_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/intinfo.h>
#include <nimg/color.h>

using NMath::scalar_t;
using NMath::Vector3f;
using NMath::IntInfo;
using nimg::ColorRGBf;

inline ColorRGBf blinn(const Vector3f &campos, const Vector3f &lightpos,
					   const IntInfo *info, const ColorRGBf &light,
					   scalar_t ks, scalar_t kd, scalar_t specexp,
					   const ColorRGBf &diffuse, const ColorRGBf &specular);

#include "brdf_blinn.inl"

#endif /* XTCORE_BRDF_BLINN_H_INCLUDED */
