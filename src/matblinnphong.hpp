/*

    This file is part of xtracer.

    mathblinnphong.hpp
    MatBlinnPhong class

    Copyright (C) 2010, 2011
    Papadopoulos Nikolaos

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General
    Public License along with this library; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301 USA

*/

#ifndef XTRACER_MAT_BLINNPHONG_HPP_INCLUDED
#define XTRACER_MAT_BLINNPHONG_HPP_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>

#include "material.hpp"
#include "light.hpp"

class MatBlinnPhong: public Material
{
	public:
		MatBlinnPhong();

		// shade
		Vector3 shade(const Camera *cam, const Light *light, const IntInfo &info);

		Vector3 specular; 	// specular intensity
		real_t kspec;		// specular constant
		real_t kdiff;		// diffuse constant
		real_t ksexp;		// specular exponential
};

#endif /* XTRACER_MAT_PHONG_HPP_INCLUDED */
