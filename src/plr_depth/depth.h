#ifndef XTRACER_DEPTH_HPP_INCLUDED
#define XTRACER_DEPTH_HPP_INCLUDED

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

class DRenderer : public xtracer::render::IRenderer
{
	public:
	DRenderer();

	virtual void setup(xtracer::render::context_t &context);
	virtual void render();

	private:

    virtual void render_depth();
    xtracer::render::context_t *m_context;
};

#endif /* XTRACER_DEPTH_HPP_INCLUDED */
