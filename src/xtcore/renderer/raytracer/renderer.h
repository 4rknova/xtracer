#ifndef XTCORE_RENDERER_PHOTONMAPPER_H_INCLUDED
#define XTCORE_RENDERER_PHOTONMAPPER_H_INCLUDED

#include <nplatform/timer.h>
#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nimg/color.h>
#include <nimg/pixmap.h>
#include <xtcore/scene.h>
#include <xtcore/renderer.h>
#include <xtcore/math/ray.h>
#include <xtcore/math/surface.h>

using nimg::ColorRGBf;
using nimg::Pixmap;
using NMath::scalar_t;
using NMath::Vector3f;

namespace xtcore {
    namespace renderer {
        namespace raytracer {

class Renderer : public xtcore::render::IRenderer
{
	public:
		Renderer();

		virtual void setup(xtcore::render::context_t &context);
		virtual void render();

	private:
		ColorRGBf eval(const xtcore::Ray &pray, const xtcore::Ray &ray, const unsigned int depth, const scalar_t ior_src = 1.0002926, const scalar_t ior_dst = 1.0);
		ColorRGBf shade(const xtcore::Ray &pray, const xtcore::Ray &ray, const unsigned int depth, xtcore::HitRecord &info, HASH_UINT64 &obj, const scalar_t ior_src = 1.0, const scalar_t ior_dst = 1.0);

        xtcore::render::context_t *m_context;
};

        } /* namespace raytracer */
    } /* namespace renderer */
} /* namespace xtcore */

#endif /* XTCORE_RENDERER_PHOTONMAPPER_H_INCLUDED */
