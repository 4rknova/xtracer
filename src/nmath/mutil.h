/*

    This file is part of the nemesis math library.

    mutil.h
    Declares some math utility functions

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

#ifndef LIBNMATH_MUTIL_H_INCLUDED
#define LIBNMATH_MUTIL_H_INCLUDED

#include "precision.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Greater Common Divisor */
static inline int gcd(int a, int b); /* Euclid's method */

/* Lowest Common Multiple */
static inline int lcm(int a, int b);

/* Check if an integer is a power of 2 */
static inline int is_power_of_2(int v);

#ifdef __cplusplus
}   /* extern "C" */
#endif /* __cplusplus */

#include "mutil.inl"

#endif /* LIBNMATH_MUTIL_H_INCLUDED */
