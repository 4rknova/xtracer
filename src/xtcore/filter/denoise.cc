#include "denoise.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace xtcore {
    namespace filter {

namespace {

float luminance(const nimg::ColorRGBAf &c)
{
    return 0.2126f * c.r() + 0.7152f * c.g() + 0.0722f * c.b();
}

float gaussian_weight(float distance2, float sigma)
{
    const float safe_sigma = std::max(0.001f, sigma);
    return std::exp(-0.5f * distance2 / (safe_sigma * safe_sigma));
}

} // namespace

Denoise::Denoise()
    : strength(0.65f)
    , radius(2.0f)
    , sigma(0.12f)
{}

void Denoise::render(Pixmap *p)
{
    if (!p) return;
    const size_t w = p->width();
    const size_t h = p->height();
    if (w == 0 || h == 0 || strength <= 0.0f) return;

    const int r = std::max(1, std::min(6, (int)std::round(radius)));
    const float blend = std::max(0.0f, std::min(1.0f, strength));
    const float sigma_range = std::max(0.001f, sigma);
    const float sigma_spatial = std::max(0.75f, (float)r * 0.75f);
    Pixmap src = *p;

    std::vector<float> spatial_weights((size_t)((r * 2 + 1) * (r * 2 + 1)), 0.0f);
    for (int ky = -r; ky <= r; ++ky) {
        for (int kx = -r; kx <= r; ++kx) {
            const float dist2 = (float)(kx * kx + ky * ky);
            spatial_weights[(size_t)((ky + r) * (r * 2 + 1) + (kx + r))] = gaussian_weight(dist2, sigma_spatial);
        }
    }

    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            const nimg::ColorRGBAf center = src.pixel_ro(x, y);
            const float norm = std::max(1.0f, luminance(center));

            float sum_r = 0.0f;
            float sum_g = 0.0f;
            float sum_b = 0.0f;
            float sum_w = 0.0f;

            for (int ky = -r; ky <= r; ++ky) {
                const int sy = std::max(0, std::min((int)h - 1, (int)y + ky));
                for (int kx = -r; kx <= r; ++kx) {
                    const int sx = std::max(0, std::min((int)w - 1, (int)x + kx));
                    const nimg::ColorRGBAf sample = src.pixel_ro((size_t)sx, (size_t)sy);
                    const float dr = (sample.r() - center.r()) / norm;
                    const float dg = (sample.g() - center.g()) / norm;
                    const float db = (sample.b() - center.b()) / norm;
                    const float range_dist2 = dr * dr + dg * dg + db * db;
                    const float spatial = spatial_weights[(size_t)((ky + r) * (r * 2 + 1) + (kx + r))];
                    const float weight = spatial * gaussian_weight(range_dist2, sigma_range);

                    sum_r += sample.r() * weight;
                    sum_g += sample.g() * weight;
                    sum_b += sample.b() * weight;
                    sum_w += weight;
                }
            }

            if (sum_w <= 0.0f) continue;

            const float inv_w = 1.0f / sum_w;
            const float filtered_r = sum_r * inv_w;
            const float filtered_g = sum_g * inv_w;
            const float filtered_b = sum_b * inv_w;

            nimg::ColorRGBAf out = center;
            out.r((center.r() * (1.0f - blend)) + (filtered_r * blend));
            out.g((center.g() * (1.0f - blend)) + (filtered_g * blend));
            out.b((center.b() * (1.0f - blend)) + (filtered_b * blend));
            p->pixel(x, y) = out;
        }
    }
}

    } /* namespace filter */
} /* namespace xtcore */
