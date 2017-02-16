#ifndef XTRACER_CONTEXT_H_INCLUDED
#define XTRACER_CONTEXT_H_INCLUDED

#include "nimg/pixmap.h"
#include "scene.h"

using nimg::Pixmap;

namespace xtracer {
	namespace render {

struct params_t
{
    size_t threads;   // Number of threads
    size_t samples;   // Number of samples
    size_t ssaa;      // Level of Screen Space Anti-Aliasing
    size_t rdepth;    // Maximum recursion depth
    size_t tile_size; // Tile size for framebuffers segmentation

    params_t();
};

struct context_t
{
	Scene    *scene;
	Pixmap   *framebuffer;
    params_t  params;

    context_t();
};

	} /* namespace render */
} /* namespace xtracer */

#endif /* XTRACER_CONTEXT_H_INCLUDED */
