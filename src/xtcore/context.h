#ifndef XTCORE_CONTEXT_H_INCLUDED
#define XTCORE_CONTEXT_H_INCLUDED

#include "strpool.h"
#include <memory>
#include "nimg/pixmap.h"
#include "scene.h"
#include "tile.h"
#include "aa.h"
#include "util/raygraph.h"

using nimg::Pixmap;
using nimg::ColorRGBf;
using xtcore::raygraph::raygraph_t;

namespace xtcore {
	namespace render {

struct params_t
{
    size_t width;     // Width of frame
    size_t height;    // Height of frame
    size_t threads;   // Number of threads
    size_t samples;   // Number of samples
    size_t aa;        // Level of Screen Space Anti-Aliasing
    size_t rdepth;    // Maximum recursion depth
    size_t tile_size; // Tile size for framebuffers segmentation

    xtcore::render::TILE_ORDER tile_order;
    xtcore::antialiasing::SAMPLE_DISTRIBUTION sample_distribution;

    HASH_UINT64 camera;

    params_t();
};

struct context_t
{
	Scene     scene;
	Tileset   tiles;
    params_t  params;
    std::unique_ptr<xtcore::asset::ICamera> transient_camera;

    /* init: Initialize context buffers
    **       Note that any change to params
    **       does not automatically modify
    **       those buffers and init should
    **       be called again
    */
    void init();
    xtcore::asset::ICamera *active_camera();
    const xtcore::asset::ICamera *active_camera() const;

    context_t();
};

/* export: Assembles the context tiles and copies the data
** to the respective container.
*/
void assemble(Pixmap &pixmap, const context_t &context);
void assemble(raygraph_t &raygraph, const context_t &context);

	} /* namespace render */
} /* namespace xtcore */

#endif /* XTCORE_CONTEXT_H_INCLUDED */
