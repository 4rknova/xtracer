#include "fxaa.h"

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

FXAA::FXAA()
    : subpix(0.75f)
    , edge_threshold(0.125f)
    , edge_threshold_min(0.0312f)
{}

void FXAA::render(Pixmap *p)
{
    if (!p) return;
    const size_t w = p->width();
    const size_t h = p->height();
    if (w < 2 || h < 2) return;

    const float subpix_amount = clamp01(subpix);
    const float edge_thresh = std::max(0.001f, edge_threshold);
    const float edge_thresh_min = std::max(0.0001f, edge_threshold_min);
    const float span_max = 8.0f;
    Pixmap src = *p;

    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            const float xf = (float)x;
            const float yf = (float)y;

            const nimg::ColorRGBAf rgb_m  = sample_clamped_bilinear(src, xf, yf);
            const nimg::ColorRGBAf rgb_nw = sample_clamped_bilinear(src, xf - 1.0f, yf - 1.0f);
            const nimg::ColorRGBAf rgb_ne = sample_clamped_bilinear(src, xf + 1.0f, yf - 1.0f);
            const nimg::ColorRGBAf rgb_sw = sample_clamped_bilinear(src, xf - 1.0f, yf + 1.0f);
            const nimg::ColorRGBAf rgb_se = sample_clamped_bilinear(src, xf + 1.0f, yf + 1.0f);
            const nimg::ColorRGBAf rgb_n  = sample_clamped_bilinear(src, xf, yf - 1.0f);
            const nimg::ColorRGBAf rgb_s  = sample_clamped_bilinear(src, xf, yf + 1.0f);
            const nimg::ColorRGBAf rgb_w  = sample_clamped_bilinear(src, xf - 1.0f, yf);
            const nimg::ColorRGBAf rgb_e  = sample_clamped_bilinear(src, xf + 1.0f, yf);

            const float luma_m = luminance(rgb_m);
            const float luma_nw = luminance(rgb_nw);
            const float luma_ne = luminance(rgb_ne);
            const float luma_sw = luminance(rgb_sw);
            const float luma_se = luminance(rgb_se);
            const float luma_n = luminance(rgb_n);
            const float luma_s = luminance(rgb_s);
            const float luma_w = luminance(rgb_w);
            const float luma_e = luminance(rgb_e);

            const float luma_min = std::min(luma_m, std::min(std::min(luma_nw, luma_ne), std::min(luma_sw, luma_se)));
            const float luma_max = std::max(luma_m, std::max(std::max(luma_nw, luma_ne), std::max(luma_sw, luma_se)));
            const float luma_range = luma_max - luma_min;
            const float threshold = std::max(edge_thresh_min, luma_max * edge_thresh);
            if (luma_range < threshold) continue;

            float dir_x = -((luma_nw + luma_ne) - (luma_sw + luma_se));
            float dir_y =  ((luma_nw + luma_sw) - (luma_ne + luma_se));

            const float dir_reduce = std::max((luma_n + luma_s + luma_w + luma_e) * (0.25f * 0.25f), 1e-4f);
            const float rcp_dir_min = 1.0f / (std::min(std::fabs(dir_x), std::fabs(dir_y)) + dir_reduce);
            dir_x = std::max(-span_max, std::min(span_max, dir_x * rcp_dir_min));
            dir_y = std::max(-span_max, std::min(span_max, dir_y * rcp_dir_min));

            const nimg::ColorRGBAf rgb_a =
                (sample_clamped_bilinear(src, xf + dir_x * (1.0f / 3.0f - 0.5f), yf + dir_y * (1.0f / 3.0f - 0.5f)) +
                 sample_clamped_bilinear(src, xf + dir_x * (2.0f / 3.0f - 0.5f), yf + dir_y * (2.0f / 3.0f - 0.5f))) * 0.5f;

            const nimg::ColorRGBAf rgb_b = rgb_a * 0.5f +
                (sample_clamped_bilinear(src, xf + dir_x * -0.5f, yf + dir_y * -0.5f) +
                 sample_clamped_bilinear(src, xf + dir_x * 0.5f, yf + dir_y * 0.5f)) * 0.25f;

            const float luma_b = luminance(rgb_b);
            nimg::ColorRGBAf filtered = ((luma_b < luma_min) || (luma_b > luma_max)) ? rgb_a : rgb_b;

            nimg::ColorRGBAf out = rgb_m * (1.0f - subpix_amount) + filtered * subpix_amount;
            out.a(rgb_m.a());
            p->pixel(x, y) = out;
        }
    }
}

    } /* namespace filter */
} /* namespace xtcore */
