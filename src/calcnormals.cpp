/*

	This file is part of libnmesh.

	calcnormals.hpp
	Normal generation mutator

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

#include "precision.h"
#include "vector.h"
#include "buffer.hpp"
#include "calcnormals.hpp"

namespace NMesh {
	namespace Mutator {

int calc_normals(Mesh &mesh)
{
	Buffer<vertex_t> &p_meshv = mesh.vertices();
	Buffer<index_t>  &p_meshi = mesh.indices();

	// Zero out all the normals.
	for (unsigned int i = 0; i < p_meshv.count(); i++) {
		p_meshv[i].nx = 0.0f;
		p_meshv[i].ny = 0.0f;
		p_meshv[i].nz = 0.0f;
	}

	for (unsigned int i = 0; i < p_meshi.count(); i+=3) {
		// Calculate 2 edge vectors (counter clock-wise).
		NMath::Vector3f v0(
			p_meshv[p_meshi[i  ]].px,
			p_meshv[p_meshi[i  ]].py,
			p_meshv[p_meshi[i  ]].pz);

		NMath::Vector3f v1(
			p_meshv[p_meshi[i+1]].px,
			p_meshv[p_meshi[i+1]].py,
			p_meshv[p_meshi[i+1]].pz);

		NMath::Vector3f v2(
			p_meshv[p_meshi[i+2]].px,
			p_meshv[p_meshi[i+2]].py,
			p_meshv[p_meshi[i+2]].pz);

		NMath::Vector3f a = v2 - v0;
		NMath::Vector3f b = v1 - v0;

		// Calculate the cross product.
		NMath::Vector3f fnormal = cross(b, a).normalized();

		// Modify the per-vertex normal
		p_meshv[p_meshi[i  ]].nx += (NMath::scalar_t)fnormal.x / 3.f;
		p_meshv[p_meshi[i  ]].ny += (NMath::scalar_t)fnormal.y / 3.f;
		p_meshv[p_meshi[i  ]].nz += (NMath::scalar_t)fnormal.z / 3.f;

		NMath::Vector3f n0 = NMath::Vector3f(p_meshv[p_meshi[i  ]].nx,
							   p_meshv[p_meshi[i  ]].ny,
							   p_meshv[p_meshi[i  ]].nz).normalized();

		p_meshv[p_meshi[i  ]].nx = (NMath::scalar_t)n0.x;
		p_meshv[p_meshi[i  ]].ny = (NMath::scalar_t)n0.y;
		p_meshv[p_meshi[i  ]].nz = (NMath::scalar_t)n0.z;

		p_meshv[p_meshi[i+1]].nx += (NMath::scalar_t)fnormal.x / 3.f;
		p_meshv[p_meshi[i+1]].ny += (NMath::scalar_t)fnormal.y / 3.f;
		p_meshv[p_meshi[i+1]].nz += (NMath::scalar_t)fnormal.z / 3.f;

		NMath::Vector3f n1 = NMath::Vector3f(p_meshv[p_meshi[i+1]].nx,
							   p_meshv[p_meshi[i+1]].ny,
							   p_meshv[p_meshi[i+1]].nz).normalized();

		p_meshv[p_meshi[i+1]].nx = (NMath::scalar_t)n1.x;
		p_meshv[p_meshi[i+1]].ny = (NMath::scalar_t)n1.y;
		p_meshv[p_meshi[i+1]].nz = (NMath::scalar_t)n1.z;

		p_meshv[p_meshi[i+2]].nx += (NMath::scalar_t)fnormal.x / 3.f;
		p_meshv[p_meshi[i+2]].ny += (NMath::scalar_t)fnormal.y / 3.f;
		p_meshv[p_meshi[i+2]].nz += (NMath::scalar_t)fnormal.z / 3.f;

		NMath::Vector3f n2 = NMath::Vector3f(p_meshv[p_meshi[i+2]].nx,
							   p_meshv[p_meshi[i+2]].ny,
							   p_meshv[p_meshi[i+2]].nz).normalized();

		p_meshv[p_meshi[i+2]].nx = (NMath::scalar_t)n2.x;
		p_meshv[p_meshi[i+2]].ny = (NMath::scalar_t)n2.y;
		p_meshv[p_meshi[i+2]].nz = (NMath::scalar_t)n2.z;
	}
/*
	for (unsigned int i = 0; i < p_meshv.count(); ++i) {
		NMath::Vector3f v = NMath::Vector3f(p_meshv[i].nx,
						      p_meshv[i].ny,
							  p_meshv[i].nz).normalized();

		p_meshv[i].nx = (NMath::scalar_t)v.x;
		p_meshv[i].ny = (NMath::scalar_t)v.y;
		p_meshv[i].nz = (NMath::scalar_t)v.z;
	}
*/
	return 0;
}

		} /* namespace Mutator */
} /* namespace NMesh */
