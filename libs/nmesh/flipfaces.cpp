/*

	This file is part of libnmesh.

	flipfaces.hpp
	Face flipping mutator

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
#include "flipfaces.hpp"

namespace NMesh {
	namespace Mutator {

int flip_faces(Mesh &mesh)
{
	Buffer<vertex_t> &p_meshv = mesh.vertices();
	Buffer<index_t>  &p_meshi = mesh.indices();

	// Invert the faces.
	for (unsigned int i = 0; i < p_meshi.count(); i+=3) {
		int temp = p_meshi[i];
		p_meshi[i] = p_meshi[i+2];
		p_meshi[i+2] = temp;
	}

	// Correct the normals.
	for (unsigned int i = 0; i < p_meshv.count(); i++) {
		p_meshv[i].nx *= -1.0f;
		p_meshv[i].ny *= -1.0f;
		p_meshv[i].nz *= -1.0f;
	}

	return 0;
}

		} /* namespace Mutator */
} /* namespace NMesh */
