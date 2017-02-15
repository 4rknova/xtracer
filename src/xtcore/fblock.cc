
#include <cstdio>
#include "fblock.h"

namespace xtracer {
	namespace render {

frame_block_t::frame_block_t()
	: x0(0)
	, y0(0)
	, x1(0)
	, y1(0)
{}

frame_block_t::frame_block_t(
               size_t x0
		  	 , size_t y0
	  	     , size_t x1
  			 , size_t y1)
{
	this->x0 = x0;
	this->y0 = y0;
	this->x1 = x1;
	this->y1 = y1;
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
            size_t x0 =  i * tile_size;
            size_t y0 =  j * tile_size;
            size_t x1 = x0 + tile_size - 1;
            size_t y1 = y0 + tile_size - 1;

            /* The last tiles on each row and column
            ** might overshoot the boundaries of the
            ** image so the coords need to be clipped.
            */
            if (x1 >  width) x1 = width;
            if (y1 > height) y1 = height;

            frame_block_t block(x0, y0, x1, y1);
            tiles.push_back(block);
        }
    }
}

	} /* namespace render */
} /* namespace xtracer */
