/*

    This file is part of the libnmath.

    types.h
    Declares global data types

    Copyright (C) 2008, 2010, 2011
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

#ifndef NMATH_TYPES_H_INCLUDED
#define NMATH_TYPES_H_INCLUDED

#include "precision.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Vectors */
struct vec2_t { scalar_t x, y; };
struct vec3_t { scalar_t x, y, z; };
struct vec4_t { scalar_t x, y, z, w; };

typedef struct vec2_t vec2_t;
typedef struct vec3_t vec3_t;
typedef struct vec4_t vec4_t;

/* Matrices */
typedef scalar_t mat3x3_t[3][3];
typedef scalar_t mat4x4_t[4][4];

/* Bounding boxes */
struct aabb2_t { vec2_t min, max; };
struct aabb3_t { vec3_t min, max; };

typedef struct aabb2_t aabb2_t;
typedef struct aabb3_t aabb3_t;

#ifdef __cplusplus
}   /* extern "C" */

/* C++ equivalents - Forward declarations */
class Vector2f;
class Vector3f;
class Vector4f;

class Matrix3x3f;
class Matrix4x4f;

class BoundingBox2;
class BoundingBox3;

#endif	/* __cplusplus */

} /* namespace NMath */

#endif /* NMATH_TYPES_H_INCLUDED */
