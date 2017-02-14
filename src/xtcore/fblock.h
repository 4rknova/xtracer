#ifndef XTRACER_FBLOCK_H_INCLUDED
#define XTRACER_FBLOCK_H_INCLUDED

#include <vector>
#include <cstddef>

namespace xtracer {
    namespace render {

struct frame_block_t {
	size_t x, y, width, height;

	frame_block_t();
	frame_block_t(size_t x, size_t y, size_t width, size_t height);

};

void segment_framebuffer(std::vector<frame_block_t> &tiles, size_t width, size_t height, size_t tile_size);

    } /* namespace render */
} /* namespace xtracer */

#endif /* XTRACER_FBLOCK_H_INCLUDED */
