/*

    This file is part of the Nemesis common utils.

    compiler.h
    Compiler detection

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

#ifndef COMMON_COMPILER_H_INCLUDED
#define COMMON_COMPILER_H_INCLUDED

/* COMPILER IDs */
#define COMPILER_UNKNOWN 0x0000
#define COMPILER_GNUGCC  0x0011
#define COMPILER_MINGW32 0x0012
#define COMPILER_MSC     0x0020
#define COMPILER_DJGPP   0x0030

/*
    COMPILER DETECTION
*/
#if defined(__MINGW32__)
    #define COMPILER               COMPILER_MINGW32
    #define COMPILER_STRING        "MinGW"
    #define COMPILER_VER_MAJOR     __MINGW32_MAJOR_VERSION
    #define COMPILER_VER_MINOR     __MINGW32_MINOR_VERSION
    #define COMPILER_VER_REVIS     0

#elif defined(__GNUC__)
    #define COMPILER               COMPILER_GNUC
    #define COMPILER_STRING        "GNUCC"
    #define COMPILER_VER_MAJOR     __GNUC__
    #define COMPILER_VER_MINOR     __GNUC_MINOR__
    #define COMPILER_VER_REVIS     __GNUC_PATCHLEVEL__

#elif defined(_MSC_VER)
    #define COMPILER               COMPILER_MSC
    #define COMPILER_STRING        "Microsoft Visual C++"
    #define COMPILER_VER_MAJOR     _MSC_VER
    #define COMPILER_VER_MINOR     0
    #define COMPILER_VER_REVIS     0

#elif defined(__DJGPP__)
    #define COMPILER               COMPILER_DJGPP
    #define COMPILER_STRING        "DJGPP"
    #define COMPILER_VER_MAJOR     __DJGPP__
    #define COMPILER_VER_MINOR     __DJGPP_MINOR__
    #define COMPILER_VER_REVIS     0


#else
    #define COMPILER               COMPILER_UNKNOWN
    #define COMPILER_STRING        "Unknown compiler"
    #define COMPILER_VER_MAJOR     0
    #define COMPILER_VER_MINOR     0
    #define COMPILER_VER_REVIS     0

#endif

#endif /* COMMON_COMPILER_H_INCLUDED */
