#include <cstdio>
#include <algorithm>
#include <vector>
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
        case TILE_ORDER_SPIRAL_IN  : order_spiral(tiles, false); break;
        case TILE_ORDER_SPIRAL_OUT : order_spiral(tiles, true ); break;
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

void order_spiral(Tileset &tiles, bool outwards)
{
    if (tiles.empty()) return;

    std::vector<size_t> xs;
    std::vector<size_t> ys;
    xs.reserve(tiles.size());
    ys.reserve(tiles.size());
    for (const auto &t : tiles) {
        xs.push_back(t.x0());
        ys.push_back(t.y0());
    }
    std::sort(xs.begin(), xs.end());
    std::sort(ys.begin(), ys.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    ys.erase(std::unique(ys.begin(), ys.end()), ys.end());

    const int cols = static_cast<int>(xs.size());
    const int rows = static_cast<int>(ys.size());
    if (cols <= 0 || rows <= 0) return;

    std::vector<int> rank(static_cast<size_t>(rows * cols), -1);
    std::vector<std::pair<int, int>> order_coords;
    order_coords.reserve(static_cast<size_t>(rows * cols));

    int cx = cols / 2;
    int cy = rows / 2;
    int x = cx;
    int y = cy;

    auto push_if_valid = [&](int px, int py) {
        if (px < 0 || py < 0 || px >= cols || py >= rows) return;
        const size_t idx = static_cast<size_t>(py * cols + px);
        if (rank[idx] >= 0) return;
        rank[idx] = static_cast<int>(order_coords.size());
        order_coords.emplace_back(px, py);
    };

    push_if_valid(x, y);

    int step_len = 1;
    while (static_cast<int>(order_coords.size()) < rows * cols) {
        for (int i = 0; i < step_len; ++i) { x += 1; push_if_valid(x, y); }
        for (int i = 0; i < step_len; ++i) { y += 1; push_if_valid(x, y); }
        step_len++;
        for (int i = 0; i < step_len; ++i) { x -= 1; push_if_valid(x, y); }
        for (int i = 0; i < step_len; ++i) { y -= 1; push_if_valid(x, y); }
        step_len++;
    }

    std::sort(tiles.begin(), tiles.end(),
        [&](const tile_t &a, const tile_t &b) -> bool {
            const int ax = static_cast<int>(std::lower_bound(xs.begin(), xs.end(), a.x0()) - xs.begin());
            const int ay = static_cast<int>(std::lower_bound(ys.begin(), ys.end(), a.y0()) - ys.begin());
            const int bx = static_cast<int>(std::lower_bound(xs.begin(), xs.end(), b.x0()) - xs.begin());
            const int by = static_cast<int>(std::lower_bound(ys.begin(), ys.end(), b.y0()) - ys.begin());

            const int ar = rank[static_cast<size_t>(ay * cols + ax)];
            const int br = rank[static_cast<size_t>(by * cols + bx)];
            return outwards ? (ar < br) : (ar > br);
        }
    );
}

	} /* namespace render */
} /* namespace xtcore */
