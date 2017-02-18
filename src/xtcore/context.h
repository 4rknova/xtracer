#ifndef XTRACER_CONTEXT_H_INCLUDED
#define XTRACER_CONTEXT_H_INCLUDED

#include "nimg/pixmap.h"
#include "scene.h"
#include "fblock.h"

namespace xtracer {
	namespace render {

struct params_t
{
    size_t width;     // Width of frame
    size_t height;    // Height of frame
    size_t threads;   // Number of threads
    size_t samples;   // Number of samples
    size_t ssaa;      // Level of Screen Space Anti-Aliasing
    size_t rdepth;    // Maximum recursion depth
    size_t tile_size; // Tile size for framebuffers segmentation

    params_t();
};

struct context_t
{
	Scene     *scene;
	Tileset   tiles;
    params_t  params;

    /* init: Initialize context buffers
    **       Note that any change to params
    **       does not automatically modify
    **       those buffers and init should
    **       be called again
    */
    void init();

    context_t();
};

/* export: Assembles the context tiles
**         and copies the data to the
**         pixmap.
*/
void assemble(nimg::Pixmap &pixmap, const context_t &context);

	} /* namespace render */
} /* namespace xtracer */

#endif /* XTRACER_CONTEXT_H_INCLUDED */
