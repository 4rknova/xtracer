#ifndef XTRACER_CONTEXT_H_INCLUDED
#define XTRACER_CONTEXT_H_INCLUDED

#include "nimg/pixmap.h"
#include "scene.h"

namespace xtracer {
	namespace render {

struct context_t
{
	public:
	Scene        *scene;
	NImg::Pixmap *framebuffer;

    size_t        threads;

    size_t        samples_ssaa;
    size_t        samples_reflection;
    size_t        samples_shadow;
    size_t        samples_dof;

    context_t();
};

	} /* namespace render */
} /* namespace xtracer */

#endif /* XTRACER_CONTEXT_H_INCLUDED */
