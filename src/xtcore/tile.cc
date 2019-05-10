#include <cstdio>
#include <algorithm>
#include "tile.h"

namespace xtcore {
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

void tile_t::setup_handler_on_init(tile_event_handler_t *h)
{
    this->m_on_init = h;
}

void tile_t::setup_handler_on_done(tile_event_handler_t *h)
{
    this->m_on_done = h;
}

void tile_t::write(size_t x, size_t y, const nimg::ColorRGBAf &col)
{
    m_data.pixel(x - x0(), y - y0()) = col;
}

void tile_t::read(size_t x, size_t y, nimg::ColorRGBAf &col) const
{
    col = m_data.pixel_ro(x - x0(), y - y0());
}

void tile_t::init()
{
    m_data.init(width(), height());
    if (m_on_init) m_on_init->handle_event(this);
    samples.clear();
}

void tile_t::submit()
{
    if (m_on_done) m_on_done->handle_event(this);
}

void tile_t::apply_filter(IFilter *filter)
{
    filter->render(&m_data);
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

void order(Tileset &tiles, TILE_ORDER order)
{
    switch (order) {
        case TILE_ORDER_SCANLINE   : order_scanline(tiles);      break;
        case TILE_ORDER_RANDOM     : order_random(tiles);        break;
        case TILE_ORDER_RADIAL_IN  : order_radial(tiles, false); break;
        case TILE_ORDER_RADIAL_OUT : order_radial(tiles, true ); break;
    }
}

void order_random(Tileset &tiles)
{
    std::random_shuffle(tiles.begin(), tiles.end());
}

void order_scanline(Tileset &tiles)
{
        auto it = tiles.begin();
        auto et = tiles.end();

        std::stable_sort(it, et, [](const tile_t &a, const tile_t &b) -> bool {
            return a.x0() < b.x0();
        });

        auto jt = tiles.begin();

        std::stable_sort(jt, et, [](const tile_t &a, const tile_t &b) -> bool {
            return a.y0() < b.y0();
        });
}

void order_radial(Tileset &tiles, bool outwards)
{
    auto it = tiles.begin();
    auto et = tiles.end();

    // Get the dimensions
    size_t w = 0, h = 0;
    for (; it != et; ++it) {
        size_t x = (*it).x1();
        size_t y = (*it).y1();
        if (w < x) w = x;
        if (h < y) h = y;
    }

    int dw = w / 2;
    int dh = h / 2;

    std::sort (tiles.begin(), tiles.end(),
        [outwards, dw, dh](const tile_t &a, const tile_t &b) -> bool {
            int ax = a.x0() + a.width()  / 2 - dw;
            int ay = a.y0() + a.height() / 2 - dh;
            int ar = ax * ax + ay * ay;

            int bx = b.x0() + b.width()  / 2 - dw;
            int by = b.y0() + b.height() / 2 - dh;
            int br = bx * bx + by * by;

            bool res = ar < br;

            return (outwards ? res : !res);
        }
    );
}

	} /* namespace render */
} /* namespace xtcore */
