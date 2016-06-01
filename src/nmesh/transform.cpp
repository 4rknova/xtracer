/*

	This file is part of libnmesh.

	transform.hpp
	Transformation mutators

	Copyright (C) 2008, 2010 - 2012
	Papadopoulos Nikolaos

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 3 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General
	Public License along with this program; if not, write to the
	Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA 02110-1301 USA

*/

#include "buffer.hpp"
#include "transform.hpp"

namespace NMesh {
	namespace Mutator {

int translate(NMesh::Mesh &mesh, NMath::scalar_t x, NMath::scalar_t y, NMath::scalar_t z)
{
	Buffer<vertex_t> &p_meshv = mesh.vertices();

	// Invert the faces.
	for (unsigned int i = 0; i < p_meshv.count(); i++) {
		p_meshv[i].px += x;
		p_meshv[i].py += y;
		p_meshv[i].pz += z;
	}

	return 0;
}

int scale(NMesh::Mesh &mesh, NMath::scalar_t x, NMath::scalar_t y, NMath::scalar_t z)
{
	Buffer<vertex_t> &p_meshv = mesh.vertices();

	// Invert the faces.
	for (unsigned int i = 0; i < p_meshv.count(); i++) {
		p_meshv[i].px *= x;
		p_meshv[i].py *= y;
		p_meshv[i].pz *= z;
	}

	return 0;
}

		} /* namespace Mutator */
} /* namespace NMesh */
