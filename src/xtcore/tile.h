#ifndef XTRACER_TILE_H_INCLUDED
#define XTRACER_TILE_H_INCLUDED

#include <vector>
#include <cstddef>
#include <nimg/pixmap.h>

namespace xtracer {
    namespace render {

typedef int (*callback) (void);

struct tile_t {

    size_t x0()     const;
    size_t x1()     const;
    size_t y0()     const;
    size_t y1()     const;
    size_t width()  const;
    size_t height() const;

    void setup(callback on_init, callback on_done);
    void write(size_t x, size_t y, const nimg::ColorRGBf &col);
    void read(size_t x, size_t y, nimg::ColorRGBf &col) const;
    void init();
    void submit();

	tile_t(size_t x0, size_t y0, size_t x1, size_t y1);

    private:
    nimg::Pixmap m_data;
	size_t       m_x0;
    size_t       m_y0;
    size_t       m_x1;
    size_t       m_y1;
    callback     m_on_init;
    callback     m_on_done;
};

typedef std::vector<tile_t> Tileset;

/*
** Segments the framebuffer grid to tiles
** tiles     : the output tiles vector
** width     : the framebuffer width
** height    : the framebuffer height
** tile_size : the horizontal and vertical size of each tile
*/
void segment_framebuffer(Tileset &tiles, size_t width, size_t height, size_t tile_size);

    } /* namespace render */
} /* namespace xtracer */

#endif /* XTRACER_TILE_H_INCLUDED */