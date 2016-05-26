#ifndef XTRACER_PHONG_HPP_INCLUDED
#define XTRACER_PHONG_HPP_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nimg/color.hpp>

#include "intinfo.h"

using NMath::scalar_t;
using NMath::Vector3f;
using NMath::IntInfo;
using NImg::ColorRGBf;

inline ColorRGBf phong(const Vector3f &campos, const Vector3f &lightpos,
					 const IntInfo *info, const ColorRGBf &light,
					 scalar_t ks, scalar_t kd, scalar_t specexp,
					 const ColorRGBf &diffuse, const ColorRGBf &specular);

inline ColorRGBf blinn_phong(const Vector3f &campos, const Vector3f &lightpos,
						   const IntInfo *info, const ColorRGBf &light,
						   scalar_t ks, scalar_t kd, scalar_t specexp,
						   const ColorRGBf &diffuse, const ColorRGBf &specular);

#include "phong.inl"

#endif /* XTRACER_PHONG_HPP_INCLUDED */
