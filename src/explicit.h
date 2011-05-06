/*

    This file is part of the NemesisPlatform library.

    explicit.h
    Explicit integral types

    Copyright (C) 2008, 2010
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

#ifndef LIBNPLATFORM_EXPLICIT_H_INCLUDED
#define LIBNPLATFORM_EXPLICIT_H_INCLUDED

#include "compiler.h"

#if (COMPILER == COMPILER_MSC)
    typedef unsigned __int8  uint8_t;
    typedef unsigned __int16 uint16_t;
    typedef unsigned __int32 uint32_t;
    typedef unsigned __int64 uint64_t;
#else
    #include <stdint.h>
#endif /* COMPILER */

#endif /* LIBNPLATFORM_EXPLICIT_H_INCLUDED */