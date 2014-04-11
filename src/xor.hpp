/*

	This file is part of libnimg.

	xor.hpp
	XOR Texture

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

#ifndef NIMG_XOR_HPP_INCLUDED
#define NIMG_XOR_HPP_INCLUDED


#include "pixmap.hpp"

namespace NImg {
	namespace Generate {

// RETURN CODES:
// 0. Everything went well.
// 1. Width or Height is 0.
// 2. Failed to initialize the Image.
int xortex(Pixmap &map, const unsigned int width, const unsigned int height);

	} /* namespace Generator */
} /* namespace NImg */

#endif /* NIMG_XOR_HPP_INCLUDED */
