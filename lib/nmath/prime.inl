/*

	This file is part of libnmath.

	prime.inl
	Prime numbers

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

#ifndef NMATH_PRIME_INL_INCLUDED
#define NMATH_PRIME_INL_INCLUDED

#ifndef NMATH_PRIME_H_INCLUDED
    #error "prime.h must be included before prime.inl"
#endif /* NMATH_PRIME_H_INCLUDED */

#include <math.h>
#include <limits.h>

namespace nmath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

static inline int isPrime(unsigned long i)
{
    if (i <= 1)
        return false;
    else if (i == 2)
        return true;
    else if (!(i % 2))
        return false;

	long j, si = sqrt(i);

	for (j = 2; j <= si; j++)
		if (!(i % j))
			return 0;

	return 1;
}

static inline unsigned long getNextPrime(unsigned long i)
{
    static const unsigned long limit = getPrevPrime(LONG_MAX);

    if (i >= limit)       // Avoid endless loop traps
        return limit;

    while(!isPrime(++i));
        return i;
}

static inline unsigned long getPrevPrime(unsigned long i)
{
    if (i <= 2)           // Avoid endless loop traps
        return 2;

    while(!isPrime(--i));
        return i;
}

#ifdef __cplusplus
}   /* extern "C" */
#endif

} /* namespace nmath */

#endif /* NMATH_PRIME_INL_INCLUDED */
