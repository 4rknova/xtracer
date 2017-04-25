#include <cstdio>
#include "tile.h"

namespace xtracer {
	namespace render {

tile_t::tile_t(
               size_t x0
		  	 , size_t y0
	  	     , size_t x1
  			 , size_t y1)
{
	this->m_x0 = x0;
	this->m_y0 = y0;
	this->m_x1 = x1;
	this->m_y1 = y1;

    this->m_on_init = 0;
    this->m_on_done = 0;
}

size_t tile_t::x0()     const { return m_x0; }
size_t tile_t::x1()     const { return m_x1; }
size_t tile_t::y0()     const { return m_y0; }
size_t tile_t::y1()     const { return m_y1; }
size_t tile_t::width()  const { return m_x1 - m_x0; }
size_t tile_t::height() const { return m_y1 - m_y0; }

void tile_t::setup(callback on_init, callback on_done)
{
    this->m_on_init = on_init;
    this->m_on_done = on_done;
}

void tile_t::write(size_t x, size_t y, const nimg::ColorRGBf &col)
{
    m_data.pixel(x - x0(), y - y0()) = col;
}

void tile_t::read(size_t x, size_t y, nimg::ColorRGBf &col) const
{
    col = m_data.pixel_ro(x - x0(), y - y0());
}

void tile_t::init()
{
    m_data.init(width(), height());
    if (m_on_init) m_on_init(this);
}

void tile_t::submit()
{
    if (m_on_done) m_on_done(this);
}

void segment_framebuffer(Tileset &tiles, size_t width, size_t height, size_t tile_size)
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
            size_t x1 = x0 + tile_size;
            size_t y1 = y0 + tile_size;

            /* The last tiles on each row and column
            ** might overshoot the boundaries of the
            ** image so the coords need to be clipped.
            */
            if (x1 >  width) x1 = width;
            if (y1 > height) y1 = height;

            tile_t tile(x0, y0, x1, y1);
            tiles.push_back(tile);
        }
    }
}

	} /* namespace render */
} /* namespace xtracer */
