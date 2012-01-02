/*

    This file is part of the nemesis math library.

    interpolation.h
    Interpolation

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

/*
    To do
    -----
    - TCB spline (Kochanek-Bartels)
    - Beta splines
    - Uniform nonrational splines
*/

#ifndef LIBNMATH_INTERPOLATION_H_INCLUDED
#define LIBNMATH_INTERPOLATION_H_INCLUDED

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* Step */
static inline scalar_t interp_step(scalar_t a, scalar_t b, scalar_t p);

/* Linear */
static inline scalar_t interp_linear(scalar_t a, scalar_t b, scalar_t p);

/* Trigonometric */
static inline scalar_t interp_cosine(scalar_t a, scalar_t b, scalar_t p);

/* Polynomial */
static inline scalar_t interp_acceleration(scalar_t a, scalar_t b, scalar_t p);
static inline scalar_t interp_deceleration(scalar_t a, scalar_t b, scalar_t p);

static inline scalar_t interp_cubic(scalar_t a, scalar_t b, scalar_t c, scalar_t d, scalar_t p);

/* Splines */
static inline scalar_t interp_hermite(scalar_t t1, scalar_t a, scalar_t b, scalar_t t2, scalar_t p);
static inline scalar_t interp_cardinal(scalar_t a, scalar_t b, scalar_t c, scalar_t d, scalar_t t, scalar_t p);
static inline scalar_t interp_catmullrom(scalar_t a, scalar_t b, scalar_t c, scalar_t d, scalar_t p);

/* Bezier */
static inline scalar_t interp_bezier_quadratic(scalar_t a, scalar_t b, scalar_t c, scalar_t p);        /* DeCasteljau */
static inline scalar_t interp_bezier_cubic(scalar_t a, scalar_t b, scalar_t c, scalar_t d, scalar_t p);  /* DeCasteljau */

#ifdef __cplusplus
}   /* extern "C" */
#endif

#include "interpolation.inl"

#endif /* LIBNMATH_INTERPOLATION_H_INCLUDED */
