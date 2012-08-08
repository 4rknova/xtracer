#include <cstring>
#include <cstdio>
#include "setup.h"
#include "argdefs.h"
#include "log.hpp"
#include "argparse.hpp"

Environment Environment::m_environment;

Environment::Environment()
	: m_width(XTRACER_SETUP_DEFAULT_FB_WIDTH),
	  m_height(XTRACER_SETUP_DEFAULT_FB_HEIGHT),
	  m_threads(XTRACER_SETUP_DEFAULT_THREAD_COUNT),
	  m_max_rdepth(XTRACER_SETUP_DEFAULT_MAX_RDEPTH),
	  m_samples_dof(XTRACER_SETUP_DEFAULT_DOF_SAMPLES),
	  m_samples_light(XTRACER_SETUP_DEFAULT_LIGHT_SAMPLES),
	  m_samples_reflec(XTRACER_SETUP_DEFAULT_REFLEC_SAMPLES),
	  m_aa(XTRACER_SETUP_DEFAULT_AA),
	  m_region_min_x(0),
	  m_region_min_y(0),
	  m_region_max_x(0),
	  m_region_max_y(0),
	  m_flag_resume(false),
	  m_output(XTRACER_OUTPUT_PPM)
{}

Environment::~Environment()
{}

Environment &Environment::handle()
{
	return m_environment;
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

unsigned int Environment::dof_samples() const
{
	return m_samples_dof;
}

unsigned int Environment::light_samples() const
{
	return m_samples_light;
}

unsigned int Environment::reflec_samples() const
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

bool Environment::flag_resume() const
{
	return m_flag_resume;
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

unsigned int Environment::scene_count()
{
	return m_scenes.size();
}

void Environment::scene_push(std::string &scene)
{
	m_scenes.push_back(scene);
}

bool Environment::scene_pop(std::string &res)
{
	if (m_scenes.empty())
		return false;

	res = m_scenes.back();
	m_scenes.pop_back();

	return true;
}

unsigned int Environment::setup(int argc, char **argv)
{
	// Display usage information.
	if (argc == 2 && !strcmp(argv[1], XTRACER_ARGDEFS_VERSION)) {
		Log::handle().log_message("%s", XTRACER_VERSION);
		return 1;
	}

	Log::handle().log_message("XTracer %s (C) 2010-2012 Papadopoulos Nikos", XTRACER_VERSION);
		
	if (argc == 2 && !strcmp(argv[1], XTRACER_ARGDEFS_HELP)) {
		Log::handle().log_message("Usage: %s [option]... scene_file...", argv[0]);
		Log::handle().log_message("For a complete list of the available options, refer to the man pages.");
		return 1;
	}

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

			if (sscanf(argv[i], "%u", &m_samples_dof) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}

			if (m_samples_dof < 2) {
				Log::handle().log_error("Invalid %s value. Must be 2 or greater.", argv[i-1]);
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

			if (sscanf(argv[i], "%u", &m_samples_light) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}
			
			if (m_samples_light < 1) {
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

			if (sscanf(argv[i], "%u", &m_samples_reflec) < 1) {
				Log::handle().log_error("Invalid %s value. Should be <uint>.", argv[i-1]);
				return 2;
			}
		
			if (m_samples_reflec < 1) {
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
		// Invalid argument
		else if (argv[i][0] == '-') {
			Log::handle().log_error("Invalid argument: %s", argv[i]);
			return 2;
		}
		// Scene file
		else {
			m_scenes.push_back(argv[i]);
		}
	}

	if (m_scenes.empty()) {
		Log::handle().log_message("No scenes were provided. Nothing to do..");
		return 1;
	}

	// Resume file feature cannot be used with multiple scenes.
	if (m_scenes.size() > 1 && !m_resume_file.empty()) {
		Log::handle().log_message("You cannot use the resume feature with multiple scenes. Ignoring..");
		m_resume_file.clear();
		m_flag_resume = false;
	}

	return 0;
}
