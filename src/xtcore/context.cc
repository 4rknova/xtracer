#include "context.h"

#define XTRACER_CONTEXT_DEFAULT_WIDTH  800
#define XTRACER_CONTEXT_DEFAULT_HEIGHT 800


namespace xtracer {
    namespace render {

params_t::params_t()
    : width(XTRACER_CONTEXT_DEFAULT_WIDTH)
    , height(XTRACER_CONTEXT_DEFAULT_HEIGHT)
    , threads(0)
    , samples(1)
    , ssaa(1)
    , rdepth(3)
    , tile_size(64)
{}

void context_t::init()
{
    segment_framebuffer(tiles, params.width, params.height, params.tile_size);
}

context_t::context_t()
    : scene(0)
{}

void assemble(nimg::Pixmap &pixmap, const context_t &context)
{
    pixmap.init(context.params.width, context.params.height);
    nimg::ColorRGBf col;

    for (size_t i = 0; i < context.tiles.size(); ++i) {
        const xtracer::render::tile_t *tile = &(context.tiles[i]);
        for (size_t y = tile->y0(); y <= tile->y1(); ++y) {
            for (size_t x = tile->x0(); x <= tile->x1(); ++x) {
                tile->read(x, y, col);
                pixmap.pixel(x, y) = col;
            }
        }
    }
}

    } /* namespace render */
} /* namespace xtracer */
