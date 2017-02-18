#include <cstring>
#include <cstdio>
#include <nmath/mutil.h>
#include <xtcore/log.h>
#include "setup.h"
#include "argdefs.h"
#include "argparse.h"

//	res = m_modifiers.back();
//	m_modifiers.pop_back();

#define IS_PARAM(x) (!strcmp(argv[i],x))

int setup(int argc, char **argv
                 , std::string            &renderer
                 , std::string            &outdir
                 , std::string            &scene
                 , std::list<std::string> &modifiers
                 , std::string            &camera
                 , params_t               &params)
{
	for (size_t i = 1; i < (size_t)argc; ++i) {
		if (IS_PARAM(XTRACER_ARGDEFS_RESOLUTION)) {
            ++i;
			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%lux%lu", &(params.width), &(params.height)) < 2) {
				Log::handle().log_error("Invalid %s value. Should be <uint>x<uint>.", argv[i-1]);
				return 2;
			}

			if ((params.width == 0) || (params.height == 0)) {
				Log::handle().log_error("Invalid %s value.", argv[i-1]);
				return 2;
			}

		}
        else if (IS_PARAM(XTRACER_ARGDEFS_RENDERER)) {
            ++i;

            if (!argv[i]) {
                Log::handle().log_error("No value was provided for %s", argv[i-1]);
                return 2;
            }

            renderer = argv[i];
        }
		else if (IS_PARAM(XTRACER_ARGDEFS_MAX_RDEPTH)) {
			++i;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%lu", &(params.rdepth)) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}

			if (params.rdepth < 1) {
				Log::handle().log_error("Invalid %s value.", argv[i-1]);
				return 2;
			}
		}
       	else if (IS_PARAM(XTRACER_ARGDEFS_SAMPLES)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%lu", &(params.samples)) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}

			if (params.samples < 1) {
				Log::handle().log_error("Invalid %s value. Must be 1 or greater.", argv[i-1]);
				return 2;
			}
		}
		else if (IS_PARAM(XTRACER_ARGDEFS_ANTIALIASING)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%lu", &(params.ssaa)) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}

			if (params.ssaa < 2) {
				Log::handle().log_error("Invalid %s value. Should be 2 or greater.", argv[i-1]);
				return 2;
			}
		}
		else if (IS_PARAM(XTRACER_ARGDEFS_OUTDIR)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			outdir = argv[i];
		}
		else if (IS_PARAM(XTRACER_ARGDEFS_ACTIVE_CAMERA)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			camera = argv[i];
		}
		else if (IS_PARAM(XTRACER_ARGDEFS_THREADS))	{
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%lu", &(params.threads)) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}
		}
		else if (IS_PARAM(XTRACER_ARGDEFS_MOD)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("Invalid argument: %s", argv[i]);
				return 2;
			}

			modifiers.push_back(argv[i]);
		}
		// Invalid argument
		else if (argv[i][0] == '-') {
			Log::handle().log_error("Invalid argument: %s", argv[i]);
			return 2;
		}
		// Scene file
		else {
			if (scene.empty()) {
				scene = argv[i];
			}
			else {
				Log::handle().log_error("Multiple scenes were given in input.");
				return 2;
			}
		}
	}
	if (scene.empty()) {
		Log::handle().log_message("No scene was provided. Nothing to do..");
		return 1;
	}

	return 0;
}
