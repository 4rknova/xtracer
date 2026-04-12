#include "sharpen.h"

#include <algorithm>
#include <cmath>

namespace xtcore {
    namespace filter {

namespace {

float luminance(const nimg::ColorRGBAf &c)
{
    return 0.2126f * c.r() + 0.7152f * c.g() + 0.0722f * c.b();
}

} // namespace

Sharpen::Sharpen()
    : amount(0.8f)
    , radius(1.0f)
    , threshold(0.02f)
{}

void Sharpen::render(Pixmap *p)
{
    if (!p) return;
    const size_t w = p->width();
    const size_t h = p->height();
    if (w == 0 || h == 0 || amount <= 0.0f) return;

    const int r = std::max(1, (int)std::round(radius));
    Pixmap src = *p;

    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            nimg::ColorRGBAf blur(0.0f, 0.0f, 0.0f, src.pixel_ro(x, y).a());
            float count = 0.0f;
            for (int ky = -r; ky <= r; ++ky) {
                const int sy = std::max(0, std::min((int)h - 1, (int)y + ky));
                for (int kx = -r; kx <= r; ++kx) {
                    const int sx = std::max(0, std::min((int)w - 1, (int)x + kx));
                    blur += src.pixel_ro((size_t)sx, (size_t)sy);
                    count += 1.0f;
                }
            }
            blur = blur * (1.0f / std::max(1.0f, count));

            const nimg::ColorRGBAf in = src.pixel_ro(x, y);
            const nimg::ColorRGBAf hi = in - blur;
            const float lum_delta = std::fabs(luminance(hi));
            if (lum_delta < threshold) continue;

            nimg::ColorRGBAf out = in + hi * amount;
            out.r(std::max(0.0f, out.r()));
            out.g(std::max(0.0f, out.g()));
            out.b(std::max(0.0f, out.b()));
            p->pixel(x, y) = out;
        }
    }
}

    } /* namespace filter */
} /* namespace xtcore */
