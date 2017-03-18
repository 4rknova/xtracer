#ifndef XTRACER_DEPTH_H_INCLUDED
#define XTRACER_DEPTH_H_INCLUDED

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
        namespace depth {

class Renderer : public xtracer::render::IRenderer
{
	public:
	Renderer();

	virtual void setup(xtracer::render::context_t &context);
	virtual void render();

	private:

    virtual void render_depth();
    xtracer::render::context_t *m_context;
};

        } /* namespace depth */
    } /* namespace renderer */
} /* namespace xtracer */

#endif /* XTRACER_DEPTH_H_INCLUDED */
