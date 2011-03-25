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


#include "xtracer.h"

#include "err.h"
#include "net.h"
#include "drv.hpp"
#include "drv/sdl.hpp"
#include "renderer.hpp" 

class Setupenvvar
{
	public:
		Setupenvvar():
			width(0), height(0),						/* Framebuffer */
			net(XT_NET_LOCAL), port(XT_NET_PROT_PORT),	/* Network */
			drv(XT_DEFAULT_DRV), 
			depth(XT_DEFAULT_RECUR_DEPTH)				/* Renderer */
		{}

		/* Framebuffer */
		unsigned int width;
		unsigned int height;

		/* Network */
		xt_mode_net_t net;
		std::string host;
		unsigned int port;

		/* Out drivers */
		XT_DRV drv;
		std::string filepath;

		/* Renderer */
		unsigned int depth;
		std::string camera;
		std::list<std::string> fscenes;
};

/* Instance of the above class, used in cli argument parsing */
Setupenvvar envvar;

xt_status_t parsearg(int argc, char **argv)
{
	/* 
		Version 
	*/
	if( argc == 2 && ((!strcmp(argv[1], "-version")) || (!strcmp(argv[1], "-ver"))))
	{
		printf("Xtracer v0.2 © 2010-2011 Papadopoulos Nikos\nUsage:	%s [option]... scene_file ...\nCheck the man page for a complete list of options.\n", argv[0]);
		return XT_STATUS_OK;
	}
    
	for (int i = 1; i < argc; i++)
	{
		/*
			Net mode
		*/
		if (!strcmp(argv[i], "-net"))
		{
			i++;
			if (!argv[i])
			{
				fprintf(stderr, "No net mode was provided.\nAvailable modes: local, master, slave.\n");
				return XT_STATUS_INVALID_CLI_ARGUMENT;
			}

			/* Local */
			if (!strcmp(argv[i], "local"))
			{
				envvar.net = XT_NET_LOCAL;
			}
			/* Master */
			else if (!strcmp(argv[i], "master"))
			{
				envvar.net = XT_NET_MASTER;
			}
			/* Slave */
			else if (!strcmp(argv[i], "slave"))
			{
				envvar.net = XT_NET_SLAVE;

				i++;			
				if (!argv[i])
				{
					fprintf(stderr, "No host was provided.\n");
					return XT_STATUS_INVALID_CLI_ARGUMENT;
				}

				char d[1000];
				
				if(strlen(argv[i]) > 999)
				{
					fprintf(stderr, "%s value too long.", argv[i-1]);
					return XT_STATUS_INVALID_CLI_ARGLENGH;
				}
	
				if ((argv[i][0] == '-') || sscanf(argv[i], "%s", &d[0]) < 1)
				{
					fprintf(stderr, "Invalid %s value.\n", argv[i-1]);
					return XT_STATUS_INVALID_CLI_ARGUMENT;
				}

				envvar.host = d;
			}
			/* Invalid mode */
			else
			{
				fprintf(stderr, "Invalid mode %s.\n", argv[i]);
				return XT_STATUS_INVALID_CLI_ARGUMENT;
			}
		}
		/*
			Port
		*/
		else if (!strcmp(argv[i], "-port"))
		{
			i++;

			if (!argv[i])
			{
				fprintf(stderr, "No %s value was provided.\n", argv[i-1]);
				return XT_STATUS_INVALID_CLI_ARGUMENT;
			}

			int d = 0;

			if ((argv[i][1] == '-') || sscanf(argv[i], "%d", &d) < 1) 
            {
                fprintf(stderr, "Invalid %s value.\n", argv[i-1]);
                return XT_STATUS_INVALID_CLI_ARGUMENT;
            }

			envvar.port = d;
		}

		/* 
			Render modes 
		*/
        else if (!strcmp(argv[i], "-drv"))
		{
			i++;			
			if (!argv[i])
			{
				fprintf(stderr, "No driver was provided.\nAvailable drivers: sdl, img, asc.\n");
				return XT_STATUS_INVALID_CLI_ARGUMENT;
			}

			/* SDL */
			if (!strcmp(argv[i], "sdl"))
			{
				envvar.drv = XT_DRV_SDL;
			}

			/* Image */
			else if (!strcmp(argv[i], "img"))
			{
				envvar.drv = XT_DRV_IMG;

				i++;

				if (!argv[i])
				{
					fprintf(stderr, "No %s value was provided.\n", argv[i-1]);
					return XT_STATUS_INVALID_CLI_ARGUMENT;
				}

				char d[1000];
				
				if(strlen(argv[i]) > 999)
				{
					fprintf(stderr, "%s value too long.", argv[i-1]);
					return XT_STATUS_INVALID_CLI_ARGLENGH;
				}
		
				if ((argv[i][0] == '-') || sscanf(argv[i], "%s", &d[0]) < 1)
				{
					fprintf(stderr, "Invalid %s value.\n", argv[i-1]);
					return XT_STATUS_INVALID_CLI_ARGUMENT;
				}

				envvar.filepath = d;
			}
			
			/* ASCII */
			else if (!strcmp(argv[i], "asc"))
			{	
				envvar.drv = XT_DRV_ASC;
			}
			else
			{
				fprintf(stderr, "Invalid driver %s.\n", argv[i]);
				return XT_STATUS_INVALID_CLI_ARGUMENT;
			}
		}

		/* 
			Renderer depth
		*/
		else if (strcmp(argv[i], "-depth") == 0)
		{
			i++;
			int d = 0;
			if (!argv[i])
			{
				fprintf(stderr, "No value was provided for %s.\n", argv[i-1]);
				return XT_STATUS_INVALID_CLI_ARGUMENT;
			}

			if ((argv[i][0] == '-') || sscanf(argv[i], "%d", &d) < 1) 
            {
                fprintf(stderr, "Invalid %s value.\n", argv[i-1]);
                return XT_STATUS_INVALID_CLI_ARGUMENT;
            }

			envvar.depth = d;
		}

		/*
			Framebuffer resolution
		*/
        else if (!strcmp(argv[i], "-res"))
		{
            i++;

			if (!argv[i])
			{
				fprintf(stderr, "No value was provided for %s.\n", argv[i-1]);
				return XT_STATUS_INVALID_CLI_ARGUMENT;
			}
			
			int w = 0, h = 0;

            if ((argv[i][0] == '-') || sscanf(argv[i], "%dx%d", &w, &h) < 2)
			{
                fprintf(stderr, "Invalid %s value. Should be %%ix%%i.\n", argv[i-1]);
                return XT_STATUS_INVALID_CLI_ARGUMENT;
            }

			envvar.width = w;
			envvar.height = h;
        }

		/* 
			Camera 
		*/
		else if(!strcmp(argv[i], "-cam"))
		{
			i++;

			if (!argv[i])
			{
				fprintf(stderr, "No value was provided for %s.\n", argv[i-1]);
				return XT_STATUS_INVALID_CLI_ARGUMENT;
			}

			char d[1000];
			if(strlen(argv[i]) > 999)
			{
				fprintf(stderr, "%s value too long.", argv[i-1]);
				return XT_STATUS_INVALID_CLI_ARGLENGH;
			}

			if ((argv[i][0] == '-') || sscanf(argv[i], "%s", &d[0]) < 1)
			{
				fprintf(stderr, "Invalid %s value.\n", argv[i-1]);
				return XT_STATUS_INVALID_CLI_ARGUMENT;
			}

			envvar.camera = d;
		}

		/*
			Invalid option
		*/
		else if (argv[i][0] == '-')
		{
			fprintf(stderr, "Invalid option '%s'.\n", argv[i]);
			return XT_STATUS_INVALID_CLI_ARGUMENT;
		}
		
		/* 
			Scene files 
		*/
        else
		{
			/* Any orphan argument is treated as a scene file path. */
			envvar.fscenes.push_back(argv[i]);				
		}
    }
	return XT_STATUS_OK;
}



int main(int argc, char **argv)
{
	/* Parse the argument list */
	{
		xt_status_t status = parsearg(argc, argv);
		if(status != XT_STATUS_OK)
		{
			return status;
		}
	}
	
	printf("▄ ▄ ▄▄▄ ▄▄▄ ▄▄▄ ▄▄▄ ▄▄▄ ▄▄▄\n");
	printf("▀▄▀  █  █▄▀ █▄█ █   █▄▄ █▄▀\n");
	printf("█ █  █  █ █ █ █ █▄▄ █▄▄ █ █\n"); 
	printf("v%s\n\n", XT_VERSION);
		
	/* Process the scene list */
	if ( envvar.fscenes.empty() && (envvar.net != XT_NET_SLAVE))
	{
		fprintf(stderr, "No scenes were provided.\n");
		return XT_STATUS_MISSING_SCENE_FILE;	
	}
	else if(envvar.net == XT_NET_SLAVE)
	{
		fprintf(stderr, "Slave mode was chosen. The provided scene files will be ignored.\n");
		envvar.fscenes.clear();
	}

	/* Startup networking if required */
	net_set_mode(envvar.net);
	net_init(envvar.port, envvar.host.c_str());

	/* Start up the framebuffer */
	printf("Initiating the framebuffer..\n");
	Framebuffer fb(envvar.width, envvar.height);
	printf("Buffer size: %ix%i\n", fb.width(),  fb.height());

	/* 
		Start up the output driver
		NOTE: This will be allocated dynamically. Don't forget to clean up later on.
	*/
	printf("Initiating the output driver..\n");
	Driver *drv = NULL;
	switch (envvar.drv)
	{
		case XT_DRV_SDL:
			drv = new DrvSDL(fb);
			break;
		case XT_DRV_DUM:	/* DUMMY DRIVER */
		default:
			drv = new Driver(fb);

	}
	drv->init();

	/* Asynchronous output update */
	/* HANDLE HERE */

	unsigned int count = 1;
	unsigned int total = envvar.fscenes.size();
	while(!envvar.fscenes.empty())
	{	
		printf("Processing %i / %i: %s..\n", count++, total, envvar.fscenes.front().c_str());
		
		/* Start up the renderer */
		Renderer renderer(envvar.fscenes.front().c_str(), fb, envvar.depth);

		printf("Recursion depth: %i\n", renderer.recursion_depth());

		renderer.render(envvar.camera.c_str());

		envvar.fscenes.pop_front();
		
		drv->update();
	}
	
	/*
		CLEAN UP
	*/
	/* Terminate networking */
	printf("Shutting down..\n");
	net_deinit();

	/* Terminate the output driver */
	drv->deinit();
	delete drv;

	/* All the other systems, will shut down automatically */
	
	return XT_STATUS_OK;
}
