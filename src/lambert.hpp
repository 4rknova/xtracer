#ifndef XTRACER_LAMBERT_HPP_INCLUDED
#define XTRACER_LAMBERT_HPP_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.hpp>

#include "intinfo.h"

using NImg::ColorRGBf;

inline ColorRGBf lambert(const Vector3f &lightpos, const IntInfo *info, const ColorRGBf &light, const ColorRGBf &diffuse);

#include "lambert.inl"

#endif /* XTRACER_LAMBERT_HPP_INCLUDED */
