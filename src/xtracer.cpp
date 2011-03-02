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

#include <cstdio>
#include <cstring>
#include <string>
#include <list>

#include <SDL/SDL.h>

#include "xtracer.h"
#include "renderer.hpp" 
#include "err.h"
#include "net.h"

int main(int argc, char **argv)
{
	/* setup flags */
	unsigned int xt_mode_rnd = XTRACER_DEFAULT_MODE_RND;	/* flag to setup the rendering mode */
	unsigned int xt_mode_net = XTRACER_DEFAULT_MODE_NET;	/* flag to setup the program to run as a master node */

	/* environment */
	unsigned int width = XTRACER_DEFAULT_SCREEN_WIDTH;		/* output buffer width */
	unsigned int height = XTRACER_DEFAULT_SCREEN_HEIGHT;	/* output buffer height */
	unsigned int rdepth = XTRACER_DEFAULT_RECURSION_DEPTH;	/* maximum recursion depth */

	/* list of scenes to render */
	std::list<std::string> fscenes;

	/* output filepath for drivers that need it */
	std::string filepath;
	int port = XT_NET_PROT_PORT;
	std::string host;

	/* Parse the cli arguments */
	
	/*
		VERSION
	*/

	if( argc == 2 && ((!strcmp(argv[1], "-version")) || (!strcmp(argv[1], "-v")) || (!strcmp(argv[1], "-ver"))))
	{
		printf("Xtracer v0.0\nby Papadopoulos Nikos 2010\nusage: %s [option]... scene_file ...\n", argv[0]);
		return XTRACER_STATUS_OK;
	}
    
	for (int i = 1; i<argc; i++)
	{
		/*
			NET MODES
		*/
		if (!strcmp(argv[i], "-mode"))
		{
			i++;
			if (!argv[i])
			{
				fprintf(stderr, "No mode was provided. Available modes: local, master, slave.\n");
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

				i++;			
				if (!argv[i])
				{
					fprintf(stderr, "No host was provided.\n");
					return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
				}

				char d[1000];
				
				if(strlen(argv[i]) > 999)
				{
					fprintf(stderr, "%s value too long.", argv[i-1]);
					return XTRACER_STATUS_INVALID_CLI_ARGLENGH;
				}
	
				if ((argv[i][0] == '-') || sscanf(argv[i], "%s", &d[0]) < 1)
				{
					fprintf(stderr, "Invalid %s value.\n", argv[i-1]);
					return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
				}
				host = d;
			}
			else
			{
				fprintf(stderr, "Invalid mode %s.\n", argv[i]);
				return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
			}
		}		
	
		/*
			PORT
		*/
		else if (!strcmp(argv[i], "-port"))
		{
			i++;
			if (!argv[i])
			{
				fprintf(stderr, "No port was provided.\n");
				return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
			}

			int d = 0;

			if ((argv[i][1] == '-') || sscanf(argv[i], "%d", &d) < 1) 
            {
                fprintf(stderr, "Invalid %s value.\n", argv[i-1]);
                return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
            }
			port = d;
		}

		/*
			RENDER MODES
		*/
        else if (!strcmp(argv[i], "-drv"))
		{
			i++;			
			if (!argv[i])
			{
				fprintf(stderr, "No driver was provided. Available drivers: sdl, img, asc.\n");
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
					fprintf(stderr, "No file was provided.\n");
					return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
				}

				char d[1000];
				
				if(strlen(argv[i]) > 999)
				{
					fprintf(stderr, "%s value too long.", argv[i-1]);
					return XTRACER_STATUS_INVALID_CLI_ARGLENGH;
				}
		
				if ((argv[i][0] == '-') || sscanf(argv[i], "%s", &d[0]) < 1)
				{
					fprintf(stderr, "Invalid %s value.\n", argv[i-1]);
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
				fprintf(stderr, "Invalid driver %s.\n", argv[i]);
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
				fprintf(stderr, "No value was provided for %s.\n", argv[i-1]);
				return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
			}

			if ((argv[i][0] == '-') || sscanf(argv[i], "%d", &d) < 1) 
            {
                fprintf(stderr, "Invalid %s value.\n", argv[i-1]);
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
				fprintf(stderr, "No value was provided for %s.\n", argv[i-1]);
				return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
			}
			
			int w = 0;
			int h = 0;
            if ((argv[i][0] == '-') || sscanf(argv[i], "%dx%d", &w, &h) < 2)
			{
                fprintf(stderr, "Invalid %s value. Should be %%ix%%i.\n", argv[i-1]);
                return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
            }
			width = w;
			height = h;
        }
		else if (argv[i][0] == '-')
		{
			fprintf(stderr, "Invalid option '%s'.\n", argv[i]);
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
			fscenes.push_back(argv[i]);				
		}
    }

	if (fscenes.empty() && (xt_mode_net != XTRACER_NET_SLAVE))
	{
		fprintf(stderr, "No scenes were provided.\n");
		return XTRACER_STATUS_MISSING_SCENE_FILE;	
	}
	else if(xt_mode_net == XTRACER_NET_SLAVE)
	{
		fprintf(stderr, "Slave mode was chosen. The provided scene files will be ignored.\n");
		fscenes.clear();
	}
	
	/* Startup networking if required */
	net_set_mode(xt_mode_net);
	net_init(port, host.c_str());

	while(!fscenes.empty())
	{	
		XTFramebuffer fb(width, height);
		printf("Processing: %s..\nBuffer size: %ix%i\n", fscenes.front().c_str(), fb.get_width(), fb.get_height());
		xt_render(fscenes.front().c_str(), fb);
		/* Scene processing done */
		fscenes.pop_front();
	}
	
	/*
		CLEAN UP
	*/
	/* Terminate networking */
	net_deinit();
	
	return XTRACER_STATUS_OK;
}
