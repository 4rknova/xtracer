#include "film_grain.h"

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

float luminance(const nimg::ColorRGBAf &c)
{
    return 0.2126f * c.r() + 0.7152f * c.g() + 0.0722f * c.b();
}

float hash_noise_signed(int x, int y, int seed)
{
    unsigned int h = (unsigned int)x * 374761393u
                   + (unsigned int)y * 668265263u
                   + (unsigned int)seed * 362437u
                   + 2246822519u;
    h ^= (h >> 13);
    h *= 1274126177u;
    h ^= (h >> 16);
    const float n01 = (float)(h & 0x00ffffffu) / 16777215.0f;
    return n01 * 2.0f - 1.0f;
}

} // namespace

FilmGrain::FilmGrain()
    : amount(0.06f)
    , size(1.0f)
    , seed(1.0f)
    , luma_weighted(1.0f)
{}

void FilmGrain::render(Pixmap *p)
{
    if (!p) return;
    const size_t w = p->width();
    const size_t h = p->height();
    if (w == 0 || h == 0 || amount <= 0.0f) return;

    const float cell = std::max(1.0f, size);
    const int seed_i = (int)seed;
    const bool weighted = luma_weighted >= 0.5f;
    Pixmap src = *p;

    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            const int gx = (int)std::floor((float)x / cell);
            const int gy = (int)std::floor((float)y / cell);
            const float n = hash_noise_signed(gx, gy, seed_i);
            const nimg::ColorRGBAf in = src.pixel_ro(x, y);
            float scale = 1.0f;
            if (weighted) {
                const float l = clamp01(luminance(in));
                scale = 0.35f + 0.65f * (1.0f - l);
            }
            const float delta = n * amount * scale;

            nimg::ColorRGBAf out = in;
            out.r(std::max(0.0f, in.r() + delta));
            out.g(std::max(0.0f, in.g() + delta));
            out.b(std::max(0.0f, in.b() + delta));
            p->pixel(x, y) = out;
        }
    }
}

    } /* namespace filter */
} /* namespace xtcore */
