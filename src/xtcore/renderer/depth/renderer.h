#ifndef XTPLUGIN_RENDERER_DEPTH_H_INCLUDED
#define XTPLUGIN_RENDERER_DEPTH_H_INCLUDED

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

namespace xtcore {
    namespace renderer {
        namespace depth {

class Renderer : public xtcore::render::IRenderer
{
	public:
	Renderer();

	virtual void setup(xtcore::render::context_t &context);
	virtual void render();

	private:
    xtcore::render::context_t *m_context;
};

        } /* namespace depth */
    } /* namespace renderer */
} /* namespace xtcore */

#endif /* XTPLUGIN_RENDERER_DEPTH_H_INCLUDED */
