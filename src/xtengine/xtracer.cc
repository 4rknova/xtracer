#include <string>
#include <sstream>

#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nimg/pixmap.h>
#include <nimg/ppm.h>
#include <nimg/img.h>
#include <ncf/util.h>

#include <plr_photonmapper/renderer.h>

#include <xtcore/scene.h>
#include <xtcore/timeutil.hpp>
#include <xtcore/log.hpp>
#include <xtcore/context.h>
#include "argdefs.h"
#include "argparse.hpp"
#include "plm.h"

using Util::String::to_string;
using Util::String::path_comp;

int main(int argc, char **argv)
{
	PLM::load();

    nimg::Pixmap p;
    nimg::io::load::image("lena.png", p);
    nimg::io::save::ppm_raw("lena.ppm", p);

    return 0;

	// Display usage information.
	if (argc == 2 && !strcmp(argv[1], XTRACER_ARGDEFS_VERSION)) {
		Log::handle().log_message("%s", XTRACER_VERSION);
		return 1;
	}

	Log::handle().log_message("XTracer %s (C) 2010-%s Nikos Papadopoulos", XTRACER_VERSION, XTRACER_YEAR);

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
    fb.init(Environment::handle().width(), Environment::handle().height());
	Environment::handle().log_info();

	// create and initialize the scene
	Scene scene;
	if (!scene.load(Environment::handle().scene())) {
		// apply the modifiers
		Log::handle().log_message("Applying modifiers..");
		scene.apply_modifiers();

		Log::handle().log_message("- Name: %s", scene.name());

		// Build the scene data
		scene.build();

        if (scene.m_cameras.size() == 0) {
            Log::handle().log_error("no cameras found");
            return 2;
        }

        scene.camera = Environment::handle().active_camera_name();

		// Create the renderer.
		xtracer::render::IRenderer *renderer = new Renderer();
		xtracer::render::context_t  context;
		context.scene       = &scene;
		context.framebuffer = &fb;

		renderer->setup(context);

		Timer timer;

		// Render.
		timer.start();
		renderer->render();
		timer.stop();

		delete renderer;

		print_time_breakdown(timer.get_time_in_mlsec());

		std::string file;

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

		Log::handle().log_message("Exporting to %s..", file.c_str());
		if (nimg::io::save::ppm_raw(file.c_str(), fb)) {
			Log::handle().log_error("Failed to export image file");
		}
	}

	// Clean up.
	Log::handle().log_message("Shutting down.");

	return 0;
}
