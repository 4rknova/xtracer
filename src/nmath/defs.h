/*

	This file is part of libnimg.

	defs.h
	C constants

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

#ifndef LIBNIMG_DEFS_H_INCLUDED
#define LIBNIMG_DEFS_H_INCLUDED

#if (__STDC_VERSION__ < 199999)
    #if defined(__GNUC__) || defined(_MSC_VER)
        #define inline __inline
    #else
        /* Inline functions not supported. Performance will suffer */
        #define inline
	#endif
#endif /* __STDC_VERSION__ */

#endif /* LIBNIMG_DEFS_H_INCLUDED */
