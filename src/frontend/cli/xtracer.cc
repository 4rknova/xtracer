#include <string>
#include <mutex>

#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nimg/pixmap.h>
#include <nimg/img.h>
#include <ncf/util.h>
#include <xtcore/log.h>
#include <xtcore/timeutil.h>
#include <xtcore/parseutil.h>
#include <xtcore/integrator.h>
#include <xtcore/macro.h>
#include "argparse.h"

#define PROGRESS_BAR_LENGTH (15)

#define ARG_CLI_VERSION "version" /* Display version and exit */

#define RENDERER(x)   (!strcmp(integrator_name.c_str(), x))
#define ARGUMENT(i,x) (!strcmp(argv[i], x))

static std::mutex mut;

size_t workers   = 0;
size_t completed = 0;
size_t total     = 0;


struct ws_handler_init_t : public xtcore::render::tile_event_handler_t
{
    void handle_event(xtcore::render::tile_t *tile)
    {
        UNUSED(tile)
        mut.lock();
        ++workers;
        mut.unlock();
    }
};

struct ws_handler_done_t : public xtcore::render::tile_event_handler_t
{
    void handle_event(xtcore::render::tile_t *tile)
    {
       UNUSED(tile)
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
		xtcore::Log::handle().post_message("%s", xtcore::get_version());
		return 1;
	}

	xtcore::Log::handle().post_message("xtracer %s (C) 2010 Nikos Papadopoulos", xtcore::get_version());

    std::string integrator_name, outdir, scene_path;
    HASH_UINT64 camera;
    xtcore::render::params_t params;
    std::list<std::string> modifiers;

	if (setup(argc, argv
            , integrator_name
            , outdir
            , scene_path
            , modifiers
            , camera
            , params
    )) return 1;

    xtcore::init();
	xtcore::render::context_t context;
	if (!xtcore::io::scn::load(&(context.scene), scene_path.c_str(), &modifiers)) {
        if (context.scene.m_cameras.size() == 0) {
            xtcore::Log::handle().post_error("no cameras found");
            return 2;
        }
        context.params.camera = camera;
    } else return 1;

	xtcore::render::IIntegrator *integrator = NULL;
/*
    if      (RENDERER("depth"     )) integrator = new xtcore::integrator::depth::Integrator();
    else if (RENDERER("stencil"   )) integrator = new xtcore::integrator::stencil::Integrator();
    else if (RENDERER("normal"    )) integrator = new xtcore::integrator::normal::Integrator();
    else if (RENDERER("uv"        )) integrator = new xtcore::integrator::uv::Integrator();
    else if (RENDERER("raytracer" )) integrator = new xtcore::integrator::raytracer::Integrator();
    else if (RENDERER("pathtracer")) integrator = new xtcore::integrator::pathtracer::Integrator();
    else return 0;
*/
    context.params = params;
    context.init();

    xtcore::render::Tileset::iterator it = context.tiles.begin();
    xtcore::render::Tileset::iterator et = context.tiles.end();
    for (; it != et; ++it) {
        (*it).setup_handler_on_init(&handler_init);
        (*it).setup_handler_on_done(&handler_done);
    }

    integrator->setup(context);
    total =  context.tiles.size();

	Timer timer;
	timer.start();
    printf("Rendering..");
	integrator->render();
	timer.stop();

    std::string t;
	print_time_breakdown(t, timer.get_time_in_mlsec());
    printf("%c[2K\r", 27);
    xtcore::Log::handle().post_message("Total time: %s", t.c_str());

    // Export the frame
    {
	    std::string file, base, extension, filename, random_token;
		#ifdef _WIN32
    		const char path_delim = '\\';
	    #else
		    const char path_delim = '/';
		#endif /* _WIN32 */

    	ncf::util::path_comp(scene_path, base, file, path_delim);
    	ncf::util::path_comp(file, filename, extension, '.');

    	if (outdir[outdir.length()-1] != path_delim && !outdir.empty()) {
			outdir.append(1, path_delim);
		}

	    file = outdir + filename + "_" + integrator_name + ".png";
        nimg::Pixmap fb;
        xtcore::render::assemble(fb, context);
		xtcore::Log::handle().post_message("Exporting to %s..", file.c_str());
		int res = nimg::io::save::png(file.c_str(), fb);
        if (res) xtcore::Log::handle().post_error("Failed to export image file");
    }

	delete integrator;

    xtcore::deinit();

    return 0;
}
