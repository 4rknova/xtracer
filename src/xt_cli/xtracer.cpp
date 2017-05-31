#include <string>
#include <mutex>

#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nimg/pixmap.h>
#include <nimg/img.h>
#include <ncf/util.h>
#include <xtcore/config.h>
#include <xtcore/proto.h>
#include <xtcore/scene.h>
#include <xtcore/timeutil.h>
#include <xtcore/log.h>
#include <xtcore/context.h>
#include <xtcore/renderer/photon_mapper/renderer.h>
#include <xtcore/renderer/stencil/renderer.h>
#include <xtcore/renderer/depth/renderer.h>
#include "argparse.h"

#define PROGRESS_BAR_LENGTH (15)

#define ARG_CLI_VERSION "version" /* Display version and exit */

#define RENDERER(x)   (!strcmp(renderer_name.c_str(), x))
#define ARGUMENT(i,x) (!strcmp(argv[i], x))

static std::mutex mut;

size_t workers   = 0;
size_t completed = 0;
size_t total     = 0;


struct ws_handler_init_t : public xtracer::render::tile_event_handler_t
{
    void handle_event(xtracer::render::tile_t *tile)
    {
        mut.lock();
        ++workers;
        mut.unlock();
    }
};

struct ws_handler_done_t : public xtracer::render::tile_event_handler_t
{
    void handle_event(xtracer::render::tile_t *tile)
    {
       mut.lock();
       ++completed;
       printf("\rRendering.. %5.2f%% @ %lu", (float)completed/total * 100.f, workers);
       fflush(stdout);
       --workers;
       mut.unlock();
    }
};

ws_handler_done_t handler_done;
ws_handler_init_t handler_init;

int main(int argc, char **argv)
{
	if (argc == 2 && ARGUMENT(1, ARG_CLI_VERSION)) {
		Log::handle().post_message("%s", xtcore::get_version());
		return 1;
	}

	Log::handle().post_message("xtracer %s (C) 2010 Nikos Papadopoulos", xtcore::get_version());

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

	xtracer::render::context_t context;
	if (!context.scene.load(scene_path.c_str(), &modifiers)) {
        if (context.scene.m_cameras.size() == 0) {
            Log::handle().post_error("no cameras found");
            return 2;
        }
        context.scene.camera = camera;
    } else return 1;

	xtracer::render::IRenderer *renderer = NULL;

    if      (RENDERER("depth")  ) renderer = new xtracer::renderer::depth::Renderer();
    else if (RENDERER("stencil")) renderer = new xtracer::renderer::stencil::Renderer();
    else                          renderer = new Renderer();

    context.params = params;
    context.init();

    xtracer::render::Tileset::iterator it = context.tiles.begin();
    xtracer::render::Tileset::iterator et = context.tiles.end();
    for (; it != et; ++it) {
        (*it).setup_handler_on_init(&handler_init);
        (*it).setup_handler_on_done(&handler_done);
    }

    renderer->setup(context);
    total =  context.tiles.size();

	Timer timer;
	timer.start();
    printf("Rendering..");
	renderer->render();
	timer.stop();

    std::string t;
	print_time_breakdown(t, timer.get_time_in_mlsec());
    printf("%c[2K\r", 27);
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
	    std::string cam = context.scene.camera;

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
