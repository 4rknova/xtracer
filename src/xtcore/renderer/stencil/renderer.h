#ifndef XTCORE_RENDERER_STENCIL_H_INCLUDED
#define XTCORE_RENDERER_STENCIL_H_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nimg/color.h>
#include <nimg/pixmap.h>
#include <nplatform/timer.h>
#include <xtcore/scene.h>
#include <xtcore/renderer.h>
#include "math/ray.h"
#include "math/hitrecord.h"

using nimg::ColorRGBf;
using nimg::Pixmap;

namespace xtcore {
    namespace renderer {
        namespace stencil {

class Renderer : public xtcore::render::IRenderer
{
	public:
	Renderer();

	virtual void setup(xtcore::render::context_t &context);
	virtual void render();

	private:
    xtcore::render::context_t *m_context;
};

        } /* namespace stencil */
    } /* namespace renderer */
} /* namespace xtcore */

#endif /* XTCORE_RENDERER_STENCIL_H_INCLUDED */
