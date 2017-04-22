#include <string>

#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nimg/pixmap.h>
#include <nimg/img.h>
#include <ncf/util.h>

#include <xtcore/proto.h>
#include <xtcore/scene.h>
#include <xtcore/timeutil.h>
#include <xtcore/log.h>
#include <xtcore/context.h>
#include <xtcore/argparse.h>

#include <xtcore/plr_photonmapper/renderer.h>
#include <xtcore/plr_stencil/renderer.h>
#include <xtcore/plr_depth/depth.h>

#define ARG_CLI_VERSION "version" /* Display version and exit */

#define RENDERER(x)   (!strcmp(renderer_name.c_str(), x))
#define ARGUMENT(i,x) (!strcmp(argv[i], x))

int main(int argc, char **argv)
{
	if (argc == 2 && ARGUMENT(1, ARG_CLI_VERSION)) {
		Log::handle().post_message("%s", XTRACER_VERSION);
		return 1;
	}

	Log::handle().post_message("xtracer %s (C) 2010-%s Nikos Papadopoulos", XTRACER_VERSION, XTRACER_YEAR);

    std::string renderer_name, outdir, scene_path, camera;
    xtracer::render::params_t params;
    std::list<std::string> modifiers;

	if (setup(argc, argv
            , renderer_name
            , outdir
            , scene_path
            , modifiers
            , camera
            , params
    )) return 1;

	Scene scene;
	if (!scene.load(scene_path.c_str(), modifiers)) {
        if (scene.m_cameras.size() == 0) {
            Log::handle().post_error("no cameras found");
            return 2;
        }
        scene.camera = camera;
    } else return 1;

	xtracer::render::IRenderer *renderer = NULL;

    if      (RENDERER("depth")  ) renderer = new xtracer::renderer::depth::Renderer();
    else if (RENDERER("stencil")) renderer = new xtracer::renderer::stencil::Renderer();
    else                          renderer = new Renderer();

	xtracer::render::context_t  context;
	context.scene  = &scene;
    context.params = params;
    context.init();

    renderer->setup(context);

	Timer timer;
	timer.start();
	renderer->render();
	timer.stop();

    std::string t;
	print_time_breakdown(t, timer.get_time_in_mlsec());
    Log::handle().post_message("Total time: %s", t.c_str());

    // Export the frame
    {
	    std::string file, base, extension, filename, sw, sh, sa, random_token;
		#ifdef _WIN32
    		const char path_delim = '\\';
	    #else
		    const char path_delim = '/';
		#endif /* _WIN32 */

    	ncf::util::path_comp(scene_path, base, file, path_delim);
    	ncf::util::path_comp(file, filename, extension, '.');
	    std::string cam = context.scene->camera;

        if (cam.empty()) cam = XTPROTO_PROP_DEFAULT;

    	if (outdir[outdir.length()-1] != path_delim && !outdir.empty()) {
			outdir.append(1, path_delim);
		}

	    file = outdir + filename + cam  + "_" + renderer_name + ".png";
        nimg::Pixmap fb;
        xtracer::render::assemble(fb, context);
		Log::handle().post_message("Exporting to %s..", file.c_str());
		int res = nimg::io::save::png(file.c_str(), fb);
        if (res) Log::handle().post_error("Failed to export image file");
    }

	delete renderer;
    return 0;
}
