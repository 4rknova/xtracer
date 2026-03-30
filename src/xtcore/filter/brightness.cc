#include "brightness.h"

#include <algorithm>

namespace xtcore {
    namespace filter {

Brightness::Brightness()
    : amount(0.0f)
{}

void Brightness::render(Pixmap *p)
{
    if (!p || amount == 0.0f) return;

    for (size_t y = 0; y < p->height(); ++y) {
        for (size_t x = 0; x < p->width(); ++x) {
            nimg::ColorRGBAf c = p->pixel_ro(x, y);
            c.r(std::max(0.0f, c.r() + amount));
            c.g(std::max(0.0f, c.g() + amount));
            c.b(std::max(0.0f, c.b() + amount));
            p->pixel(x, y) = c;
        }
    }
}

    } /* namespace filter */
} /* namespace xtcore */
