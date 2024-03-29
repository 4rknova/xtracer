#include "context.h"

#define XTCORE_CONTEXT_DEFAULT_WIDTH   (500)
#define XTCORE_CONTEXT_DEFAULT_HEIGHT  (500)
#define XTCORE_CONTEXT_DEFAULT_TILESZ  (32)
#define XTCORE_CONTEXT_DEFAULT_THREADS (0)
#define XTCORE_CONTEXT_DEFAULT_SAMPLES (1)
#define XTCORE_CONTEXT_DEFAULT_AA      (1)
#define XTCORE_CONTEXT_DEFAULT_RDEPTH  (100)

using nimg::ColorRGBAf;

namespace xtcore {
    namespace render {

params_t::params_t()
    : width     (XTCORE_CONTEXT_DEFAULT_WIDTH)
    , height    (XTCORE_CONTEXT_DEFAULT_HEIGHT)
    , threads   (XTCORE_CONTEXT_DEFAULT_THREADS)
    , samples   (XTCORE_CONTEXT_DEFAULT_SAMPLES)
    , aa        (XTCORE_CONTEXT_DEFAULT_AA)
    , rdepth    (XTCORE_CONTEXT_DEFAULT_RDEPTH)
    , tile_size (XTCORE_CONTEXT_DEFAULT_TILESZ)
    , tile_order(xtcore::render::TILE_ORDER_RANDOM)
    , sample_distribution(xtcore::antialiasing::SAMPLE_DISTRIBUTION_GRID)
    , camera    (0)
{}

void context_t::init()
{
    tiles.clear();
    segment_framebuffer(tiles, params.width, params.height, params.tile_size);
}

context_t::context_t()
{}

void assemble(Pixmap &pixmap, const context_t &context)
{
    pixmap.init(context.params.width, context.params.height);
    ColorRGBAf col;

    for (size_t i = 0; i < context.tiles.size(); ++i) {
        const xtcore::render::tile_t *tile = &(context.tiles[i]);
        for (size_t y = tile->y0(); y < tile->y1(); ++y) {
            for (size_t x = tile->x0(); x < tile->x1(); ++x) {
                tile->read(x, y, col);
                pixmap.pixel(x, y) = col;
            }
        }
    }
}

void assemble(raygraph_t &raygraph, const context_t &context)
{
    raygraph.bundles.clear();

    for (size_t i = 0; i < context.tiles.size(); ++i) {
        const xtcore::render::tile_t *tile = &(context.tiles[i]);
        raygraph.bundles.push_back(&(tile->raygraph_bundle));
    }
}

    } /* namespace render */
} /* namespace xtcore */
