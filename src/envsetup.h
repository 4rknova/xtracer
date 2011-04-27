/*

    This file is part of the common utils.

    envsetup.h
    Compile time environment detection and setup

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

/*
    Here we define some macros to setup
    the host and target system at compile time

    MACRO           DESCRIPTION
    -------------------------------------------
    OS_HOST_BASE    Host's operating system family
    OS_HOST_GENR    Host's operating system specific genre
    OS_HOST_ARCH    Host's operating system architecture

    OS_TARGET_BASE  Target's operating system family
    OS_TARGET_GENR  Target's operating system specific genre
    OS_TARGET_ARCH  Target's operating system architecture

    To enable cross compiling use the following macros

    #define OS_CROSSCOMPILE
    #define OS_TARGET_BASE OS_BASE_XXX
    #define OS_TARGET_GENR OS_GENR_XXX
    #define OS_TARGET_ARCH OS_ARCH_XXX

    Alternatively for gcc just use
    -D'OS_CROSSCOMPILE' 'OS_TARGET_BASE=OS_BASE_XXX' 'OS_TARGET_GENR=OS_GENR_XXX' 'OS_TARGET_ARCH OS_ARCH_XXX'
*/

#ifndef COMMON_OS_H_INCLUDED
#define COMMON_OS_H_INCLUDED

/* OS GENRE */
#define OS_GENR_UNKNOWN 0x0000
#define OS_GENR_UNIX    0x0011
#define OS_GENR_OS2     0x0021
#define OS_GENR_MSDOS   0x0022
#define OS_GENR_MSWIN   0x0023
#define OS_GENR_BEOS    0x0041

/* OS BASE */
#define OS_BASE_UNKNOWN 0x0000
#define OS_BASE_LINUX   0x0011
#define OS_BASE_HAIKU   0x0012
#define OS_BASE_BSD     0x0013
#define OS_BASE_MACOS   0x0014
#define OS_BASE_OS2     0x0021
#define OS_BASE_MSDOS   0x0022
#define OS_BASE_MSWIN   0x0023
#define OS_BASE_MSWINCE 0x0024
#define OS_BASE_SONYPSP 0x0050

/* OS ARCHITECTURES */
#define OS_ARCH_UNKNOWN 0x0000
#define OS_ARCH_8BIT    0x0008
#define OS_ARCH_16BIT   0x0010
#define OS_ARCH_32BIT   0x0020
#define OS_ARCH_64BIT   0x0040

/*
    COMPILE TIME HOST DETECTION
*/
#if defined(_WIN32_WCE)
    #define OS_HOST_GENR OS_GENR_MSWIN
    #define OS_HOST_BASE OS_BASE_MSWINCE
    #define OS_HOST_ARCH OS_ARCH_32BIT

#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #define OS_HOST_GENR OS_GENR_MSWIN
    #define OS_HOST_BASE OS_BASE_MSWIN
    #define OS_HOST_ARCH OS_ARCH_32BIT

#elif defined(_WIN64)
    #define OS_HOST_GENR OS_GENR_MSWIN
    #define OS_HOST_BASE OS_BASE_MSWIN
    #define OS_HOST_ARCH OS_ARCH_64BIT

#elif defined(__WINDOWS__) || defined(__TOS_WIN__)
    #define OS_HOST_GENR OS_GENR_MSWIN
    #define OS_HOST_BASE OS_BASE_MSWIN
    #define OS_HOST_ARCH OS_ARCH_UNKNOWN

#elif defined(MSDOS) || defined(_MSDOS) || defined(__MSDOS__) || defined(__DOS__)
    #define OS_HOST_GENR OS_GENR_MSDOS
    #define OS_HOST_BASE OS_BASE_MSDOS
    #define OS_HOST_ARCH OS_ARCH_16

#elif defined(__linux__) || defined(linux) || defined(__linux)
    #define OS_HOST_GENR OS_GENR_UNIX
    #define OS_HOST_BASE OS_BASE_LINUX
    #define OS_HOST_ARCH OS_ARCH_UNKNOWN

#elif defined(OS2) || defined(_OS2) || defined(__OS2__) || defined(__TOS_OS2__)
    #define OS_HOST_GENR OS_GENR_OS2
    #define OS_HOST_BASE OS_BASE_OS2
    #define OS_HOST_ARCH OS_ARCH_UNKNOWN

#elif defined(_PSPSDK)
    #define OS_HOST_GENR OS_GENR_UNIX
    #define OS_HOST_BASE OS_BASE_SONYPSP
    #define OS_HOST_ARCH OS_ARCH_32BIT

#else
    #warning Compile time OS detection of host failed.
    #define OS_HOST_GENR OS_GENR_UNKNOWN
    #define OS_HOST_BASE OS_BASE_UNKNOWN
    #define OS_HOST_ARCH OS_ARCH_UNKNOWN

#endif

/*
    COMPILE TIME TARGET DETECTION
*/
#ifndef OS_CROSSCOMPILE
    #undef OS_TARGET_GENR
    #undef OS_TARGET_BASE
    #undef OS_TARGET_ARCH

    #define OS_TARGET_GENR OS_HOST_GENR
    #define OS_TARGET_BASE OS_HOST_BASE
    #define OS_TARGET_ARCH OS_HOST_ARCH

#else
    #ifndef OS_TARGET_GENR
        #warning OS_TARGET_GENR is not defined for cross compiling
        #define OS_TARGET_GENR OS_GENR_UNKNOWN
    #endif

    #ifndef OS_TARGET_BASE
        #warning OS_TARGET_BASE is not defined for cross compiling
        #define OS_TARGET_BASE OS_BASE_UNKNOWN
    #endif

    #ifndef OS_TARGET_ARCH
        #warning OS_TARGET_ARCH is not defined for cross compiling
        #define OS_TARGET_ARCH OS_BASE_UNKNOWN
    #endif

#endif

#endif /* COMMON_OS_H_INCLUDED */
