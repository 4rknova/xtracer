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

#include <stdio.h>

#include "sdl.hpp"
#include "pixel.h"

DrvSDL::DrvSDL( Framebuffer &fb):
	Driver(fb), m_p_screen(NULL)
{}

DrvSDL::~DrvSDL()
{}

xt_status_t DrvSDL::init()
{
	printf("Creating the sdl window..\n");

	if( SDL_Init( SDL_INIT_VIDEO ) != 0 )
		return 1;
	
	/* Set up the screen */
	printf("Setting up the window environment..\n");
	m_p_screen = SDL_SetVideoMode( m_p_fb->width(), m_p_fb->height(), m_p_fb->bpp(), SDL_SWSURFACE);
	if(!m_p_screen)
		return 1;
	/* Set the window caption */
	SDL_WM_SetCaption( "Xtracer", NULL );
	return 0;
}

xt_status_t DrvSDL::deinit()
{
	/* Deinit */
	/* Block the window from closing */
	SDL_Event event;
	int done = 0;

	while(SDL_PollEvent(&event) || !done)
	{
		switch (event.type)
		{
			case SDL_KEYDOWN:
			//	if (event.key.keysym.sym == SDLK_ESCAPE)
					done = 1;
				break;
		}
	}

	SDL_Quit(); /* This will release m_p_window as well */
	return 0;
}

xt_status_t DrvSDL::update(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
	/* Copy the pixel window here */
	/* If the surface must be locked */
	if(SDL_MUSTLOCK(m_p_screen))
	{
		/* Lock the surface */
		SDL_LockSurface(m_p_screen);
	}

	/* Check for "out of bounds" and inverse mapping errors */
	if ((x0 > m_p_fb->width()) 
		|| (x1 > m_p_fb->width()) 
		|| (y0 > m_p_fb->height()) 
		|| (y1 > m_p_fb->height())
		|| (x0 > x1)
		|| (y0 > y1))
		return XT_STATUS_FB_OUTOFBOUNDS;

	/* Convert the pixels to 32 bit */
	pixel32_t *pixels = (pixel32_t *)m_p_screen->pixels;

	/* Set the pixels */
	for(unsigned int y = y0; y < y1; y++)
	{
		for(unsigned int x = x0; x < x1; x++)
		{
			pixel32_t pixel = m_p_fb->get_pixel(x, y);
			pixels[(y * m_p_screen->w) + x] = SDL_MapRGB( m_p_screen->format, get_pixel32_r(pixel), get_pixel32_g(pixel), get_pixel32_b(pixel));

		}
	}

	/* Unlock surface */
	if(SDL_MUSTLOCK(m_p_screen))
	{
		SDL_UnlockSurface(m_p_screen);
	}

	flip();

	SDL_Event event;
	/* Empty the event queue */
	while(SDL_PollEvent(&event));

	return XT_STATUS_OK;
}

xt_status_t DrvSDL::flip()
{
	/* Update the screen */
	if( SDL_Flip(m_p_screen) == -1 )
	{
		return XT_STATUS_OUTDRV_FLIP_FAIL;
	}
	return XT_STATUS_OK;
}
