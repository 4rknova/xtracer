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

#include "sdl.hpp"  

#include <stdio.h>
#include <SDL/SDL.h>

DrvSDL::DrvSDL( Framebuffer &fb, XT_DRV_INTV intv, XT_DRV_SYN sync):
	Driver(fb, intv, sync), m_p_screen(NULL)
{}

DrvSDL::~DrvSDL()
{}

xt_status_t DrvSDL::init()
{
	printf("Creating the sdl window..\n");

	if( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )
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
	SDL_Quit(); /* This will release m_p_screen as well */
	return 0;
}

xt_status_t DrvSDL::update(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
	/* Copy the pixel window here */
	return XT_STATUS_OK;
}

xt_status_t DrvSDL::update()
{
	/* Update the whole buffer */
	return update(0, 0, m_p_fb->width(), m_p_fb->height());
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
