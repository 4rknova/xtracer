/*

    This file is part of the libnmath.

    matrix.inl
    Matrix inline functions

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

#ifndef NMATH_MATRIX_INL_INCLUDED
#define NMATH_MATRIX_INL_INCLUDED

#ifndef NMATH_MATRIX_H_INCLUDED
    #error "matrix.h must be included before matrix.inl"
#endif /* NMATH_MATRIX_H_INCLUDED */

#ifdef __cplusplus
	#include <cstring>
#else
	#include <string.h>
#endif  /* __cplusplus */

namespace NMath {

#ifdef __cplusplus

inline scalar_t *Matrix3x3f::operator [](int index)
{
    return data[index < 9 ? index : 8];
}

inline const scalar_t *Matrix3x3f::operator [](int index) const
{
    return data[index < 9 ? index : 8];
}

inline void Matrix3x3f::reset_identity()
{
    memcpy(data, identity.data, 9 * sizeof(scalar_t));
}

inline scalar_t *Matrix4x4f::operator [](int index) 
{
    return data[index < 16 ? index : 15];
}

inline const scalar_t *Matrix4x4f::operator [](int index) const
{
    return data[index < 16 ? index : 15];
}

inline void Matrix4x4f::reset_identity()
{
    memcpy(data, identity.data, 16 * sizeof(scalar_t));
}

#endif /* extern "C" */

} /* namespace NMath */

#endif /* NMATH_MATRIX_INL_INCLUDED */
