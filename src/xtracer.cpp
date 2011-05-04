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
#include <iostream>

#include "setup.hpp"

// the scenes to be processed
#include <list>
std::list<std::string> fscenes;

// the output driver
XT_DRV driver = XT_SETUP_DEFAULT_DRV;

// the resolution
int width = XT_SETUP_DEFAULT_WIDTH;
int height = XT_SETUP_DEFAULT_HEIGHT;

// the camera
std::string camera;

// verbosity
unsigned int verbose = 0;

// gamma correction
double gamma_correction = XT_SETUP_DEFAULT_GAMMA;

// maximum recursion depth
int max_rdepth = XT_SETUP_DEFAULT_MAXRDEPTH;

// other flags
bool flag_render_light_positions = false;

unsigned int parsearg(int argc, char **argv)
{
	for (int i = 1; i < argc; i++)
	{
		// resolution
        if (!strcmp(argv[i], "-res"))
		{
            i++;

			if (!argv[i])
			{
				std::cerr << "No value was provided for " << argv[i-1] << "\n";
				return 1;
			}
			
            if (sscanf(argv[i], "%dx%d", &width, &height) < 2)
			{
                std::cerr << "Invalid " << argv[i-1] << " value. Should be %ix%i.\n";
                return 1;
            }

			if ((width <= 0) || (height <= 0))
			{
				std::cerr 
					<< "Invalid " << argv[i-1] << " value. "
					<< "You provided a negative dimension size.\n";
				return 1;
			}
        }
		// maximum recursion depth
		else if (!strcmp(argv[i], "-depth"))
		{
			i++;

			if (!argv[i])
			{
				std::cerr << "No value was provided for " << argv[i-1] << "\n";
				return 1;
			}

			if (sscanf(argv[i], "%d", &max_rdepth) < 1)
			{
				std::cerr << "Invalid " << argv[i-1] << " value. Should be %ix%i.\n";
				return 1;
			}

			if (max_rdepth <= 0)
			{
				std::cerr
					<< "Invalid " << argv[i-1] << " value. "
					<< "You provided a negative number.\n";
				return 1;
			}
		}
		// output driver
		else if (!strcmp(argv[i], "-drv"))
		{
			i++;

			if (!argv[i])
			{
				std::cerr << "No value was provided for " << argv[i-1] << "\n";
				return 1;
			}
			// ppm driver
			if (!strcmp(argv[i], "ppm"))
			{
				driver = XT_DRV_PPM;
			}
			// sdl driver
			else if (!strcmp(argv[i], "sdl"))
			{
				driver = XT_DRV_SDL;
			}
			// dummy driver
			else if (!strcmp(argv[i], "dum"))
			{
				driver = XT_DRV_DUM;
			}
			else
			{
				std::cerr 
					<< "Invalid " << argv[i-1] << " value. "
					<< "Please consult the man pages for the available drivers.";
				return 1;
			}
		}
		// gamma
		else if (!strcmp(argv[i], "-gamma"))
		{
			i++;

			if (!argv[i])
			{
				std::cerr << "No value was provided for " << argv[i-1] << "\n";
				return 1;
			}

            if (sscanf(argv[i], "%lf", &gamma_correction) < 1)
			{
                std::cerr << "Invalid " << argv[i-1] << " value. Should be a %lf.\n";
				
                return 1;
            }
		}
		// camera
		else if (!strcmp(argv[i], "-cam"))
		{
			i++;

			if (!argv[i])
			{
				std::cerr << "No value was provided for " << argv[i-1] << "\n";
				return 1;
			}

			camera = argv[i];
		}
		else if (!strcmp(argv[i], "-lightpos"))
		{
			flag_render_light_positions = true;
		}
		// verbosity
		else if (!strcmp(argv[i], "-v"))
		{
			verbose++;
		}
		// invalid option
		else if (argv[i][0] == '-')
		{
			std::cerr << "Invalid option: " << argv[i] << "\n";
			return 1;
		}
		// scene files
        else
		{
			// any orphan argument is treated as a scene file path.
			fscenes.push_back(argv[i]);				
		}
    }

	return 0;
}

#include <nmath/mutil.h>

// framebuffer
#include "fb.hpp"

// drivers
#include "drv.hpp"
#include "drvsdl.hpp"
#include "drvppm.hpp"

#include "renderer.hpp"
#include "scene.hpp"

int main(int argc, char **argv)
{
	std::cout << "Xtracer v" << XT_VERSION << " © 2010-2011 Papadopoulos Nikos\n";

	// usage information
	if (argc == 2)
	{
		if (((!strcmp(argv[1], "-version")) || (!strcmp(argv[1], "-ver"))))
		{
			return 0;
		}
		else if (!strcmp(argv[1], "-help"))
		{
			std::cout
				<< "Usage: " << argv[0] << " [option]... scene_file...\n"
				<< "For a complete list of the available options, refer to the man pages.\n";
			return 0;
		}
	}
    
	// parse the argument list
	if(parsearg(argc, argv))
		return 1;

	// check for empty scene list
	if (fscenes.empty())
	{
		std::cerr << "No scenes were provided.\n";
		return 1;	
	}

	// initiate the framebuffer
	std::cout << "Initiating the framebuffer..\n";
	Framebuffer fb(width, height);
	int aspect_gcd = gcd(fb.width(), fb.height());
	std::cout 
		<< "Resolution [ " 
		<< fb.width() / aspect_gcd  << ":" << fb.height() / aspect_gcd << " / "
		<< fb.width() << "x" << fb.height() << " ]\n";

	// initiate the output driver
	std::cout << "Initiating the output driver..\n";
	Driver *drv = NULL;
	switch ((int)driver)
	{
		case XT_DRV_SDL:
			drv = new DrvSDL(fb);
			break;
		case XT_DRV_PPM:
			drv = new DrvPPM(fb);
			break;
		case XT_DRV_DUM:	/* DUMMY DRIVER */
		default:
			drv = new Driver(fb);
	}

	// start processing
	unsigned int scene_index = 1;
	unsigned int scene_total = fscenes.size();
	while(!fscenes.empty())
	{
		// get the front scene path
		std::string source  = fscenes.front();

		// pop the front scene
		fscenes.pop_front();

		std::cout
			<< "\n"
			<< "Processing [ " << scene_index++ << "/" << scene_total << " ]: " 
			<< source << "\n";

		// create and initialize the scene
		Scene scene(source.c_str());

		// initiate the scene and detect errors
		if (scene.init())
			continue;

		if (verbose)
			scene.analyze();

		// set the camera
		scene.set_camera(camera.c_str());
	
		// create the renderer
		Renderer renderer(fb, scene, drv, max_rdepth);
		// setup the environment
		renderer.verbosity(verbose);
		renderer.max_recursion_depth(max_rdepth);
		std::cout << "Maximum recursion depth: "<< renderer.max_recursion_depth() << "\n";
		renderer.gamma_correction(gamma_correction);
		renderer.light_geometry(flag_render_light_positions);
		// render
		renderer.render();
	}
	
	// clean up
	std::cout 
		<< "\n"
		<< "Shutting down..\n";

	// terminate the output driver
	drv->deinit();

	// release the dynamically allocated memory
	delete drv;
	
	return 0;
}
