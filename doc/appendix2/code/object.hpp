/*

    This file is part of xtracer.

    object.hpp
    Object class

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

#ifndef XTRACER_OBJECT_HPP_INCLUDED
#define XTRACER_OBJECT_HPP_INCLUDED

#include <string>

class Object 
{
	public:
		Object();
		~Object();

		// The geometry's name
		std::string geometry;

		// The material's name
		std::string material;
};

#endif /* XTRACER_OBJECT_HPP_INCLUDED */