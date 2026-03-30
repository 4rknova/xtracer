#include "vignette.h"

#include <algorithm>
#include <cmath>

namespace xtcore {
    namespace filter {

namespace {

float clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

} // namespace

Vignette::Vignette()
    : strength(0.35f)
    , radius(0.5f)
    , softness(0.35f)
    , center_x(0.5f)
    , center_y(0.5f)
{}

void Vignette::render(Pixmap *p)
{
    if (!p) return;
    const size_t w = p->width();
    const size_t h = p->height();
    if (w == 0 || h == 0 || strength <= 0.0f) return;

    const float nx_scale = (w > 1) ? (1.0f / (float)(w - 1)) : 0.0f;
    const float ny_scale = (h > 1) ? (1.0f / (float)(h - 1)) : 0.0f;
    const float norm_scale = 1.41421356237f; // sqrt(2)
    const float edge0 = std::max(0.0f, std::min(1.0f, radius));
    const float edge1 = std::max(edge0 + 1e-6f, std::min(1.0f, edge0 + softness));

    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            const float nx = nx_scale * (float)x;
            const float ny = ny_scale * (float)y;
            const float vx = nx - center_x;
            const float vy = ny - center_y;
            const float dist_norm = std::min(1.0f, std::sqrt(vx * vx + vy * vy) * norm_scale);
            const float t = clamp01((dist_norm - edge0) / (edge1 - edge0));
            const float darken = 1.0f - strength * t;

            nimg::ColorRGBAf c = p->pixel_ro(x, y);
            c.r(std::max(0.0f, c.r() * darken));
            c.g(std::max(0.0f, c.g() * darken));
            c.b(std::max(0.0f, c.b() * darken));
            p->pixel(x, y) = c;
        }
    }
}

    } /* namespace filter */
} /* namespace xtcore */
