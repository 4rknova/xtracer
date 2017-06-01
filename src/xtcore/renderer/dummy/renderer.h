#ifndef XTPLUGIN_RENDERER_DUMMY_H_INCLUDED
#define XTPLUGIN_RENDERER_DUMMY_H_INCLUDED

#include <xtcore/renderer.h>
#include <xtcore/context.h>

using nimg::ColorRGBf;
using nimg::Pixmap;

namespace xtcore {
    namespace renderer {
        namespace dummy {

class Renderer : public xtcore::render::IRenderer
{
	public:
	virtual void setup(xtcore::render::context_t &context);
	virtual void render();
};

        } /* namespace dummy */
    } /* namespace renderer */
} /* namespace xtcore */

#endif /* XTPLUGIN_RENDERER_DUMMY_H_INCLUDED */
