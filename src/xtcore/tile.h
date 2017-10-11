#ifndef XTCORE_TILE_H_INCLUDED
#define XTCORE_TILE_H_INCLUDED

#include <vector>
#include <cstddef>
#include <nimg/pixmap.h>
#include "aa_sample.h"

namespace xtcore {
    namespace render {

struct tile_t;  // Forward Declaration

struct tile_event_handler_t
{
	virtual void handle_event(tile_t *tile) = 0;
};

struct tile_t {
    size_t x0()     const;
    size_t x1()     const;
    size_t y0()     const;
    size_t y1()     const;
    size_t width()  const;
    size_t height() const;

    void setup_handler_on_init  (tile_event_handler_t *h);
    void setup_handler_on_done  (tile_event_handler_t *h);

    void write (size_t x, size_t y, const nimg::ColorRGBAf &col);
    void read  (size_t x, size_t y,       nimg::ColorRGBAf &col) const;

    // Event triggers
    void init();
    void update();
    void submit();

	tile_t(size_t x0, size_t y0, size_t x1, size_t y1);

    xtcore::antialiasing::sample_set_t samples;

    private:
    nimg::Pixmap m_data;
	size_t       m_x0;
    size_t       m_y0;
    size_t       m_x1;
    size_t       m_y1;

    tile_event_handler_t *m_on_init;
    tile_event_handler_t *m_on_done;
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


enum TILE_ORDER
{
      TILE_ORDER_UNCHANGED
    , TILE_ORDER_RANDOM
    , TILE_ORDER_RADIAL_IN
    , TILE_ORDER_RADIAL_OUT
};

void order(Tileset &tiles, TILE_ORDER order);
void order_random(Tileset &tiles);
void order_radial(Tileset &tiles, bool outwards = true);

    } /* namespace render */
} /* namespace xtcore */

#endif /* XTCORE_TILE_H_INCLUDED */
