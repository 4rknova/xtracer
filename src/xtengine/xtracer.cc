#include <string>
#include <sstream>

#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nimg/pixmap.h>
#include <nimg/ppm.h>
#include <nimg/img.h>
#include <ncf/util.h>

#include <xtcore/scene.h>
#include <xtcore/timeutil.h>
#include <xtcore/log.h>
#include <xtcore/context.h>

#include <plr_photonmapper/renderer.h>

#include "argdefs.h"
#include "argparse.h"
#include "plm.h"

using Util::String::to_string;
using Util::String::path_comp;

int main(int argc, char **argv)
{
	// Display usage information.
	if (argc == 2 && !strcmp(argv[1], XTRACER_ARGDEFS_VERSION)) {
		Log::handle().log_message("%s", XTRACER_VERSION);
		return 1;
	}

	Log::handle().log_message("xtracer %s (C) 2010-%s Nikos Papadopoulos", XTRACER_VERSION, XTRACER_YEAR);

	if (argc == 2 && !strcmp(argv[1], XTRACER_ARGDEFS_HELP)) {
		Log::handle().log_message("Usage: %s [option]... scene_file...", argv[0]);
		Log::handle().log_message("For a complete list of the available options, refer to the man pages.");
		return 1;
	}

    PLM::load();

	if (Environment::handle().setup(argc, argv)) return 1;

	Pixmap fb;
    fb.init(Environment::handle().width(), Environment::handle().height());
	Environment::handle().log_info();

	// create and initialize the scene
	Scene scene;
	if (!scene.load(Environment::handle().scene())) {
		scene.apply_modifiers();
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
		Environment::handle().configure(context.params);

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
					  + ".png";

		Log::handle().log_message("Exporting to %s..", file.c_str());
		if (nimg::io::save::png(file.c_str(), fb)) {
			Log::handle().log_error("Failed to export image file");
		}
	}

	return 0;
}
