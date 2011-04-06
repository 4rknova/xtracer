/*

    This file is part of xtracer.

    spscheme.hpp
    Space partitioning scheme

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

#ifndef XTRACER_SPSCHEME_HPP_INCLUDED
#define XTRACER_SPSCHEME_HPP_INCLUDED

#include <list>
#include <nmath/ray.h>

#include "err.h"
#include "geometry.hpp"

class SPScheme
{
	friend class Scene;
	protected:
		SPScheme();
		~SPScheme();

		xt_status_t build(std::list<Geometry *> &g);
		real_t trace(Ray &ray, Geometry *obj);

	private:
		std::list<Geometry *> *m_p_geometry;
};

#endif /* XTRACER_SPSCHEME_HPP_INCLUDED */
