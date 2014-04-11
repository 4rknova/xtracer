/*

	This file is part of libnmesh.

	vertex.h
	Vertex

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

#ifndef NMESH_VERTEX_H_INCLUDED
#define NMESH_VERTEX_H_INCLUDED

#include "precision.h"

namespace NMesh {

#ifdef __cplusplus
	extern "C" {
#endif /* __cplusplus */

struct vertex_t
{
	vertex_t()
		: px(0), py(0), pz(0),
		  nx(0), ny(0), nz(0),
		  bx(0), by(0), bz(0),
 		  tx(0), ty(0), tz(0),
		  u(0), v(0),
		  r(0), g(0), b(0), a(1)
	{}

	NMath::scalar_t px, py, pz;		/* Position */
	NMath::scalar_t nx, ny, nz;		/* Normal */
	NMath::scalar_t bx, by, bz;		/* Binormal */
	NMath::scalar_t tx, ty, tz;		/* Tangent */
	NMath::scalar_t u, v;				/* Texture coordinates */
	NMath::scalar_t r, g, b, a;		/* Color */
};

typedef struct vertex_t vertex_t;

typedef unsigned int index_t;

#ifdef __cplusplus
	} /* extern */
#endif /* __cplusplus */

} /* namespace NMesh */

#endif /* NMESH_VERTEX_H_INCLUDED */
