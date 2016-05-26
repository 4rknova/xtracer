#include <string>
#include <sstream>
#include <nmath/mutil.h>
#include <nmath/prng.h>
#include "util.hpp"
#include "scene.h"
#include "argdefs.h"
#include "argparse.hpp"
#include "timeutil.hpp"
#include "log.hpp"
#include "photon_mapper.h"

#include <nimg/pixmap.h>
#include <nimg/ppm.hpp>

#include "plm.h"


using NCF::Util::to_string;
using NCF::Util::path_comp;

int main(int argc, char **argv)
{
	PLM::load();


	// Display usage information.
	if (argc == 2 && !strcmp(argv[1], XTRACER_ARGDEFS_VERSION)) {
		Log::handle().log_message("%s", XTRACER_VERSION);
		return 1;
	}

	Log::handle().log_message("XTracer %s (C) 2010-2012 Papadopoulos Nikos", XTRACER_VERSION);

	if (argc == 2 && !strcmp(argv[1], XTRACER_ARGDEFS_HELP)) {
		Log::handle().log_message("Usage: %s [option]... scene_file...", argv[0]);
		Log::handle().log_message("For a complete list of the available options, refer to the man pages.");
		return 1;
	}

	// parse the argument list.
	if (Environment::handle().setup(argc, argv)) {
		return 1;
	}

	// Create and initiate the pixmap.
	Log::handle().log_message("Initiating the pixmap..");
	Pixmap fb;

	if (Environment::handle().flag_resume()) {
		if(NImg::IO::Import::ppm_raw(Environment::handle().resume_file(), fb)) {
			Log::handle().log_error("Failed to load %s", Environment::handle().resume_file());
			return 1;
		}
	}
	else {
		fb.init(Environment::handle().width(), Environment::handle().height());
	}

	Environment::handle().log_info();

	// Export.
	if (Environment::handle().output() == XTRACER_OUTPUT_NUL) {
		Log::handle().log_warning("Benchmark mode selected.");
	}

	// create and initialize the scene
	Scene scene;
	if (!scene.load(Environment::handle().scene())) {
		// apply the modifiers
		Log::handle().log_message("Applying modifiers..");
		scene.apply_modifiers();

		Log::handle().log_message("- Name: %s", scene.name());

		// Build the scene data
		scene.build();

		// set the camera
		scene.set_camera(Environment::handle().active_camera_name());

		// Create the renderer.
		Renderer *renderer = new PhotonMapper();

		renderer->setup(fb, scene);

		Timer timer;

		// Render.
		timer.start();
		renderer->render();
		timer.stop();

		delete renderer;

		print_time_breakdown(timer.get_time_in_mlsec());

		if (Environment::handle().output() == XTRACER_OUTPUT_PPM) {
			std::string file;

			if (Environment::handle().flag_resume()) {
				file = Environment::handle().resume_file();
			}
			else {
				std::string base, sw, sh, sa, random_token;

				#ifdef _WIN32
					const char path_delim = '\\';
				#else
					const char path_delim = '/';
				#endif /* _WIN32 */

				path_comp(Environment::handle().scene(), base, file, path_delim);

				std::string cam = Environment::handle().active_camera_name();
				std::string outdir = Environment::handle().outdir();
				if (outdir[outdir.length()-1] != path_delim && !outdir.empty()) {
					outdir.append(1, path_delim);
				}

				to_string(random_token, (int)NMath::prng_c(1000000, 9999999));
				to_string(sw, (int)Environment::handle().width());
				to_string(sh, (int)Environment::handle().height());
				to_string(sa, (int)Environment::handle().aa());
				file = outdir + file + "_cam-" + cam
							  + "_aa" + sa + "_res"
							  + sw + "x" + sh
							  + "_" + random_token
							  + ".ppm";
			}

			Log::handle().log_message("Exporting to %s..", file.c_str());
			if (NImg::IO::Export::ppm_raw(file.c_str(), fb)) {
				Log::handle().log_error("Failed to export image file");
			}
		}
	}

	// Clean up.
	Log::handle().log_message("Shutting down.");

	return 0;
}
