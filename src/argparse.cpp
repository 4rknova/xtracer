#include <cstring>
#include <cstdio>
#include "setup.h"
#include "argdefs.h"
#include "log.hpp"
#include "mutil.h"
#include "argparse.hpp"

Environment Environment::m_environment;

Environment::Environment()
	: m_gui(false)
	, m_aa(XTRACER_SETUP_DEFAULT_AA)
	, m_width(XTRACER_SETUP_DEFAULT_FB_WIDTH)
	, m_height(XTRACER_SETUP_DEFAULT_FB_HEIGHT)

	, m_threads(XTRACER_SETUP_DEFAULT_THREAD_COUNT)

	, m_max_rdepth(XTRACER_SETUP_DEFAULT_MAX_RDEPTH)

	, m_samples_dof(XTRACER_SETUP_DEFAULT_DOF_SAMPLES)
	, m_samples_light(XTRACER_SETUP_DEFAULT_LIGHT_SAMPLES)
	, m_samples_reflec(XTRACER_SETUP_DEFAULT_REFLEC_SAMPLES)

	, m_region_min_x(0)
	, m_region_min_y(0)
	, m_region_max_x(0)
	, m_region_max_y(0)

	, m_flag_gi(XTRACER_SETUP_DEFAULT_GI)
	, m_flag_giviz(XTRACER_SETUP_DEFAULT_GIVIZ)
	, m_photon_count(XTRACER_SETUP_DEFAULT_PHOTON_COUNT)
	, m_photon_max_samples(XTRACER_SETUP_DEFAULT_PHOTON_SAMPLES)
	, m_photon_max_sampling_radius(XTRACER_SETUP_DEFAULT_PHOTON_SRADIUS)
	, m_photon_power_scaling(XTRACER_SETUP_DEFAULT_PHOTON_POWERSC)

	, m_octree_max_depth(XTRACER_SETUP_DEFAULT_OCTREE_MAX_DEPTH)
	, m_octree_max_items_per_node(XTRACER_SETUP_DEFAULT_OCTREE_MAX_IPNDE)

	, m_output(XTRACER_OUTPUT_PPM)
	, m_flag_resume(false)
{}

Environment::~Environment()
{}

Environment &Environment::handle()
{
	return m_environment;
}

bool Environment::gui() const
{
	return m_gui;
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

unsigned int Environment::samples_dof() const
{
	return m_samples_dof;
}

unsigned int Environment::samples_light() const
{
	return m_samples_light;
}

unsigned int Environment::samples_reflection() const
{
	return m_samples_reflec;
}

unsigned int Environment::aa() const
{
	return m_aa < 1 ? 1 : m_aa;
}

unsigned int Environment::region_min_x() const
{
	return m_region_min_x;
}

unsigned int Environment::region_min_y() const
{
	return m_region_min_y;
}

unsigned int Environment::region_max_x() const
{
	return m_region_max_x;
}

unsigned int Environment::region_max_y() const
{
	return m_region_max_y;
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

bool Environment::flag_resume() const
{
	return m_flag_resume;
}

unsigned int Environment::octree_max_depth()
{
	return m_octree_max_depth;
}

unsigned int Environment::octree_max_items_per_node()
{
	return m_octree_max_items_per_node;
}

const char *Environment::resume_file() const
{
	return m_resume_file.c_str();
}

XTRACER_OUTPUT_TYPE Environment::output() const
{
	return m_output;
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
	// Temporary variables to hold the sample configuration.
	unsigned int arg_s_mc	 = 0;
	unsigned int arg_s_light = 0;
	unsigned int arg_s_refl	 = 0;
	unsigned int arg_s_dof	 = 0;

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
		// GUI
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_GUI)) {
			m_gui = true;
		}
		// Maximum recursion depth
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_MAX_RDEPTH)) {
			i++;

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
		// DOF samples
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_SAMPLES_DOF)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%u", &arg_s_dof) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}

			if (arg_s_dof < 1) {
				Log::handle().log_error("Invalid %s value. Must be 1 or greater.", argv[i-1]);
				return 2;
			}
		}
		// Light samples
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_SAMPLES_LIGHT)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%u", &arg_s_light) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}

			if (arg_s_light < 1) {
				Log::handle().log_error("Invalid %s value. Must be 1 or greater.", argv[i-1]);
				return 2;
			}
		}
		// Reflection samples
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_SAMPLES_REFLEC)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%u", &arg_s_refl) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}

			if (arg_s_refl < 1) {
				Log::handle().log_error("Invalid %s value. Must be 1 or greater.", argv[i-1]);
				return 2;
			}
		}
		// Monte carlo samples
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_SAMPLES_GLOBAL)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%u", &arg_s_mc) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}

			if (arg_s_mc < 1) {
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
		// Output mode.
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_OUTDRV)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (!strcmp(argv[i], XTRACER_OUTPUT_STR_NUL)) {
				m_output = XTRACER_OUTPUT_NUL;
			}
			else if (!strcmp(argv[i], XTRACER_OUTPUT_STR_PPM)) {
				m_output = XTRACER_OUTPUT_PPM;
			}
			else {
				Log::handle().log_error("Invalid %s value.", argv[i-1]);
				return 2;
			}
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
		// Region.
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_REGION)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%ux%u:%ux%u", &m_region_min_x, &m_region_min_y,
											   &m_region_max_x, &m_region_max_y) < 4) {
				Log::handle().log_error("Invalid %s value. Should be <uint>x<uint>:<uint>x<uint>.", argv[i-1]);
				return 2;
			}

			if (    (m_region_min_x > m_region_max_x)
				 || (m_region_min_y > m_region_max_y)
				 || (m_region_min_x == m_region_max_x && m_region_min_y == m_region_max_y)) {
				Log::handle().log_error("Invalid %s value.", argv[i-1]);
				return 2;
			}
		}
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
		// Octree max depth.
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_OCTREE_MAX_DEPTH)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%u", &m_octree_max_depth) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}

			if (m_octree_max_depth < 1) {
				 Log::handle().log_error("Invalid %s value.", argv[i-1]);
				 return 2;
			}
		}
		// Octre max items per node.
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_OCTREE_MAX_IPN)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			if (sscanf(argv[i], "%u", &m_octree_max_items_per_node) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}

			if (m_octree_max_items_per_node < 1) {
				Log::handle().log_error("Invalid %s value.", argv[i-1]);
				return 2;
			}
		}
		// Resume file.
		else if (!strcmp(argv[i], XTRACER_ARGDEFS_RESUMEFILE)) {
			i++;

			if (!argv[i]) {
				Log::handle().log_error("No value was provided for %s", argv[i-1]);
				return 2;
			}

			m_resume_file = argv[i];
			m_flag_resume = true;
		}
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

	// Setup the sampling environment
	m_samples_dof = (arg_s_dof > 0 ? arg_s_dof : (arg_s_mc > 0 ? arg_s_mc : 1));
	m_samples_light = (arg_s_light > 0 ? arg_s_light : (arg_s_mc > 0 ? arg_s_mc : 1));
	m_samples_reflec = (arg_s_refl > 0 ? arg_s_refl : (arg_s_mc > 0 ? arg_s_mc : 1));

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
	Log::handle().log_message("- Output       : Resolution %ix%i, Aspect ratio %i:%i",
		m_width, m_height, m_width / aspgcd, m_height / aspgcd);
	Log::handle().log_message("- Antialiasing : Supersampling %ix%i, %i spp",
		m_aa, m_aa, m_aa * m_aa);
	Log::handle().log_message("- Recursion    : %i", m_max_rdepth);
	Log::handle().log_message("- Sampling     : DoF %i, Shading %i, Reflections %i",
		m_samples_dof, m_samples_light, m_samples_reflec);

	if (m_flag_gi) {
		Log::handle().log_message("- Photon maps  : Total %i, Per pixel %i, Radius %f",
			m_photon_count, m_photon_max_samples, m_photon_max_sampling_radius);
	}
}
