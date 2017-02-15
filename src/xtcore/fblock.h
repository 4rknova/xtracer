#ifndef XTRACER_FBLOCK_H_INCLUDED
#define XTRACER_FBLOCK_H_INCLUDED

#include <vector>
#include <cstddef>

namespace xtracer {
    namespace render {

struct frame_block_t {
	size_t x0, y0, x1, y1;

	frame_block_t();
	frame_block_t(size_t x0, size_t y0, size_t x1, size_t y1);
};

/*
** Segments the framebuffer grid to tiles
** tiles     : the output tiles vector
** width     : the framebuffer width
** height    : the framebuffer height
** tile_size : the horizontal and vertical size of each tile
*/
void segment_framebuffer(std::vector<frame_block_t> &tiles, size_t width, size_t height, size_t tile_size);

    } /* namespace render */
} /* namespace xtracer */

#endif /* XTRACER_FBLOCK_H_INCLUDED */
