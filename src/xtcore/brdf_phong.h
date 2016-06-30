#ifndef XTCORE_BRDF_PHONG_H_INCLUDED
#define XTCORE_BRDF_PHONG_H_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/intinfo.h>
#include <nimg/color.hpp>

using NMath::scalar_t;
using NMath::Vector3f;
using NMath::IntInfo;
using NImg::ColorRGBf;

inline ColorRGBf phong(const Vector3f &campos, const Vector3f &lightpos,
		  			   const IntInfo *info, const ColorRGBf &light,
					   scalar_t ks, scalar_t kd, scalar_t specexp,
					   const ColorRGBf &diffuse, const ColorRGBf &specular);

#include "brdf_phong.inl"

#endif /* XTCORE_BRDF_PHONG_H_INCLUDED */
