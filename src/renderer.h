#ifndef XTRACER_RENDERER_H_INCLUDED
#define XTRACER_RENDERER_H_INCLUDED

#include "pixmap.h"
#include "scene.h"

class Renderer
{
	public:
		virtual ~Renderer();

		virtual void setup(NImg::Pixmap &fb, Scene &scene) = 0;
		virtual void render(void) = 0;

};

#endif /* XTRACER_RENDERER_H_INCLUDED */
