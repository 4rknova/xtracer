/*

	This file is part of xtracer.

	sdl.cpp
	SDL driver

	Copyright (C) 2010, 2011
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

#ifdef ENABLE_SDL

#include "drvsdl.hpp"
#include "pixel.h"

DrvSDL::DrvSDL(Framebuffer &fb):
	Driver(fb), m_screen(NULL)
{}

DrvSDL::~DrvSDL()
{}

unsigned int DrvSDL::init()
{
	if( SDL_Init( SDL_INIT_VIDEO ) != 0 )
		return 1;
	
	// setup the window
	m_screen = SDL_SetVideoMode( m_fb->width(), m_fb->height(), 32, SDL_SWSURFACE);

	if(!m_screen)
		return 1;

	// setup the window caption
	std::string caption = "Xtracer v";
	caption.append(XT_VERSION);
	caption.append(" - ");
	caption.append(m_fb->tag());

	SDL_WM_SetCaption(caption.c_str(), NULL);
	return 0;
}

#include <iostream>

unsigned int DrvSDL::deinit()
{
	if(!m_screen)
		return 1;

	// block the window from closing
	// until escape is pressed
	SDL_Event event;
	int done = 0;

	std::cout << "Press ESC on the sdl window to exit.\n";

	while(SDL_PollEvent(&event) || !done)
	{
		switch (event.type)
		{
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					done = 1;
				break;
		}
	}

	SDL_Quit(); // This will release m_window as well
	return 0;
}

unsigned int DrvSDL::update(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
	// check if the surface must be locked
	if(SDL_MUSTLOCK(m_screen))
	{
		SDL_LockSurface(m_screen);
	}

	// check for "out of bounds" and inverse mapping errors
	if ((x0 > m_fb->width()) 
		|| (x1 > m_fb->width()) 
		|| (y0 > m_fb->height()) 
		|| (y1 > m_fb->height())
		|| (x0 > x1)
		|| (y0 > y1))
		return 1;

	// convert the pixels to 32 bit
	pixel32_t *pixels = (pixel32_t *)m_screen->pixels;

	// set the pixels
	for(unsigned int y = y0; y < y1; y++)
	{
		for(unsigned int x = x0; x < x1; x++)
		{
			Vector3 pixel = *(m_fb->pixel(x, y));

			pixel32_t p = rgba_f_to_pixel32(pixel.x, pixel.y, pixel.z, 1.0);
			
			pixels[(y * m_screen->w) + x] = 
				SDL_MapRGB( 
						m_screen->format, 
						get_pixel32_r(p), 
						get_pixel32_g(p), 
						get_pixel32_b(p));
		}
	}

	// check if the surface must be unlocked
	if(SDL_MUSTLOCK(m_screen))
	{
		SDL_UnlockSurface(m_screen);
	}

	// flip the buffer
	flip();

	// empty the event queue
	SDL_Event event;
	while(SDL_PollEvent(&event));

	return 0;
}

unsigned int DrvSDL::flip()
{
	// update the screen
	if (SDL_Flip(m_screen) == -1)
	{
		return 1;
	}
	return 0;
}

#endif /* ENABLE_SDL */
