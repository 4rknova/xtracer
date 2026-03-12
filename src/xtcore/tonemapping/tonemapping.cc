#include <algorithm>
#include <cmath>
#include <vector>
#include <nimg/luminance.h>

#include "tonemapping.h"

namespace xtcore {
namespace tonemapping {

namespace {

inline float clamp01(float v)
{
    return std::max(0.0f, std::min(1.0f, v));
}

inline float reinhard_extended(float x, float white_point)
{
    if (white_point <= 0.0f) return x / (1.0f + x);
    const float w2 = white_point * white_point;
    return (x * (1.0f + x / w2)) / (1.0f + x);
}

inline nimg::ColorRGBf apply_reinhard(const nimg::ColorRGBf &c, float white_point)
{
    return nimg::ColorRGBf(
          reinhard_extended(c.r(), white_point)
        , reinhard_extended(c.g(), white_point)
        , reinhard_extended(c.b(), white_point)
    );
}

inline nimg::ColorRGBf apply_reinhard_luminance(const nimg::ColorRGBf &c, float white_point)
{
    const float l = nimg::eval::luminance(c);
    if (l <= 0.0f) return nimg::ColorRGBf(0, 0, 0);

    const float mapped_l = reinhard_extended(l, white_point);
    const float s = mapped_l / l;
    return c * s;
}

inline nimg::ColorRGBf apply_aces_fitted(const nimg::ColorRGBf &c)
{
    // Narkowicz ACES fitted curve.
    const float a = 2.51f;
    const float b = 0.03f;
    const float c1 = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;

    const float r = (c.r() * (a * c.r() + b)) / (c.r() * (c1 * c.r() + d) + e);
    const float g = (c.g() * (a * c.g() + b)) / (c.g() * (c1 * c.g() + d) + e);
    const float bch = (c.b() * (a * c.b() + b)) / (c.b() * (c1 * c.b() + d) + e);

    return nimg::ColorRGBf(clamp01(r), clamp01(g), clamp01(bch));
}

inline size_t idx(size_t x, size_t y, size_t w)
{
    return y * w + x;
}

void gaussian_blur_separable(const std::vector<float> &src,
                             std::vector<float> &dst,
                             size_t w, size_t h,
                             float sigma)
{
    if (w == 0 || h == 0) return;

    sigma = std::max(0.15f, sigma);
    int radius = std::max(1, (int)std::ceil(3.0f * sigma));

    std::vector<float> kernel((size_t)(2 * radius + 1), 0.0f);
    float sum = 0.0f;
    for (int i = -radius; i <= radius; ++i) {
        const float x = (float)i;
        const float v = std::exp(-(x * x) / (2.0f * sigma * sigma));
        kernel[(size_t)(i + radius)] = v;
        sum += v;
    }
    if (sum <= 0.0f) sum = 1.0f;
    for (size_t i = 0; i < kernel.size(); ++i) kernel[i] /= sum;

    std::vector<float> tmp(w * h, 0.0f);

    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            float acc = 0.0f;
            for (int k = -radius; k <= radius; ++k) {
                int sx = (int)x + k;
                if (sx < 0) sx = 0;
                if (sx >= (int)w) sx = (int)w - 1;
                acc += src[idx((size_t)sx, y, w)] * kernel[(size_t)(k + radius)];
            }
            tmp[idx(x, y, w)] = acc;
        }
    }

    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            float acc = 0.0f;
            for (int k = -radius; k <= radius; ++k) {
                int sy = (int)y + k;
                if (sy < 0) sy = 0;
                if (sy >= (int)h) sy = (int)h - 1;
                acc += tmp[idx(x, (size_t)sy, w)] * kernel[(size_t)(k + radius)];
            }
            dst[idx(x, y, w)] = acc;
        }
    }
}

void apply_mantiuk_2006_pixmap(nimg::Pixmap &pixmap, const settings_t &settings)
{
    const size_t w = pixmap.width();
    const size_t h = pixmap.height();
    if (w == 0 || h == 0) return;

    const float contrast = std::max(0.0f, std::min(1.0f, settings.mantiuk_contrast));
    const float saturation = std::max(0.0f, std::min(2.0f, settings.mantiuk_saturation));
    const float detail = std::max(1.0f, std::min(99.0f, settings.mantiuk_detail));
    const float white_point = (settings.white_point > 0.0f) ? settings.white_point : 1.0f;

    const float exposure = (settings.exposure > 0.0f) ? settings.exposure : 1.0f;
    const float eps = 1e-6f;

    std::vector<float> lum(w * h, 0.0f);
    std::vector<float> log_lum(w * h, 0.0f);
    std::vector<float> base(w * h, 0.0f);
    std::vector<float> log_mapped(w * h, 0.0f);

    float mean_log = 0.0f;
    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            nimg::ColorRGBAf px = pixmap.pixel(x, y);
            nimg::ColorRGBf c = nimg::ColorRGBf(px) * exposure;
            const float l = std::max(eps, nimg::eval::luminance(c));
            const size_t i = idx(x, y, w);
            lum[i] = l;
            log_lum[i] = std::log(l);
            mean_log += log_lum[i];
        }
    }
    mean_log /= (float)(w * h);

    // Detail controls local adaptation scale. Larger detail => tighter local adaptation.
    // Use an exponential mapping to make the control perceptually stronger across the range.
    const float t = (detail - 1.0f) / 98.0f;
    const float sigma = 12.0f * std::pow(0.05f, t); // ~12.0 -> ~0.6
    gaussian_blur_separable(log_lum, base, w, h, sigma);

    // Keep base compression modest by default; preserve local contrast with detail gain.
    const float base_scale = 0.18f + 0.82f * contrast;
    const float dnorm = (detail - 50.0f) / 49.0f;   // [-1, 1]
    const float detail_gain = std::pow(2.5f, dnorm); // ~0.4 -> ~2.5
    for (size_t i = 0; i < log_mapped.size(); ++i) {
        const float detail_layer = log_lum[i] - base[i];
        const float centered_base = base[i] - mean_log;
        log_mapped[i] = centered_base * base_scale + detail_layer * detail_gain;
    }

    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            const size_t i = idx(x, y, w);
            float mapped_l = std::exp(log_mapped[i]);
            mapped_l = reinhard_extended(mapped_l, white_point);
            mapped_l = std::max(eps, mapped_l);
            const float src_l = std::max(eps, lum[i]);

            nimg::ColorRGBAf px = pixmap.pixel(x, y);
            nimg::ColorRGBf c = nimg::ColorRGBf(px) * exposure;

            // Chroma-preserving reconstruction with explicit saturation control.
            const float rr = c.r() / src_l;
            const float gg = c.g() / src_l;
            const float bb = c.b() / src_l;
            const float sr = 1.0f + saturation * (rr - 1.0f);
            const float sg = 1.0f + saturation * (gg - 1.0f);
            const float sb = 1.0f + saturation * (bb - 1.0f);

            float r = mapped_l * std::max(0.0f, sr);
            float g = mapped_l * std::max(0.0f, sg);
            float b = mapped_l * std::max(0.0f, sb);

            px.r(clamp01(r));
            px.g(clamp01(g));
            px.b(clamp01(b));
            pixmap.pixel(x, y) = px;
        }
    }
}

} // namespace

nimg::ColorRGBf apply(const nimg::ColorRGBf &color, const settings_t &settings)
{
    nimg::ColorRGBf c = color;

    const float exposure = (settings.exposure > 0.0f) ? settings.exposure : 1.0f;
    const float white_point = (settings.white_point > 0.0f) ? settings.white_point : 1.0f;
    c *= exposure;

    switch (settings.op) {
        case OP_NONE:
            return c;
        case OP_REINHARD:
            return apply_reinhard(c, white_point);
        case OP_REINHARD_LUMINANCE:
            return apply_reinhard_luminance(c, white_point);
        case OP_MANTIUK_2006:
            // Full Mantiuk path needs image context and is handled in apply(Pixmap).
            return apply_reinhard_luminance(c, white_point);
        case OP_ACES_FITTED:
            return apply_aces_fitted(c);
    }

    return c;
}

void apply(nimg::Pixmap &pixmap, const settings_t &settings)
{
    if (settings.op == OP_MANTIUK_2006) {
        apply_mantiuk_2006_pixmap(pixmap, settings);
        return;
    }

    for (size_t y = 0; y < pixmap.height(); ++y) {
        for (size_t x = 0; x < pixmap.width(); ++x) {
            nimg::ColorRGBAf px = pixmap.pixel(x, y);
            nimg::ColorRGBf tm = apply(nimg::ColorRGBf(px), settings);
            px.r(tm.r());
            px.g(tm.g());
            px.b(tm.b());
            pixmap.pixel(x, y) = px;
        }
    }
}

} /* namespace tonemapping */
} /* namespace xtcore */
