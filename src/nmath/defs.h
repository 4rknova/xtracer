/*

    This file is part of the nemesis math library.

    defs.h
    Defines math related constants

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

#ifndef LIBNMATH_DEFS_H_INCLUDED
#define LIBNMATH_DEFS_H_INCLUDED

#if (__STDC_VERSION__ < 199999)
    #if defined(__GNUC__) || defined(_MSC_VER)
        #define inline __inline
    #else
        /* Inline functions not supported. Performance will suffer */
        #define inline
	#endif
#endif /* __STDC_VERSION__ */

/*
    PI
*/
#define PI_DOUBLE		6.283185307179586232
#define PI				3.14159265358979323846
#define PI_HALF			1.57079632679489661923
#define PI_QUARTER		0.78539816339744830962

#ifndef M_PI
    #define M_PI	    PI
#endif /* M_PI */

#ifndef EPSILON
	#define EPSILON   		1E-8
	#define FLOAT_EPSILON	EPSILON
	#define DOUBLE_EPSILON	1E-15
#endif /* EPSILON */

/*
     EULER_E
*/
#define EULER_E         2.7182818284590452354

#ifndef M_E
    #define M_E			EULER_E
#endif /* M_E */

#define LN2		0.69314718055994530942
#define LN10	2.30258509299404568402

#endif /* LIBNMATH_DEFS_H_INCLUDED */
