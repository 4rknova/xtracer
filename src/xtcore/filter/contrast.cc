#include "contrast.h"

#include <algorithm>

namespace xtcore {
    namespace filter {

Contrast::Contrast()
    : amount(1.0f)
    , pivot(0.5f)
{}

void Contrast::render(Pixmap *p)
{
    if (!p || amount == 1.0f) return;

    for (size_t y = 0; y < p->height(); ++y) {
        for (size_t x = 0; x < p->width(); ++x) {
            nimg::ColorRGBAf c = p->pixel_ro(x, y);
            c.r(std::max(0.0f, (c.r() - pivot) * amount + pivot));
            c.g(std::max(0.0f, (c.g() - pivot) * amount + pivot));
            c.b(std::max(0.0f, (c.b() - pivot) * amount + pivot));
            p->pixel(x, y) = c;
        }
    }
}

    } /* namespace filter */
} /* namespace xtcore */
