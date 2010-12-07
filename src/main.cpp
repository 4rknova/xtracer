/*

    This file is part of xtracer.

    main.cpp
    Xtracer entry point

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
#include <string.h>
#include "err.h"

/* Default values for the screen buffer dimensions */
#define DEFAULT_SCREEN_WIDTH	640
#define DEFAULT_SCREEN_HEIGHT	480

/*
	Environmental options

	The command line accepts the following options

		OPTION				DESCRIPTION
		-interactive		The program will start in an sdl window and render the frames in that window. If this option is not given, the rendered frames will be dumbed to image files
		-depth	%i			The maximum recursion depth given as an integer %i
		-buffer %ix%i		The screen buffer dimmensions given as an integer pair formatted as %ix%i
*/
struct envsetup_t
{
	envsetup_t() 
		: is_interactive(false), width(DEFAULT_SCREEN_WIDTH), height(DEFAULT_SCREEN_HEIGHT), depth(0)
	{}

	bool is_interactive;	/* Defines whether the rendered images will be displayed in an interactive window or dumped in image files */

	int width;				/* The screen buffer width */
	int height;				/* The screen buffer height */

	int depth;				/* The maximum recursion depth. 0 denotes unlimited depth */
} envsetup;

/*
	MAIN
*/
int main(int argc, char **argv)
{
	int i=0;

	/* Parse the cli arguments */
    for (i=1; i<argc; i++)
	{
        if (strcmp(argv[i], "-interactive") == 0)
		{
            envsetup.is_interactive=true;
        }
		else if (strcmp(argv[i], "-depth") == 0)
		{
			i++;

			int depth = 0;
			if (!argv[i] || sscanf(argv[i], "%d", &depth) < 1) 
            {
                fprintf(stderr, "\nInvalid -depth value. Should be %%i");
                return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
            }
			envsetup.depth = depth;
		}
        else if (strcmp(argv[i], "-buffer") == 0)
		{
            i++;

			int width = 0;
			int height = 0;
            if (!argv[i] || sscanf(argv[i], "%dx%d", &width, &height) < 2)
			{
                fprintf(stderr, "\nInvalid -size value. Should be %%ix%%i");
                return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
            }
			envsetup.width = width;
			envsetup.height = height;
        }
        else
		{
			/*
				Scene file is loaded. Any orphan argument is treated as the expected scene file path.
			*/
        }
    }

	return XTRACER_STATUS_OK;
}
