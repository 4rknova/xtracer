#ifndef XTRACER_RENDERER_HPP_INCLUDED
#define XTRACER_RENDERER_HPP_INCLUDED

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

namespace xtracer {
    namespace renderer {
        namespace stencil {

class Renderer : public xtracer::render::IRenderer
{
	public:
	Renderer();

	virtual void setup(xtracer::render::context_t &context);
	virtual void render();

	private:
    xtracer::render::context_t *m_context;
};

        } /* namespace stencil */
    } /* namespace renderer */
} /* namespace xtracer */

#endif /* XTRACER_RENDERER_HPP_INCLUDED */
