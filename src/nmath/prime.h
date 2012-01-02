/*

    This file is part of the nemesis math library.

    prime.h
    Prime numbers related functions

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

#ifndef LIBNMATH_PRIME_H_INCLUDED
#define LIBNMATH_PRIME_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

static inline int isPrime(unsigned long i);
static inline unsigned long getNextPrime(unsigned long i);
static inline unsigned long getPrevPrime(unsigned long i);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#include "prime.inl"

#endif /* LIBNMATH_PRIME_H_INCLUDED */
