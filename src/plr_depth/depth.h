#ifndef XTRACER_DEPTH_HPP_INCLUDED
#define XTRACER_DEPTH_HPP_INCLUDED

#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nmath/ray.h>
#include <nmath/intinfo.h>
#include <nimg/color.hpp>
#include <nimg/pixmap.h>
#include <nplatform/timer.h>
#include <xtcore/scene.h>
#include <xtcore/renderer.h>

using NImg::ColorRGBf;
using NImg::Pixmap;

class DRenderer : public IRenderer
{
	public:
	DRenderer();

	virtual void setup(Pixmap &fb, Scene &scene);
	virtual void render(void);

	private:
	Pixmap *mFramebuffer;
	Scene *mScene;
};

#endif /* XTRACER_DEPTH_HPP_INCLUDED */
