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

SPCRes::SPCRes()
	: geometry(NULL), distance(NM_INFINITY)
{}

SPScheme::SPScheme()
{}

SPScheme::~SPScheme()
{}

xt_status_t SPScheme::build(std::map<std::string, Geometry *> &g)
{  
	m_p_geometry = &g;
	return XT_STATUS_OK;
}

#include <iostream>
SPCRes SPScheme::test(const Ray &ray)
{
	SPCRes res;
	std::map<std::string, Geometry *>::iterator it;

	for (it = m_p_geometry->begin(); it != m_p_geometry->end(); it++)
	{
		/* Check ray colision */
		real_t d = (*it).second->collision(ray);
	
//		if (d< NM_INFINITY)std::cout << ray.origin << "-->" << ray.direction << "dist: " << (float)d << "\n";
	
		if (d < res.distance)
		{
			res.distance = d;
			res.geometry = (*it).second;
 		}
 	}

	return res;
}
