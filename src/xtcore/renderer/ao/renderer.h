#ifndef XTCORE_RENDERER_AO_H_INCLUDED
#define XTCORE_RENDERER_AO_H_INCLUDED

#include <nplatform/timer.h>

#include <nmath/precision.h>
#include <nmath/vector.h>

#include <nimg/color.h>
#include <nimg/pixmap.h>

#include <xtcore/math/hitrecord.h>
#include <xtcore/math/ray.h>
#include <xtcore/scene.h>
#include <xtcore/renderer.h>

using nimg::ColorRGBf;
using nimg::Pixmap;

namespace xtcore {
    namespace renderer {
        namespace ao {

class Renderer : public xtcore::render::IRenderer
{
	public:
	Renderer();

	virtual void setup(xtcore::render::context_t &context);
	virtual void render();

	private:
    nimg::ColorRGBf eval(size_t depth, const xtcore::Ray &ray, xtcore::HitRecord &hit);

    xtcore::render::context_t *m_context;
};

        } /* namespace ao */
    } /* namespace renderer */
} /* namespace xtcore */

#endif /* XTCORE_RENDERER_AO_H_INCLUDED */
