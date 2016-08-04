#ifndef XTRACER_CONTEXT_H_INCLUDED
#define XTRACER_CONTEXT_H_INCLUDED

#include "nimg/pixmap.h"
#include "scene.h"

namespace XT {
	namespace Render {

class Context
{
	public:
	Scene        *scene;
	NImg::Pixmap *framebuffer;

	private:
};

	}
}


#endif /* XTRACER_CONTEXT_H_INCLUDED */
