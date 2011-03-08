/*

	This file is part of xtracer.

	sdl.c
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

#include "sdl.h"

#ifdef __cplusplus
	extern "C" {
#endif  /* __cplusplus */

SDL_Surface* sdl_screen = NULL;

int out_drv_sdl_init(unsigned int width, unsigned int height, unsigned int bpp)
{
	if( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )
		return 1;
	
	/* Set up the screen */
	sdl_screen = SDL_SetVideoMode( width, height, bpp, SDL_SWSURFACE );
	if(!sdl_screen)
		return 1;
	/* Set the window caption */
	SDL_WM_SetCaption( "Xtracer", NULL );
}

int out_drv_sdl_deinit()
{
	/* Deinit */
	SDL_Quit(); /* This will release sdl_screen as well */
}

int out_drv_sdl_set(unsigned int x, unsigned int y, unsigned int color)
{
	/* Update the screen */
	if( SDL_Flip( SCREEN ) == -1 )
	{
		return 1;
	}
}

#ifdef __cplusplus
	}   /* extern "C" */
#endif /* __cplusplus */
