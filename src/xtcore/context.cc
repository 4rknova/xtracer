#include "context.h"

#define XTCORE_CONTEXT_DEFAULT_WIDTH  800
#define XTCORE_CONTEXT_DEFAULT_HEIGHT 800

namespace xtcore {
    namespace render {

params_t::params_t()
    : width(XTCORE_CONTEXT_DEFAULT_WIDTH)
    , height(XTCORE_CONTEXT_DEFAULT_HEIGHT)
    , threads(0)
    , samples(1)
    , aa(1)
    , rdepth(3)
    , tile_size(8)
{}

void context_t::init()
{
    tiles.clear();
    segment_framebuffer(tiles, params.width, params.height, params.tile_size);
}

context_t::context_t()
{}

void assemble(nimg::Pixmap &pixmap, const context_t &context)
{
    pixmap.init(context.params.width, context.params.height);
    nimg::ColorRGBAf col;

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

    } /* namespace render */
} /* namespace xtcore */
