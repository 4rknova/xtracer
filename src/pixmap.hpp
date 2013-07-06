/*

	This file is part of libnimg.

	pixmap.hpp
	Pixmap

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

#ifndef NIMG_PIXMAP_HPP_INCLUDED
#define NIMG_PIXMAP_HPP_INCLUDED


#include "buffer.hpp"
#include "color.hpp"

namespace NImg {

// For visual studio:
// Disable "<type> needs to have dll-interface to be used by clients"
// This warning refers to STL member variables which are private and
// therefore can be safely ignored.
#pragma warning(push)
#pragma warning(disable : 4251) // unreferenced formal parameter

class Pixmap
{
	public:
		Pixmap();
		Pixmap(const Pixmap &img);
		Pixmap &operator =(const Pixmap &img);
		~Pixmap();

		// NOTES:
		// If the user sets one or both of the dimensions as 0
		// it is equivalent to resetting the buffer as total size is 0.
		// RETURN CODES:
		//	0. Everything went well.
		//  1. Failed to initialize pixels.
		int init(const unsigned int w, const unsigned int h);

		unsigned int width() const;
		unsigned int height() const;

		const ColorRGBAf &pixel_ro(const unsigned int x, const unsigned int y) const;
		ColorRGBAf &pixel(const unsigned int x, const unsigned int y);

	private:
		unsigned int m_width, m_height;
		mutable Buffer<ColorRGBAf> m_pixels;
};

#pragma warning (pop)

} /* namespace NImg */

#endif /* NIMG_TEXTURE_HPP_INCLUDED */
