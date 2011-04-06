/*

    This file is part of xtracer.

    spscheme.cpp
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

#include "spscheme.hpp"

SPScheme::SPScheme()
{}

SPScheme::~SPScheme()
{}

xt_status_t SPScheme::build(std::list<Geometry *> *g)
{
	return XT_STATUS_OK;
}

real_t SPScheme::trace(Ray *ray, Geometry *obj)
{

	std::list<Geometry *>::iterator it;

	for (it = m_p_scene.geometry.begin(); it != m_p_scene.geometry.end(); it++)
	{
		(*it)->collision(primary);
		if (pdepth < NM_INFINITY)
		{
			// Test jere
		}
	}


}

