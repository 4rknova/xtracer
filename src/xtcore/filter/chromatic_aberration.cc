#include "chromatic_aberration.h"

#include <algorithm>
#include <cmath>

namespace xtcore {
    namespace filter {

namespace {

nimg::ColorRGBAf sample_clamped_bilinear(const Pixmap &map, float xf, float yf)
{
    const int w = (int)map.width();
    const int h = (int)map.height();
    if (w <= 0 || h <= 0) return nimg::ColorRGBAf();

    if (xf < 0.0f) xf = 0.0f;
    if (yf < 0.0f) yf = 0.0f;
    if (xf > (float)(w - 1)) xf = (float)(w - 1);
    if (yf > (float)(h - 1)) yf = (float)(h - 1);

    const int x0 = (int)std::floor(xf);
    const int y0 = (int)std::floor(yf);
    const int x1 = std::min(x0 + 1, w - 1);
    const int y1 = std::min(y0 + 1, h - 1);

    const float tx = xf - (float)x0;
    const float ty = yf - (float)y0;
    const float w00 = (1.0f - tx) * (1.0f - ty);
    const float w10 = tx * (1.0f - ty);
    const float w01 = (1.0f - tx) * ty;
    const float w11 = tx * ty;

    const nimg::ColorRGBAf c00 = map.pixel_ro((size_t)x0, (size_t)y0);
    const nimg::ColorRGBAf c10 = map.pixel_ro((size_t)x1, (size_t)y0);
    const nimg::ColorRGBAf c01 = map.pixel_ro((size_t)x0, (size_t)y1);
    const nimg::ColorRGBAf c11 = map.pixel_ro((size_t)x1, (size_t)y1);
    return c00 * w00 + c10 * w10 + c01 * w01 + c11 * w11;
}

} // namespace

ChromaticAberration::ChromaticAberration()
    : amount(1.5f)
    , center_x(0.5f)
    , center_y(0.5f)
    , falloff(1.0f)
{}

void ChromaticAberration::render(Pixmap *p)
{
    if (!p) return;
    const size_t w = p->width();
    const size_t h = p->height();
    if (w == 0 || h == 0 || amount <= 0.0f) return;

    Pixmap src = *p;
    const float nx_scale = (w > 1) ? (1.0f / (float)(w - 1)) : 0.0f;
    const float ny_scale = (h > 1) ? (1.0f / (float)(h - 1)) : 0.0f;
    const float norm_scale = 1.41421356237f; // sqrt(2)

    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            const float nx = nx_scale * (float)x;
            const float ny = ny_scale * (float)y;
            const float vx = nx - center_x;
            const float vy = ny - center_y;
            const float dist = std::sqrt(vx * vx + vy * vy);
            if (dist <= 1e-8f) continue;
            const float ux = vx / dist;
            const float uy = vy / dist;
            const float dist_norm = std::min(1.0f, dist * norm_scale);
            const float shift = amount * std::pow(dist_norm, falloff);

            const nimg::ColorRGBAf c_base = sample_clamped_bilinear(src, (float)x, (float)y);
            const nimg::ColorRGBAf c_r = sample_clamped_bilinear(src, (float)x + ux * shift, (float)y + uy * shift);
            const nimg::ColorRGBAf c_b = sample_clamped_bilinear(src, (float)x - ux * shift, (float)y - uy * shift);

            nimg::ColorRGBAf out = c_base;
            out.r(c_r.r());
            out.b(c_b.b());
            p->pixel(x, y) = out;
        }
    }
}

    } /* namespace filter */
} /* namespace xtcore */
