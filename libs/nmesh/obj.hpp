/*

	This file is part of libnmesh.

	obj.hpp
	OBJ

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

#ifndef NMESH_OBJ_HPP_INCLUDED
#define NMESH_OBJ_HPP_INCLUDED


#include "nmesh.hpp"

namespace NMesh {
		namespace IO {
			namespace Import {

// RESTRICTIONS:
// When exporting the obj file you MUST
// select to triangulate the mesh and export
// uv coordinates and normals. Incompatible
// formats will be treated as having erroneous
// syntax.
//
// RETURN CODES:
// 0. Everything went well.
// 1. filename equals NULL.
// 3. Failed to open file.
// 4. Invalid format.
// 5. Failed to initialize the Mesh.
int obj(const char *file, NMesh::Mesh &mesh);

			} /* namespace Import */

			namespace Export {
			} /* namespace Export */

		} /* namespace IO */
} /* namespace NMesh */

#endif /* NMESH_OBJ_HPP_INCLUDED */
