#ifndef XTPLUGIN_RENDERER_PHOTONMAPPER_H_INCLUDED
#define XTPLUGIN_RENDERER_PHOTONMAPPER_H_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/ray.h>
#include <nmath/intinfo.h>
#include <nimg/color.h>
#include <nimg/pixmap.h>
#include <nplatform/timer.h>
#include <xtcore/scene.h>
#include <xtcore/renderer.h>

using nimg::ColorRGBf;
using nimg::Pixmap;
using NMath::Ray;
using NMath::IntInfo;
using NMath::scalar_t;

class Renderer : public xtcore::render::IRenderer
{
	public:
		Renderer();

		virtual void setup(xtcore::render::context_t &context);
		virtual void render();

	private:
		ColorRGBf trace_ray(const Ray &pray, const Ray &ray, const unsigned int depth, const scalar_t ior_src = 1.0002926, const scalar_t ior_dst = 1.0);
		ColorRGBf shade(const Ray &pray, const Ray &ray, const unsigned int depth, IntInfo &info, std::string &obj, const scalar_t ior_src = 1.0, const scalar_t ior_dst = 1.0);

        xtcore::render::context_t *m_context;
};

#endif /* XTPLUGIN_RENDERER_PHOTONMAPPER_H_INCLUDED */
