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

#include "renderer.hpp" 

#include "xtracer.h"
#include "out.h"
#include "err.h"
#include "net.h"

#include "pixel.h"

int main(int argc, char **argv)
{
	/* Initiate subsystems */
	out_init();				/* Initiate the output environment */
	xtrenderer_init(); 		/* Initiate the renderer environment */

	/* setup flags */
	unsigned int xt_mode_net = XTRACER_DEFAULT_MODE_NET;	/* flag to setup the program to run as a master node */

	/* list of scenes to render */
	std::list<std::string> fscenes;

	/* output filepath for drivers that need it */
	std::string filepath;
	int port = XT_NET_PROT_PORT;
	std::string host;

	/* Parse the cli arguments */
	
	/* Version */
	if( argc == 2 && ((!strcmp(argv[1], "-version")) || (!strcmp(argv[1], "-v")) || (!strcmp(argv[1], "-ver"))))
	{
		printf("Xtracer v0.2 © 2010-2011 Papadopoulos Nikos\nusage:	%s [option]... scene_file ...\nCheck the man page for a complete list of options.\n", argv[0]);
		return XTRACER_STATUS_OK;
	}
    
	for (int i = 1; i<argc; i++)
	{
		/* Net modes */
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
		else if (!strcmp(argv[i], "-port"))
		{
			i++;
			if (!argv[i])
			{
				fprintf(stderr, "No %s value was provided.\n", argv[i-1]);
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

		/* Render modes */
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
				out_set_drv(XTRACER_DRV_SDL);
			}
			else if (!strcmp(argv[i], "img"))
			{
				out_set_drv(XTRACER_DRV_IMG);

				i++;

				if (!argv[i])
				{
					fprintf(stderr, "No %s value was provided.\n", argv[i-1]);
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
			}
			else if (!strcmp(argv[i], "asc"))
			{	
				out_set_drv(XTRACER_DRV_ASC);
			}
			else
			{
				fprintf(stderr, "Invalid driver %s.\n", argv[i]);
				return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
			}
		}

		/* Synchronization */
		else if (strcmp(argv[i], "-async") == 0)
		{
			out_set_syn(XTRACER_SYN_ASYN);
		}
		else if (strcmp(argv[i], "-sync") == 0)
		{
			out_set_syn(XTRACER_SYN_SYNC);
		}
		else if (strcmp(argv[i], "-syncintv") == 0)
		{
			i++;

			if (!argv[i])
			{
				fprintf(stderr, "No %s value was provided.\n", argv[i-1]);
				return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
			}

			 int d = 0;
			 
			 if ((argv[i][1] == '-') || sscanf(argv[i], "%d", &d) < 1)
			 {
				fprintf(stderr, "Invalid %s value.\n", argv[i-1]);
				return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
			}
			
			out_set_syn_intv(d);
		}
		
		/* Renderer environment  */
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

			xtrenderer_set_rdepth(d);
		}
        else if (!strcmp(argv[i], "-res"))
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

			xtrenderer_set_width(w);
			xtrenderer_set_height(h);
        }
		else if (argv[i][0] == '-')
		{
			fprintf(stderr, "Invalid option '%s'.\n", argv[i]);
			return XTRACER_STATUS_INVALID_CLI_ARGUMENT;
		}
		
		/* Scene files */
        else
		{
			/* Any orphan argument is treated as a scene file path. */
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
	
	/* Startup the output driver */
	out_drv_init(640,480,24);
	
	/* Startup networking if required */
	net_set_mode(xt_mode_net);
	net_init(port, host.c_str());

	printf("Buffer size: %ix%i, Recursion depth: %i\n", 
			xtrenderer_get_width(),  xtrenderer_get_height(),
			xtrenderer_get_rdepth());

	while(!fscenes.empty())
	{	
		printf("Processing: %s..\n", fscenes.front().c_str());

		xtrender(fscenes.front().c_str());

		fscenes.pop_front();
	}
	
	/*
		CLEAN UP
	*/
	/* Terminate networking */
	out_drv_deinit();
	// xt_renderer_deinit();
	net_deinit();
	
	return XTRACER_STATUS_OK;
}
