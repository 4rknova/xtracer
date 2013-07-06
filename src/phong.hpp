/*

    This file is part of xtracer.

    phong.hpp
    Phong

    Copyright (C) 2008, 2010
    Papadopoulos Nikolaos

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General
    Public License along with this program; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301 USA

*/

#ifndef XTRACER_PHONG_HPP_INCLUDED
#define XTRACER_PHONG_HPP_INCLUDED

#include "precision.h"
#include "vector.h"
#include "intinfo.h"
#include "color.hpp"

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
