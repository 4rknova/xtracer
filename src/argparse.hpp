#ifndef XTRACER_ARGPARSE_HPP_INCLUDED
#define XTRACER_ARGPARSE_HPP_INCLUDED

#include <string>
#include <list>
#include "outdrv.h"

class Environment
{
	public:
		static Environment &handle();

		unsigned int width() const;
		unsigned int height() const;
		
		unsigned int threads() const;
		
		unsigned int max_rdepth() const;
		
		unsigned int samples_dof() const;
		unsigned int samples_light() const;
		unsigned int samples_reflection() const;

		unsigned int aa() const;

		unsigned int region_min_x() const;
		unsigned int region_min_y() const;
		unsigned int region_max_x() const;
		unsigned int region_max_y() const;

		bool flag_gi() const;
		bool flag_giviz() const;
		unsigned int photon_count() const;
		unsigned int photon_max_samples() const;
		float photon_max_sampling_radius() const;
		float photon_power_scaling() const;
		
		unsigned int octree_max_depth();
		unsigned int octree_max_items_per_node();

		XTRACER_OUTPUT_TYPE output() const;

		bool flag_resume() const;
		const char *resume_file() const;
		const char *outdir() const;
		const char *active_camera_name() const;

		// RETURN CODES:
		//	0. Everything went well.
		//	1. Code for normal termination.
		//	*. Invalid parametre.
		unsigned int setup(int argc, char **argv);

		unsigned int scene_count();
		void scene_push(std::string &scene);
		bool scene_pop(std::string &res);

	private:
		static Environment m_environment;

		Environment();
		~Environment();

		unsigned int m_aa;
		unsigned int m_width, m_height;
		
		unsigned int m_threads;
		
		unsigned int m_max_rdepth;
		
		unsigned int m_samples_dof;
		unsigned int m_samples_light;
		unsigned int m_samples_reflec;
		
		unsigned int m_region_min_x;
		unsigned int m_region_min_y;
		unsigned int m_region_max_x;
		unsigned int m_region_max_y;

		bool m_flag_gi;
		bool m_flag_giviz;
		unsigned int m_photon_count;
		unsigned int m_photon_max_samples;
		float m_photon_max_sampling_radius;
		float m_photon_power_scaling;

		unsigned int m_octree_max_depth;
		unsigned int m_octree_max_items_per_node;

		XTRACER_OUTPUT_TYPE m_output;

		bool m_flag_resume;
		std::string m_resume_file;
		std::string m_outdir;
		std::string m_active_camera_name;

		std::list<std::string> m_scenes;
};

#endif /* XTRACER_ARGPARSE_HPP_INCLUDED */
