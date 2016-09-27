#ifndef XTRACER_PHOTON_MAPPER_H_INCLUDED
#define XTRACER_PHOTON_MAPPER_H_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/ray.h>
#include <nmath/intinfo.h>
#include <nimg/color.h>
#include <nimg/pixmap.h>
#include <nplatform/timer.h>
#include <xtcore/scene.h>
#include <xtcore/renderer.h>
#include "photonmap.h"

using nimg::ColorRGBf;
using nimg::Pixmap;

class Renderer : public xtracer::render::IRenderer
{
	public:
		Renderer();

		virtual void setup(xtracer::render::context_t &context);
		virtual void render();

	private:
		void pass_ptrace(); // Rendering pass: Photon tracing.
		void pass_rtrace();	// Rendering pass: Ray tracing.

		bool      trace_photon (const Ray &ray, const unsigned int depth, const ColorRGBf power, unsigned int &map_capacity);
		ColorRGBf trace_ray    (const Ray &pray, const Ray &ray, const unsigned int depth, const scalar_t ior_src = 1.0002926, const scalar_t ior_dst = 1.0);
		ColorRGBf shade        (const Ray &pray, const Ray &ray, const unsigned int depth, IntInfo &info, std::string &obj, const scalar_t ior_src = 1.0, const scalar_t ior_dst = 1.0);

        xtracer::render::context_t *m_context;
		PhotonMap                   m_pm_global;
		PhotonMap                   m_pm_caustic;
};

#endif /* XTRACER_PHOTON_MAPPER_H_INCLUDED */
