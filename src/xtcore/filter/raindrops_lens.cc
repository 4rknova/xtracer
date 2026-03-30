#include "raindrops_lens.h"

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

float hash01(unsigned int x, unsigned int y, unsigned int s)
{
    unsigned int h = x * 374761393u + y * 668265263u + s * 362437u + 2246822519u;
    h ^= (h >> 13);
    h *= 1274126177u;
    h ^= (h >> 16);
    return (float)(h & 0x00ffffffu) / 16777215.0f;
}

nimg::ColorRGBAf sample_clamped_bilinear(const Pixmap &map, float xf, float yf)
{
    const int w = (int)map.width();
    const int h = (int)map.height();
    if (w <= 0 || h <= 0) return nimg::ColorRGBAf();

    xf = std::max(0.0f, std::min((float)(w - 1), xf));
    yf = std::max(0.0f, std::min((float)(h - 1), yf));

    const int x0 = (int)std::floor(xf);
    const int y0 = (int)std::floor(yf);
    const int x1 = std::min(x0 + 1, w - 1);
    const int y1 = std::min(y0 + 1, h - 1);
    const float tx = xf - (float)x0;
    const float ty = yf - (float)y0;

    const nimg::ColorRGBAf c00 = map.pixel_ro((size_t)x0, (size_t)y0);
    const nimg::ColorRGBAf c10 = map.pixel_ro((size_t)x1, (size_t)y0);
    const nimg::ColorRGBAf c01 = map.pixel_ro((size_t)x0, (size_t)y1);
    const nimg::ColorRGBAf c11 = map.pixel_ro((size_t)x1, (size_t)y1);
    const float w00 = (1.0f - tx) * (1.0f - ty);
    const float w10 = tx * (1.0f - ty);
    const float w01 = (1.0f - tx) * ty;
    const float w11 = tx * ty;
    return c00 * w00 + c10 * w10 + c01 * w01 + c11 * w11;
}

} // namespace

RaindropsLens::RaindropsLens()
    : density(0.35f)
    , size(0.45f)
    , distortion(12.0f)
    , seed(1.0f)
{}

void RaindropsLens::render(Pixmap *p)
{
    if (!p) return;
    const size_t w = p->width();
    const size_t h = p->height();
    if (w == 0 || h == 0 || density <= 0.0f || distortion <= 0.0f) return;

    Pixmap src = *p;
    const float aspect = (h > 0) ? ((float)w / (float)h) : 1.0f;
    const int cells = std::max(6, (int)std::round(8.0f + clamp01(density) * 56.0f));
    const float cell = 1.0f / (float)cells;
    const float base_radius = cell * (0.16f + clamp01(size) * 0.42f);
    const float pixel_shift = distortion;
    const unsigned int seed_i = (unsigned int)std::max(0.0f, std::floor(seed));

    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            const float u = ((float)x + 0.5f) / (float)w;
            const float v = ((float)y + 0.5f) / (float)h;
            const int cx = (int)std::floor(u * (float)cells);
            const int cy = (int)std::floor(v * (float)cells);

            float best_k = 0.0f;
            float best_dx = 0.0f;
            float best_dy = 0.0f;
            float best_edge = 0.0f;

            for (int oy = -1; oy <= 1; ++oy) {
                for (int ox = -1; ox <= 1; ++ox) {
                    const int gcx = cx + ox;
                    const int gcy = cy + oy;
                    if (gcx < 0 || gcy < 0 || gcx >= cells || gcy >= cells) continue;

                    const float alive = hash01((unsigned int)gcx, (unsigned int)gcy, seed_i + 11u);
                    if (alive > clamp01(density)) continue;

                    const float jx = hash01((unsigned int)gcx, (unsigned int)gcy, seed_i + 101u);
                    const float jy = hash01((unsigned int)gcx, (unsigned int)gcy, seed_i + 173u);
                    const float rr = hash01((unsigned int)gcx, (unsigned int)gcy, seed_i + 239u);

                    const float du = ((float)gcx + (0.2f + 0.6f * jx)) * cell;
                    const float dv = ((float)gcy + (0.2f + 0.6f * jy)) * cell;
                    const float rad = base_radius * (0.75f + 0.5f * rr);

                    const float vx = (u - du) * aspect;
                    const float vy = v - dv;
                    const float dist = std::sqrt(vx * vx + vy * vy);
                    if (dist >= rad || dist <= 1e-6f) continue;

                    const float k = 1.0f - (dist / rad);
                    if (k <= best_k) continue;
                    best_k = k;
                    best_dx = vx / dist;
                    best_dy = vy / dist;
                    best_edge = clamp01((dist / rad - 0.68f) / 0.32f);
                }
            }

            if (best_k <= 0.0f) continue;

            const float shift = pixel_shift * best_k * best_k;
            const float sx = (float)x - best_dx * shift;
            const float sy = (float)y - best_dy * shift;
            nimg::ColorRGBAf col = sample_clamped_bilinear(src, sx, sy);

            const float rim = best_edge * best_edge * 0.18f;
            col.r(std::max(0.0f, col.r() + rim));
            col.g(std::max(0.0f, col.g() + rim));
            col.b(std::max(0.0f, col.b() + rim));
            p->pixel(x, y) = col;
        }
    }
}

    } /* namespace filter */
} /* namespace xtcore */
