/*

    This file is part of xtracer.

    matlambert.cpp
    Matlambert class

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

#include "matlambert.hpp"

MatLambert::MatLambert(const Vector3 &ints)
	: intensity(ints)
{}

MatLambert::MatLambert()
	: intensity(Vector3(0, 0, 0))
{}

Vector3 MatLambert::shade()
{
	return intensity;
}
