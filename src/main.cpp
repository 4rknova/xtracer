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
/* Default values for the raytracer environment */
#define DEFAULT_RECURSION_DEPTH	5


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
	int iIsMaster = 0;	/* flag to setup the program to run as a master node */
	int iIsSlave = 0;	/* flag to setup the program to run as a slave node */

	/* Parse the cli arguments */
    for (int i=1; i<argc; i++)
	{
		/* Output version information and exit */
		if((!strcmp(argv[i], "-version")) || (!strcmp(argv[i], "-v")))
		{
			printf("Xtracer v0.0\nby Papadopoulos Nikos 2010\nusage: %s [option]... scene_file\n", argv[0]);
			return XTRACER_STATUS_OK;
		}
		/* Start as a master node */
		else if (!strcmp(argv[i], "-master"))
		{
			if (iIsSlave)
			{
				fprintf(stderr, "Cannot use -master with -slave.\n");
				return XTRACER_STATUS_INVALID_CLI_ARGCOMBN;
			}
			iIsMaster = 1;
		}
		/* Start as a slave node */
		else if (!strcmp(argv[i], "-slave"))
		{
			if (iIsMaster)
			{
				fprintf(stderr, "Cannot use -slave with -master.\n");
				return XTRACER_STATUS_INVALID_CLI_ARGCOMBN;
			}
			iIsSlave = 1;
		}
		/* Render in a sdl window */
        else if (!strcmp(argv[i], "-interactive"))
		{
            envsetup.is_interactive=true;
        }
		/* Setup the maximum recursion depth */
		else if (strcmp(argv[i], "-depth") == 0)
		{
			i++;

			int depth = 0;
			if (!argv[i] || sscanf(argv[i], "%d", &depth) < 1) 
            {
                fprintf(stderr, "Invalid -depth value. Should be %%i\n");
                return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
            }
			envsetup.depth = depth;
		}
		/* Setup the resolution */
        else if (!strcmp(argv[i], "-buffer"))
		{
            i++;

			int width = 0;
			int height = 0;
            if (!argv[i] || sscanf(argv[i], "%dx%d", &width, &height) < 2)
			{
                fprintf(stderr, "Invalid -size value. Should be %%ix%%i\n");
                return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
            }
			envsetup.width = width;
			envsetup.height = height;
        }
		/* Load the scene file */
        else
		{
			/*
				Scene file is loaded. Any orphan argument is treated as the expected scene file path.
			*/
        }
    }


	/* Start up in the specified mode */
	if (iIsSlave)
	{
		printf("Starting as a slave node\n");
	}
	else if (iIsMaster)
	{
		printf("Startimg as a mster node\n");
	}
	
	return XTRACER_STATUS_OK;
}
