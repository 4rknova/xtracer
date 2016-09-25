#ifndef XTRACER_ARGPARSE_H_INCLUDED
#define XTRACER_ARGPARSE_H_INCLUDED

#include <string>
#include <list>
#include <xtcore/context.h>

using xtracer::render::params_t;

class Environment
{
	public:
		static Environment &handle();

		unsigned int width() const;
		unsigned int height() const;

		unsigned int threads() const;

		unsigned int max_rdepth() const;

		unsigned int samples() const;

		unsigned int aa() const;

		bool flag_gi() const;
		bool flag_giviz() const;
		unsigned int photon_count() const;
		unsigned int photon_max_samples() const;
		float photon_max_sampling_radius() const;
		float photon_power_scaling() const;

		const char *outdir() const;
		const char *active_camera_name() const;

		unsigned int setup(int argc, char **argv);
        void         configure(params_t &params);

		void modifier_push(std::string &modifier);
		bool modifier_pop(std::string &res);

		const char *scene();

		void log_info() const;

	private:
		static Environment m_environment;

		Environment();
		~Environment();

		unsigned int m_aa;
		unsigned int m_width, m_height;

		unsigned int m_threads;

		unsigned int m_max_rdepth;

		unsigned int m_samples;

		bool m_flag_gi;
		bool m_flag_giviz;
		unsigned int m_photon_count;
		unsigned int m_photon_max_samples;
		float m_photon_max_sampling_radius;
		float m_photon_power_scaling;


		std::string m_outdir;
		std::string m_active_camera_name;
		std::list<std::string> m_modifiers;

		std::string mScene;
};

#endif /* XTRACER_ARGPARSE_H_INCLUDED */
