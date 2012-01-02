/*

	This file is part of xtracer.

	drvsdl.hpp
	SDL driver

	Copyright (C) 2010, 2011
	Papadopoulos Nikolaos

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 3 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General
	Public License along with this program; if not, write to the
	Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA 02110-1301 USA

*/

#ifndef XTRACER_DRV_SDL_HPP_INCLUDED
#define XTRACER_DRV_SDL_HPP_INCLUDED

#ifdef ENABLE_SDL

#ifdef _MSC_VER 
	// As usual we have to treat our little "special" platform separately.
	// I have no intention to litter my code more than I already have to keep visual studio satisfied.
	// Windows just fails when it comes to having a solid infrastructure for software development.
	// If this fails to compile, you will have to set the SDL library paths manually.
	#include <SDL.h>
#else
	#include <SDL/SDL.h>
#endif /* _MSC_VER */

#include "drv.hpp"
#include "console.hpp"

class DrvSDL: public Driver
{
	public:
		DrvSDL(Framebuffer &fb, Console &con);
		~DrvSDL();

		unsigned int init();
		unsigned int deinit();
		unsigned int update(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1);
		unsigned int flip();

		unsigned int hint();

		bool is_realtime();

	private:
		SDL_Surface* m_screen;
};

#endif /* ENABLE_SDL */

#endif /* XTRACER_DRV_SDL_HPP_INCLUDED */
