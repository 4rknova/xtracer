
#include <iostream>
#include "fblock.h"

namespace xtracer {
	namespace render {

frame_block_t::frame_block_t()
	: x        (0)
	, y        (0)
	, width    (0)
	, height   (0)
{}

frame_block_t::frame_block_t(
               size_t x,
		  	   size_t y,
	  	       size_t width,
  			   size_t height)
{
	this->x        = x;
	this->y        = y;
	this->width    = width;
	this->height   = height;
}

void segment_framebuffer(std::vector<frame_block_t> &tiles, size_t width, size_t height, size_t tile_size)
{
    tiles.clear();

    const size_t dx = (width  % tile_size) > 0 ? 1 : 0;
    const size_t dy = (height % tile_size) > 0 ? 1 : 0;
    const size_t tx = width  / tile_size + dx;
    const size_t ty = height / tile_size + dy;

    for (size_t j = 0; j < ty; ++j) {
        for (size_t i = 0; i < tx; ++i) {
            const size_t x = i * tile_size;
            const size_t y = j * tile_size;
            frame_block_t block(x, y, x + tile_size, y + tile_size);
            tiles.push_back(block);
        }
    }
}

	} /* namespace render */
} /* namespace xtracer */
