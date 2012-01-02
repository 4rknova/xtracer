/*

    This file is part of xtracer.

	xtracer.cpp
	Xtracer entry point

    Copyright (C) 2010, 2011
    Papadopoulos Nikolaos

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General
    Public License along with this program; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301 USA

*/

#ifdef _MSC_VER
	#include <fcntl.h>
	#include <io.h>
	#define WIN32_LEAN_AND_MEAN
	#define WIN64_LEAN_AND_MEAN
	#include <windows.h>
#endif /* _MSC_VER */

#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include "nmath/mutil.h"

#include "setup.hpp"

// the scenes to be processed
#include <list>
std::list<std::string> fscenes;
std::list<std::string> modifiers;

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

// exposure
double exposure = 0;

// maximum recursion depth
int max_rdepth = XT_SETUP_DEFAULT_MAXRDEPTH;

// other flags
bool flag_render_light_positions = false;
bool flag_realtime_update = false;

// antialiasing
int antialiasing = 0;

// threads to spawn
int threads = 0;

// dof samples
int dof_samples = 2;

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
				std::cerr << "Invalid " << argv[i-1] << " value. Should be %i.\n";
				return 1;
			}

			if (max_rdepth < 0)
			{
				std::cerr
					<< "Invalid " << argv[i-1] << " value. "
					<< "You provided a negative number.\n";
				return 1;
			}
		}
		// dof samples
		else if (!strcmp(argv[i], "-dofsamples"))
		{
			i++;

			if (!argv[i])
			{
				std::cerr << "No value was provided for " << argv[i-1] << "\n";
				return 1;
			}

			if (sscanf(argv[i], "%d", &dof_samples) < 1)
			{
				std::cerr << "Invalid " << argv[i-1] << " value. Should be %i.\n";
				return 1;
			}

			if (dof_samples < 2)
			{
				std::cerr
					<< "Invalid " << argv[i-1] << " value. "
					<< "You must provide an integer equal to 2 or greater.\n";
				return 1;
			}
		}
		// antialiasing
		else if (!strcmp(argv[i], "-aa"))
		{
			i++;

			if (!argv[i])
			{
				std::cerr << "No value was provided for " << argv[i-1] << "\n";
				return 1;
			}

			if (sscanf(argv[i], "%d", &antialiasing) < 1)
			{
				std::cerr << "Invalid " << argv[i-1] << " value. Should be %i.\n";
				return 1;
			}
			
			if (antialiasing <= 0)
			{
				std::cerr
					<< "Invalid " << argv[i-1] << " value. "
					<< "You must provide an integer equal to 1 or greater.\n";
				return 1;
			}

			if (!is_power_of_2(antialiasing))
			{
				std::cerr << "Invalid " << argv[i-1] << " value. Should be a power of 2.\n";
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
				#ifndef ENABLE_SDL
					std::cout << "This version was built without the sdl driver support.\n";
					return 1;
				#endif /* ENABLE_SDL */
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
					<< "Please consult the man pages for the available drivers.\n";
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
		// exposure
		else if (!strcmp(argv[i], "-exposure"))
		{
			i++;

			if (!argv[i])
			{
				std::cerr << "No value was provided for " << argv[i-1] << "\n";
				return 1;
			}	
            
			if (sscanf(argv[i], "%lf", &exposure) < 1)
			{
                std::cerr << "Invalid " << argv[i-1] << " value. Should be a %lf.\n";
			}				
			
			if (exposure <= 0)
			{
				std::cerr
					<< "Invalid " << argv[i-1] << " value. "
					<< "You must provide a positive integer.\n";
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
		// modifiers
		else if (!strcmp(argv[i], "-mod"))
		{
			i++;

			if (!argv[i])
			{
				std::cerr << "No value was provided for " << argv[i-1] << "\n";
				return 1;
			}

			modifiers.push_back(argv[i]);
		}
		else if (!strcmp(argv[i], "-lightpos"))
		{
			flag_render_light_positions = true;
		}
		else if (!strcmp(argv[i], "-realtime"))
		{
			flag_realtime_update = true;
		}
		// threads
		else if (!strcmp(argv[i], "-threads"))
		{
			i++;

			if (!argv[i])
			{
				std::cerr << "No value was provided for " << argv[i-1] << "\n";
				return 1;
			}

			if (sscanf(argv[i], "%d", &threads) < 1)
			{
				std::cerr << "Invalid " << argv[i-1] << " value. Should be %i.\n";
				return 1;
			}

			if (threads < 0)
			{
				std::cerr
					<< "Invalid " << argv[i-1] << " value. "
					<< "You provided a negative number.\n";
				return 1;
			}
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

#include "nmath/mutil.h"

// framebuffer
#include "fb.hpp"

// drivers
#include "drv.hpp"
#include "drvsdl.hpp"
#include "drvppm.hpp"

#include "console.hpp"
#include "renderer.hpp"
#include "scene.hpp"

int main(int argc, char **argv)
{
	#ifdef _MSC_VER
		AllocConsole();
		HANDLE lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		long hConHandle = _open_osfhandle((long)lStdHandle, _O_TEXT);
		FILE *fp = _fdopen( hConHandle, "w" );
		*stdout = *fp;
		setvbuf( stdout, NULL, _IONBF, 0 );
		setvbuf( stderr, NULL, _IONBF, 0 );
	#endif /* _MSC_VER */

	#ifdef XT_VERSION
		std::cout << "Xtracer v" << XT_VERSION << " (c) 2010-2012 Papadopoulos Nikos\n";
	#else
		std::cout << "Xtracer (c) 2010-2012 Papadopoulos Nikos\n";
	#endif /* XT_VERSION */

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

	// initiate the console
	Console con;

	// initiate the output driver
	std::cout << "Initiating the output driver..\n";
	Driver *drv = NULL;
	switch ((int)driver)
	{
		#ifdef ENABLE_SDL
			case XT_DRV_SDL:
				drv = new DrvSDL(fb, con);
				break;
		#endif /* ENABLE_SDL */

		case XT_DRV_PPM:
			drv = new DrvPPM(fb, con);
			break;
		case XT_DRV_DUM:	/* DUMMY DRIVER */
		default:
			drv = new Driver(fb, con);
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

		// apply modifiers
		while(!modifiers.empty())
		{
			scene.apply_modifier(modifiers.front().c_str());
			modifiers.pop_front();
		}

		// Build the scene data
		scene.build();
		
		if (verbose)
			scene.analyze();

		// set the camera
		scene.set_camera(camera.c_str());

		// create the renderer
		Renderer renderer(fb, scene, drv, max_rdepth);

		// setup the environment
		renderer.max_recursion_depth(max_rdepth);
		renderer.light_geometry(flag_render_light_positions);
		renderer.antialiasing(antialiasing);

		// postprocessing
		renderer.gamma_correction(gamma_correction);
		renderer.exposure(exposure);
		renderer.dof_samples(dof_samples);
		
		// realtime update
		if (flag_realtime_update && !(drv->is_realtime()))
			std::cout << "Warning: Realtime output update cannot be used with this driver. Ignoring..\n";
		else
			renderer.realtime_update(flag_realtime_update);

		// render
		renderer.threads(threads);
		renderer.render();
	}
	
	// clean up
	std::cout 
		<< "\n"
		<< "Shutting down..\n";

	// release the dynamically allocated memory
	delete drv;	// The driver
	
	return 0;
}
