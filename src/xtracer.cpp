#include <string>
#include <sstream>
#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <ncf/util.hpp>
#include <nimg/framebuffer.hpp>
#include <nimg/ppm.hpp>
#include "renderer.hpp"
#include "scene.hpp"
#include "argparse.hpp"
#include "timer.hpp"
#include "timeutil.hpp"
#include "log.hpp"

using NCF::Util::to_string;
using NCF::Util::path_comp;

int main(int argc, char **argv)
{
	// parse the argument list.
	if (Environment::handle().setup(argc, argv)) {
		return 1;
	}

	// Process the scenes.
	unsigned int scene_count = Environment::handle().scene_count();
	unsigned int scene_index = 0;
	while (Environment::handle().scene_count())
	{
		// Create and initiate the framebuffer.
		Log::handle().log_message("Initiating the framebuffer..");
		Framebuffer fb;
			
		if (Environment::handle().flag_resume()) {
			if(NImg::IO::Import::ppm_raw(Environment::handle().resume_file(), fb)) {
				Log::handle().log_error("Failed to load %s", Environment::handle().resume_file());
				return 1;
			}
		}
		else {
			fb.init(Environment::handle().width(), Environment::handle().height());
		}

		// Log info.
		{
			int aspgcd = NMath::gcd(fb.width(), fb.height());
			Log::handle().log_message("- Output       : Resolution %ix%i, Aspect ratio %i:%i", 
				fb.width(), fb.height(), 
				fb.width() / aspgcd, fb.height() / aspgcd);
			Log::handle().log_message("- Antialiasing : Supersampling %ix%i, %i spp",
				Environment::handle().aa(), Environment::handle().aa(),
				Environment::handle().aa() * Environment::handle().aa());
			Log::handle().log_message("- Recursion    : %i", Environment::handle().max_rdepth());
			Log::handle().log_message("- Sampling     : DoF %i, Shading %i, Reflections %i",
				Environment::handle().samples_dof(), 
				Environment::handle().samples_light(), Environment::handle().samples_reflection());

			if (Environment::handle().flag_gi()) {
				Log::handle().log_message("- Photon maps  : Total %i, Per pixel %i, Radius %f", 
					Environment::handle().photon_count(), Environment::handle().photon_max_samples(),
					Environment::handle().photon_max_sampling_radius());
			}
		}
		
		// Export.
		if (Environment::handle().output() == XTRACER_OUTPUT_NUL) {
			Log::handle().log_warning("Benchmark mode selected.");
		}

		// Get the next scene.
		std::string scene_source;
		Environment::handle().scene_pop(scene_source);

		if (scene_count > 1) {
			Log::handle().log_message("Current task %i / %i", ++scene_index, scene_count);
		}

		// create and initialize the scene
		Scene scene;
		if (scene.load(scene_source.c_str()))
			continue;

		// Build the scene data
		scene.build();
		
		// set the camera
		scene.set_camera(Environment::handle().active_camera_name());

		// Create the renderer.
		Renderer renderer;
		
		// Create and initiate a timer.
		Timer timer;
		timer.start();

		// Render.
		renderer.render(fb, scene);

		// Timer stats.
		timer.stop();
		unsigned int days, hours, mins;
		float secs;

		convert_mlseconds(timer.get_time_in_mlsec(), days, hours, mins, secs);

		Log::handle().log_message("Total time:");

		if (days  > 0){
			Log::handle().set_append();
			Log::handle().log_message(" %i days,", days);
		}
		if (hours  > 0) {
			Log::handle().set_append();
			Log::handle().log_message(" %i hours,", hours);
		}
		if (mins  > 0) {
			Log::handle().set_append();
			Log::handle().log_message(" %i mins,", mins);
		}	
		Log::handle().set_append();
		Log::handle().log_message(" %f seconds.", secs);

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

				path_comp(scene_source, base, file, path_delim);
		
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
	Log::handle().log_message("All tasks completed."); 
	Log::handle().log_message("Shutting down.");

	return 0;
}
