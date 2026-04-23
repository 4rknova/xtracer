#include <algorithm>
#include <cmath>
#include <nimg/luminance.h>
#include <nmath/prng.h>
#include <nmath/precision.h>
#include <nimg/color.h>
#include "sampler_erp.h"

namespace xtcore {
    namespace sampler {

namespace {

inline nmath::scalar_t clamp_scalar(nmath::scalar_t v, nmath::scalar_t lo, nmath::scalar_t hi)
{
    return std::max(lo, std::min(v, hi));
}

inline void direction_to_erp_uv(const nmath::Vector3f &dir_in, nmath::scalar_t &u, nmath::scalar_t &v)
{
    const nmath::Vector3f dir = dir_in.normalized();
    const nmath::scalar_t theta = nmath_acos(clamp_scalar(dir.y, (nmath::scalar_t)-1.0, (nmath::scalar_t)1.0));
    const nmath::scalar_t phi = nmath_atan2(dir.x, -dir.z);
    u = (nmath::scalar_t)0.5 + phi / (nmath::PI_DOUBLE * (nmath::scalar_t)2.0);
    v = theta / nmath::PI;
}

inline nmath::Vector3f erp_uv_to_direction(nmath::scalar_t u, nmath::scalar_t v)
{
    const nmath::scalar_t theta = clamp_scalar(v, (nmath::scalar_t)0.0, (nmath::scalar_t)1.0) * nmath::PI;
    const nmath::scalar_t phi = (u - (nmath::scalar_t)0.5) * nmath::PI_DOUBLE * (nmath::scalar_t)2.0;
    const nmath::scalar_t sin_theta = nmath_sin(theta);
    return nmath::Vector3f(
        sin_theta * nmath_sin(phi),
        nmath_cos(theta),
        -sin_theta * nmath_cos(phi)
    ).normalized();
}

} // namespace

ERP::ERP()
    : m_distribution_ready(false)
    , m_total_weight(0.0)
{}

int ERP::load(const char *file)
{
    m_distribution_ready = false;
    m_row_cdf.clear();
    m_conditional_cdf.clear();
    m_total_weight = 0.0;
    m_texture.set_filtering(FILTERING_LINEAR);
    return m_texture.load(file);
}

nimg::ColorRGBf ERP::sample(const nmath::Vector3f &tc) const
{
    nmath::scalar_t u = 0.0;
    nmath::scalar_t v = 0.0;
    direction_to_erp_uv(tc, u, v);
    nmath::Vector3f coords((float)u, (float)v, 0);
    return m_texture.sample(coords);
}

void ERP::build_distribution() const
{
    if (m_distribution_ready) return;

    const size_t w = m_texture.width();
    const size_t h = m_texture.height();
    m_row_cdf.assign(h, 0.0);
    m_conditional_cdf.assign(w * h, 0.0);
    m_total_weight = 0.0;

    if (w == 0 || h == 0) {
        m_distribution_ready = true;
        return;
    }

    for (size_t y = 0; y < h; ++y) {
        const nmath::scalar_t theta = (((nmath::scalar_t)y + (nmath::scalar_t)0.5) / (nmath::scalar_t)h) * nmath::PI;
        const nmath::scalar_t sin_theta = std::max((nmath::scalar_t)0.0, nmath_sin(theta));
        nmath::scalar_t row_sum = 0.0;
        for (size_t x = 0; x < w; ++x) {
            const nimg::ColorRGBf texel(m_texture.pixel_ro(x, y));
            const nmath::scalar_t weight = std::max((nmath::scalar_t)0.0, (nmath::scalar_t)nimg::eval::luminance(texel) * sin_theta);
            row_sum += weight;
            m_conditional_cdf[y * w + x] = row_sum;
        }
        if (row_sum > (nmath::scalar_t)EPSILON) {
            for (size_t x = 0; x < w; ++x) {
                m_conditional_cdf[y * w + x] /= row_sum;
            }
        }
        m_total_weight += row_sum;
        m_row_cdf[y] = m_total_weight;
    }

    if (m_total_weight > (nmath::scalar_t)EPSILON) {
        for (size_t y = 0; y < h; ++y) {
            m_row_cdf[y] /= m_total_weight;
        }
    }

    m_distribution_ready = true;
}

bool ERP::sample_texel(size_t &x, size_t &y, nmath::scalar_t &pmf) const
{
    build_distribution();
    const size_t w = m_texture.width();
    const size_t h = m_texture.height();
    if (w == 0 || h == 0 || m_total_weight <= (nmath::scalar_t)EPSILON) return false;

    const nmath::scalar_t uy = nmath::prng_c(0.0, 1.0);
    y = std::lower_bound(m_row_cdf.begin(), m_row_cdf.end(), uy) - m_row_cdf.begin();
    if (y >= h) y = h - 1;

    const nmath::scalar_t ux = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t *row = &m_conditional_cdf[y * w];
    x = std::lower_bound(row, row + w, ux) - row;
    if (x >= w) x = w - 1;

    pmf = texel_pmf(x, y);
    return pmf > (nmath::scalar_t)EPSILON;
}

nmath::scalar_t ERP::texel_pmf(size_t x, size_t y) const
{
    build_distribution();
    const size_t w = m_texture.width();
    const size_t h = m_texture.height();
    if (w == 0 || h == 0 || m_total_weight <= (nmath::scalar_t)EPSILON) return 0.0;

    const nmath::scalar_t theta = (((nmath::scalar_t)y + (nmath::scalar_t)0.5) / (nmath::scalar_t)h) * nmath::PI;
    const nmath::scalar_t sin_theta = std::max((nmath::scalar_t)0.0, nmath_sin(theta));
    const nimg::ColorRGBf texel(m_texture.pixel_ro(x, y));
    const nmath::scalar_t weight = std::max((nmath::scalar_t)0.0, (nmath::scalar_t)nimg::eval::luminance(texel) * sin_theta);
    return weight / m_total_weight;
}

bool ERP::sample_direction(nmath::Vector3f &direction, nmath::scalar_t &pdf, nimg::ColorRGBf &radiance) const
{
    size_t x = 0, y = 0;
    nmath::scalar_t pmf = 0.0;
    if (!sample_texel(x, y, pmf)) {
        direction = nmath::Vector3f(0.0f, 1.0f, 0.0f);
        pdf = 0.0;
        radiance = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
        return false;
    }

    const nmath::scalar_t w = (nmath::scalar_t)m_texture.width();
    const nmath::scalar_t h = (nmath::scalar_t)m_texture.height();
    const nmath::scalar_t u = (((nmath::scalar_t)x + nmath::prng_c(0.0, 1.0)) / w);
    const nmath::scalar_t v = (((nmath::scalar_t)y + nmath::prng_c(0.0, 1.0)) / h);
    direction = erp_uv_to_direction(u, v);
    pdf = pdf_direction(direction);
    radiance = sample(direction);
    return pdf > (nmath::scalar_t)EPSILON;
}

nmath::scalar_t ERP::pdf_direction(const nmath::Vector3f &direction) const
{
    build_distribution();
    const size_t w = m_texture.width();
    const size_t h = m_texture.height();
    if (w == 0 || h == 0 || m_total_weight <= (nmath::scalar_t)EPSILON) return 0.0;

    nmath::scalar_t u = 0.0;
    nmath::scalar_t v = 0.0;
    direction_to_erp_uv(direction, u, v);
    const size_t x = std::min((size_t)std::floor((double)(clamp_scalar(u, (nmath::scalar_t)0.0, (nmath::scalar_t)0.999999) * (nmath::scalar_t)w)), w - 1);
    const size_t y = std::min((size_t)std::floor((double)(clamp_scalar(v, (nmath::scalar_t)0.0, (nmath::scalar_t)0.999999) * (nmath::scalar_t)h)), h - 1);

    const nmath::scalar_t pmf = texel_pmf(x, y);
    const nmath::scalar_t theta = clamp_scalar(v, (nmath::scalar_t)0.0, (nmath::scalar_t)1.0) * nmath::PI;
    const nmath::scalar_t sin_theta = std::max((nmath::scalar_t)1e-5, nmath_sin(theta));
    const nmath::scalar_t solid_angle = (nmath::PI_DOUBLE * (nmath::scalar_t)2.0 * nmath::PI * sin_theta) / ((nmath::scalar_t)w * (nmath::scalar_t)h);
    return pmf / std::max((nmath::scalar_t)EPSILON, solid_angle);
}

    } /* namespace sampler */
} /* namespace xtcore */
