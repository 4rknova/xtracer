/*

    This file is part of xtracer.

	xtracer.cpp
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

/*
	Environmental options

	The command line accepts the following options
 
	OPTION              DESCRIPTION

	RENDERING MODES

	-interactive        The images will be rendered in an sdl window.

	-mode				Available modes:
						local	- Stand alone renderer without networking
						master	- Master node accepts incoming connections and coordinates rendering
						slave	- Slave node connects to a master node and accepts rendering tasks

	-drv				Available drivers
						sdl		- Render to SDL window
						img %s	- Render to image file
						asc		- Render to ascii ( outputs to console )

	-depth  %i          The maximum recursion depth given as an integer %i

	-buffer %ix%i       The screen buffer dimmensions given as an integer pair formatted as %ix%i

	-version, -v, -ver	Outputs version info and exits
*/

#include <cstdio>
#include <cstring>
#include <string>
#include <list>

#include "err.h"

#include <nparse/cfgparser.hpp>

/* Default values for the screen buffer dimensions */
#define XTRACER_DEFAULT_SCREEN_WIDTH	640
#define XTRACER_DEFAULT_SCREEN_HEIGHT	480
/* Default values for the raytracer environment */
#define XTRACER_DEFAULT_RECURSION_DEPTH	5


enum XTRACER_MODE_NET
{
	XTRACER_NET_LOCAL,		/* Start as stand alone, no networking */
	XTRACER_NET_MASTER,		/* Start as master node */
	XTRACER_NET_SLAVE		/* Start as slave node */
};

/* Default net mode */
#define XTRACER_DEFAULT_MODE_NET XTRACER_NET_LOCAL

enum XTRACER_MODE_RND
{
	XTRACER_RND_SDL,		/* Render to SDL window */
	XTRACER_RND_IMAGE,		/* Render to image file */
	XTRACER_RND_ASCII		/* Render to ASCII text file */
};

/* Default render mode */
#define XTRACER_DEFAULT_MODE_RND XTRACER_RND_SDL

int main(int argc, char **argv)
{
	/* setup flags */
	unsigned int xt_mode_rnd = XTRACER_DEFAULT_MODE_NET;	/* flag to setup the rendering mode */
	unsigned int xt_mode_net = XTRACER_DEFAULT_MODE_RND;	/* flag to setup the program to run as a master node */

	/* environment */
	unsigned int width = XTRACER_DEFAULT_SCREEN_WIDTH;		/* output buffer width */
	unsigned int height = XTRACER_DEFAULT_SCREEN_HEIGHT;	/* output buffer height */
	unsigned int rdepth = XTRACER_DEFAULT_RECURSION_DEPTH;	/* maximum recursion depth */

	/* list of scenes to render */
	std::list<NCFGParser *> scenes;

	/* output filepath for drivers that need it */
	std::string filepath;

	/* Parse the cli arguments */
	
	/*
		VERSION
	*/
	int i = 1;

	if((!strcmp(argv[i], "-version")) || (!strcmp(argv[i], "-v")) || (!strcmp(argv[i], "-ver")))
	{
		i++;
		printf("Xtracer v0.0\nby Papadopoulos Nikos 2010\nusage: %s [option]... scene_file\n", argv[0]);
		return XTRACER_STATUS_OK;
	}
    
	for (; i<argc; i++)
	{
		/*
			NET MODES
		*/
		if (!strcmp(argv[i], "-mode"))
		{
			i++;
			if (!argv[i])
			{
				fprintf(stderr, "no mode was provided\n");
				return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
			}

			if (!strcmp(argv[i], "local"))
			{
				xt_mode_net = XTRACER_NET_LOCAL;
			}
			else if (!strcmp(argv[i], "master"))
			{
				xt_mode_net = XTRACER_NET_MASTER;
			}
			else if (!strcmp(argv[i], "slave"))
			{
				xt_mode_net = XTRACER_NET_SLAVE;
			}
			else
			{
				fprintf(stderr, "Invalid mode. Available modes: local, master, slave\n");
				return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
			}
		}		

		/*
			RENDER MODES
		*/
        else if (!strcmp(argv[i], "-drv"))
		{
			i++;			
			if (!argv[i])
			{
				fprintf(stderr, "no driver was provided\n");
				return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
			}

			if (!strcmp(argv[i], "sdl"))
			{
				xt_mode_rnd = XTRACER_RND_SDL;
			}
			else if (!strcmp(argv[i], "img"))
			{
				i++;

				if (!argv[i])
				{
					fprintf(stderr, "no file was provided\n");
					return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
				}

				char d[1000];
				/* FIX ME: Potential buffer overflow */
				if (argv[i] && sscanf(argv[i], "%s", &d[0]) < 1)
				{
					fprintf(stderr, "Failed to read file path.\n");
					return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
				}
				filepath = d;
				xt_mode_rnd = XTRACER_RND_IMAGE;
			}
			else if (!strcmp(argv[i], "asc"))
			{	
				xt_mode_rnd = XTRACER_RND_ASCII;
			}
			else
			{
				fprintf(stderr, "Invalid driver. Available drivers: sdl, img, asc\n");
				return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
			}
		}
		
		/*
			ENVIRONMENT
		*/
		/* Maximum recursion depth */
		else if (strcmp(argv[i], "-depth") == 0)
		{
			i++;
			int d = 0;
			if (!argv[i])
			{
				fprintf(stderr, "No value was provided for %s\n", argv[i-1]);
				return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
			}

			if (sscanf(argv[i], "%d", &d) < 1) 
            {
                fprintf(stderr, "Invalid -depth value. Should be %%i\n");
                return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
            }
			rdepth = d;
		}
		/* Resolution */
        else if (!strcmp(argv[i], "-buffer"))
		{
            i++;

			if (!argv[i])
			{
				fprintf(stderr, "No value was provided for %s\n", argv[i-1]);
				return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
			}
			
			int w = 0;
			int h = 0;
            if (sscanf(argv[i], "%dx%d", &w, &h) < 2)
			{
                fprintf(stderr, "Invalid -size value. Should be %%ix%%i\n");
                return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
            }
			width = w;
			height = h;
        }
		else if (argv[i][0] == '-')
		{
			fprintf(stderr, "Invalid option %s\n", argv[i]);
			return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
		}
		
		/*
			SCENE FILE
		*/
        else
		{
			/*
				Any orphan argument is treated as the expected scene file path.
			*/
			printf("Adding scene %i -> %s\n", scenes.size()+1, argv[i]);
			NCFGParser *scene = new NCFGParser(argv[i]);
			scenes.push_back(scene);				
		}
    }

	switch(xt_mode_net)
	{
		case XTRACER_NET_MASTER:
			printf("Setting up master node..\n");
		case XTRACER_NET_LOCAL:	
			break;
		case XTRACER_NET_SLAVE:
			printf("Setting up as slave node..\n");
			break;
	}
			
	/*
		CLEAN UP
	*/
	if (!scenes.empty())
	{
		printf("Cleaning up the scene queue [%i]..\n", scenes.size());
		for(std::list<NCFGParser *>::iterator it = scenes.begin(); it != scenes.end(); it++)
		{
			printf("Releasing scene -> %s\n", (*it)->get_source().c_str());
			delete *it;
		}
	}

	return XTRACER_STATUS_OK;
}

