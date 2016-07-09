#ifndef XTRACER_PHOTON_MAPPER_HPP_INCLUDED
#define XTRACER_PHOTON_MAPPER_HPP_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/ray.h>
#include <nmath/intinfo.h>
#include <nimg/color.hpp>
#include <nimg/pixmap.h>
#include <nplatform/timer.h>
#include <xtcore/scene.h>
#include <xtcore/renderer.h>
#include "photonmap.h"

using NImg::ColorRGBf;
using NImg::Pixmap;

class Renderer : public IRenderer
{
	public:
		Renderer();

		virtual void setup(Pixmap &fb, Scene &scene);
		virtual void render(void);

	private:
		// Pass functions.
		void pass_ptrace(Scene *scene);						// Rendering pass: Photon tracing.
		void pass_rtrace(Pixmap *fb, Scene *scene);			// Rendering pass: Ray tracing.

		// Trace functions.
		bool trace_photon(Scene *scene, const Ray &ray, const unsigned int depth,
						  const ColorRGBf power, unsigned int &map_capacity);

		ColorRGBf trace_ray(Scene *scene, const Ray &ray, const unsigned int depth,
							const scalar_t ior_src = 1.0, const scalar_t ior_dst = 1.0);

		// Shading.
		ColorRGBf shade(Scene *scene, const Ray &ray, const unsigned int depth,
						IntInfo &info, std::string &obj,
						const scalar_t ior_src = 1.0, const scalar_t ior_dst = 1.0);

		Pixmap *mFramebuffer;
		Scene *mScene;

		PhotonMap m_pm_global;
		PhotonMap m_pm_caustic;
};

#endif /* XTRACER_PHOTON_MAPPER_HPP_INCLUDED */
