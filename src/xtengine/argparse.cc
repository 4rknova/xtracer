#include <cstring>
#include <cstdio>
#include <nmath/mutil.h>
#include <xtcore/log.h>
#include "setup.h"
#include "argdefs.h"
#include "argparse.h"

Environment Environment::m_environment;

Environment::Environment()
	: m_aa(XTRACER_SETUP_DEFAULT_AA)
	, m_width(XTRACER_SETUP_DEFAULT_FB_WIDTH)
	, m_height(XTRACER_SETUP_DEFAULT_FB_HEIGHT)
	, m_threads(XTRACER_SETUP_DEFAULT_THREAD_COUNT)
	, m_max_rdepth(XTRACER_SETUP_DEFAULT_MAX_RDEPTH)
	, m_samples(XTRACER_SETUP_DEFAULT_SAMPLES)

//	, m_flag_gi(XTRACER_SETUP_DEFAULT_GI)
//	, m_flag_giviz(XTRACER_SETUP_DEFAULT_GIVIZ)
//	, m_photon_count(XTRACER_SETUP_DEFAULT_PHOTON_COUNT)
//	, m_photon_max_samples(XTRACER_SETUP_DEFAULT_PHOTON_SAMPLES)
//	, m_photon_max_sampling_radius(XTRACER_SETUP_DEFAULT_PHOTON_SRADIUS)
//	, m_photon_power_scaling(XTRACER_SETUP_DEFAULT_PHOTON_POWERSC)

{}

Environment::~Environment()
{}

void Environment::configure(params_t &params)
{
    params.threads = m_threads;
    params.samples = m_samples;
    params.ssaa    = m_aa;
    params.rdepth  = m_max_rdepth;
}

Environment &Environment::handle()
{
	return m_environment;
}

const char *Environment::renderer() const
{
    return m_renderer.c_str();
}

unsigned int Environment::width() const
{
	return m_width;
}

unsigned int Environment::height() const
{
	return m_height;
}

unsigned int Environment::threads() const
{
	return m_threads;
}

unsigned int Environment::max_rdepth() const
{
	return m_max_rdepth;
}

unsigned int Environment::samples() const
{
    return m_samples;
}

unsigned int Environment::aa() const
{
	return m_aa < 1 ? 1 : m_aa;
}

unsigned int Environment::photon_count() const
{
	return m_photon_count;
}

unsigned int Environment::photon_max_samples() const
{
	return m_photon_max_samples;
}

float Environment::photon_max_sampling_radius() const
{
	return m_photon_max_sampling_radius;
}

float Environment::photon_power_scaling() const
{
	return m_photon_power_scaling;
}

bool Environment::flag_gi() const
{
	return m_flag_gi;
}

bool Environment::flag_giviz() const
{
	return m_flag_giviz;
}

const char *Environment::outdir() const
{
	return m_outdir.c_str();
}

const char *Environment::active_camera_name() const
{
	return m_active_camera_name.c_str();
}

void Environment::modifier_push(std::string &modifier)
{
	m_modifiers.push_back(modifier);
}

std::list<std::string> Environment::modifiers()
{
    return m_modifiers;
}

bool Environment::modifier_pop(std::string &res)
{
	if (m_modifiers.empty()) {
		return false;
	}

	res = m_modifiers.back();
	m_modifiers.pop_back();

	return true;
}

unsigned int Environment::setup(int argc, char **argv)
{
	for (unsigned int i = 1; i < (unsigned int)argc; i++) {
		// Framebuffer resolution.
		if (!strcmp(argv[i], XTRACER_ARGDEFS_RESOLUTION)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%ux%u", &m_width, &m_height) < 2) {
				Log::handle().log_error("Invalid %s value. Should be <uint>x<uint>.", argv[i-1]);
				return 2;
			}

			if ((m_width == 0) || (m_height == 0)) {
				Log::handle().log_error("Invalid %s value.", argv[i-1]);
				return 2;
			}

		}
        else if (!strcmp(argv[i], XTRACER_ARGDEFS_RENDERER)) {
            ++i;

            if (!argv[i]) {
                Log::handle().log_error("No value was provided for %s", argv[i-1]);
                return 2;
            }

            m_renderer = argv[i];
        }
		// Maximum recursion depth
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_MAX_RDEPTH)) {
			++i;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%u", &m_max_rdepth) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}

			if (m_max_rdepth < 1) {
				Log::handle().log_error("Invalid %s value.", argv[i-1]);
				return 2;
			}
		}
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_SAMPLES)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%u", &m_samples) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}

			if (m_samples < 1) {
				Log::handle().log_error("Invalid %s value. Must be 1 or greater.", argv[i-1]);
				return 2;
			}
		}
		// Antialiasing
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_ANTIALIASING)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%u", &m_aa) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}

			if (m_aa < 2) {
				Log::handle().log_error("Invalid %s value. Should be 2 or greater.", argv[i-1]);
				return 2;
			}
		}
		// Output directory.
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_OUTDIR)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			m_outdir = argv[i];
		}
		// Camera id
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_ACTIVE_CAMERA)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			m_active_camera_name = argv[i];
		}
		// Threads
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_THREADS))	{
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%u", &m_threads) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}
		}
/*
		// G.I.
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_GI)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%u:%u:%fx%f", &m_photon_count, &m_photon_max_samples, &m_photon_max_sampling_radius, &m_photon_power_scaling) < 4) {
				Log::handle().log_error("Invalid %s value. Should be <uint>:<uint>:<float>x<float>.", argv[i-1]);
				return 2;
			}

			if ((int)m_photon_count < 1 || m_photon_max_samples < 1 || m_photon_max_sampling_radius <= 0.0) {
				Log::handle().log_error("Invalid %s value.", argv[i-1]);
				return 2;
			}

			m_flag_gi = true;
		}
		// G.I. photonmap visualization
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_GIVIZ)) {
			m_flag_giviz = true;
		}
*/
		// Modifier
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_MOD)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("Invalid argument: %s", argv[i]);
				return 2;
			}

			m_modifiers.push_back(argv[i]);
		}
		// Invalid argument
		else if (argv[i][0] == '-') {
			Log::handle().log_error("Invalid argument: %s", argv[i]);
			return 2;
		}
		// Scene file
		else {
			if (mScene.empty()) {
				mScene = argv[i];
			}
			else {
				Log::handle().log_error("Multiple scenes were given in input.");
				return 2;
			}
		}
	}
	if (mScene.empty()) {
		Log::handle().log_message("No scene was provided. Nothing to do..");
		return 1;
	}

	return 0;
}

const char *Environment::scene()
{
	return mScene.c_str();
}

void Environment::log_info() const
{
	int aspgcd = NMath::gcd(m_width, m_height);
	Log::handle().log_message(" Resolution   : %ix%i ", m_width, m_height);
    Log::handle().log_message(" Aspect Ratio : %i:%i ", m_width / aspgcd, m_height / aspgcd);
	Log::handle().log_message(" SSAA         : %i spp", m_aa * m_aa);
	Log::handle().log_message(" Recursion    : %i", m_max_rdepth);
	Log::handle().log_message(" Sampling     : %i", m_samples);

	if (m_flag_gi) {
		Log::handle().log_message("- Photon maps  : Total %i, Per pixel %i, Radius %f",
			m_photon_count, m_photon_max_samples, m_photon_max_sampling_radius);
	}
}
