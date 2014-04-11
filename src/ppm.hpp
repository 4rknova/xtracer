/*

	This file is part of libnimg.

	ppm.cpp
	PPM

	Copyright (C) 2012
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

#ifndef NIMG_PPM_HPP_INCLUDED
#define NIMG_PPM_HPP_INCLUDED


#include "pixmap.hpp"

namespace NImg {
	namespace IO {
		namespace Import {
//  RESTRICTIONS:
//  Only images with a pixel component maximum value of
//  255 are supported. This is the most common format.
//
// RETURN CODES:
// 0. Everything went well.
// 1. Filename is NULL.
// 2. File I/O error.
// 3. Invalid format.
// 4. Corrupted file.
// 5. Failed to initialize the Image.
int ppm_raw(const char *filename, Pixmap &map);

		} /* namespace Import */

		namespace Export {
//  RESTRICTIONS:
//  Only images with a pixel component maximum value of
//  255 are supported. This is the most common format.
//
// RETURN CODES:
// 0. Everything went well.
// 1. Filename is NULL.
// 2. File I/O error.
// 3. Nothing to export. Width or height is 0.
int ppm_raw(const char *filename, const Pixmap &map);

		} /* namespace Export */
	} /* namespace IO */
} /* namespace NImg */

#endif /* NIMG_PPM_HPP_INCLUDED */
