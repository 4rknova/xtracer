#include <string>
#include <sstream>

#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nimg/pixmap.h>
#include <nimg/ppm.h>
#include <nimg/img.h>
#include <ncf/util.h>

#include <xtcore/proto.h>
#include <xtcore/scene.h>
#include <xtcore/timeutil.h>
#include <xtcore/log.h>
#include <xtcore/context.h>
#include <xtcore/broadcast.h>

#include <plr_photonmapper/renderer.h>
#include <plr_depth/depth.h>
#define NIMG_TEST
#include <nimg/genetic_algo.cc>
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

    size_t width, height;
    std::string renderer_name, outdir, scene_path, camera;
    xtracer::render::params_t params;
    std::list<std::string> modifiers;

	if (setup(argc, argv
            , width
            , height
            , renderer_name
            , outdir
            , scene_path
            , modifiers
            , camera
            , params
    )) return 1;

	Pixmap fb;
    fb.init(width, height);

    std::string t = "xtracer";
    t.append(" ");
    t.append(scene_path);

    xtracer::network::broadcast(t.c_str());

	// create and initialize the scene
	Scene scene;
	if (!scene.load(scene_path.c_str(), modifiers)) {

        if (scene.m_cameras.size() == 0) {
            Log::handle().log_error("no cameras found");
            return 2;
        }

        scene.camera = camera;
        // Create the renderer.

		xtracer::render::IRenderer *renderer = NULL;

        if (!strcmp(renderer_name.c_str(), "depth")) renderer = new DRenderer();
        else renderer = new Renderer();
		xtracer::render::context_t  context;
		context.scene       = &scene;
		context.framebuffer = &fb;
        context.params      = params;
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

		path_comp(scene_path, base, file, path_delim);
		std::string cam = context.scene->camera;
        if (cam.empty()) cam = XTPROTO_PROP_DEFAULT;

		if (outdir[outdir.length()-1] != path_delim && !outdir.empty()) {
			outdir.append(1, path_delim);
		}

		file = outdir + file + "_cam-" + cam  + ".png";

		Log::handle().log_message("Exporting to %s..", file.c_str());
		if (nimg::io::save::png(file.c_str(), fb)) {
			Log::handle().log_error("Failed to export image file");
		}
	}

	return 0;
}
