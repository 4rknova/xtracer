#include <algorithm>
#include <cmath>

#include <nimg/color.h>
#include <nimg/luminance.h>
#include <nmath/precision.h>
#include <nmath/prng.h>

#include "sampler_cubemap.h"

namespace xtcore {
    namespace sampler {

namespace {

inline nmath::scalar_t clamp_scalar(nmath::scalar_t v, nmath::scalar_t lo, nmath::scalar_t hi)
{
    return std::max(lo, std::min(v, hi));
}

inline size_t face_index(CUBEMAP_FACE face)
{
    return static_cast<size_t>(face);
}

} // namespace

Cubemap::Cubemap()
    : m_distribution_ready(false)
    , m_total_weight(0.0)
{}

int Cubemap::load(const char *file, CUBEMAP_FACE face)
{
    m_distribution_ready = false;
    m_face_cdf.clear();
    for (size_t i = 0; i < 6; ++i) m_conditional_cdf[i].clear();
    m_total_weight = 0.0;
    return m_textures[face].load(file);
}

bool Cubemap::direction_to_face_uv(const nmath::Vector3f &tc, CUBEMAP_FACE &face, nmath::scalar_t &u, nmath::scalar_t &v) const
{
    const nmath::Vector3f dir = tc.normalized();

    if ((nmath_abs(dir.x) >= nmath_abs(dir.y)) && (nmath_abs(dir.x) >= nmath_abs(dir.z))) {
        if (dir.x > 0.0f) {
            face = CUBEMAP_FACE_RIGHT;
            u = (dir.z / dir.x + (nmath::scalar_t)1.0) * (nmath::scalar_t)0.5;
            v = (nmath::scalar_t)1.0 - (dir.y / dir.x + (nmath::scalar_t)1.0) * (nmath::scalar_t)0.5;
            return true;
        } else if (dir.x < 0.0f) {
            face = CUBEMAP_FACE_LEFT;
            u = (dir.z / dir.x + (nmath::scalar_t)1.0) * (nmath::scalar_t)0.5;
            v = (dir.y / dir.x + (nmath::scalar_t)1.0) * (nmath::scalar_t)0.5;
            return true;
        }
    } else if ((nmath_abs(dir.y) >= nmath_abs(dir.x)) && (nmath_abs(dir.y) >= nmath_abs(dir.z))) {
        if (dir.y > 0.0f) {
            face = CUBEMAP_FACE_TOP;
            u = (dir.x / dir.y + (nmath::scalar_t)1.0) * (nmath::scalar_t)0.5;
            v = (nmath::scalar_t)1.0 - (dir.z / dir.y + (nmath::scalar_t)1.0) * (nmath::scalar_t)0.5;
            return true;
        } else if (dir.y < 0.0f) {
            face = CUBEMAP_FACE_BOTTOM;
            u = (nmath::scalar_t)1.0 - (dir.x / dir.y + (nmath::scalar_t)1.0) * (nmath::scalar_t)0.5;
            v = (nmath::scalar_t)1.0 - (dir.z / dir.y + (nmath::scalar_t)1.0) * (nmath::scalar_t)0.5;
            return true;
        }
    } else if ((nmath_abs(dir.z) >= nmath_abs(dir.x)) && (nmath_abs(dir.z) >= nmath_abs(dir.y))) {
        if (dir.z > 0.0f) {
            face = CUBEMAP_FACE_BACK;
            u = (nmath::scalar_t)1.0 - (dir.x / dir.z + (nmath::scalar_t)1.0) * (nmath::scalar_t)0.5;
            v = (nmath::scalar_t)1.0 - (dir.y / dir.z + (nmath::scalar_t)1.0) * (nmath::scalar_t)0.5;
            return true;
        } else if (dir.z < 0.0f) {
            face = CUBEMAP_FACE_FRONT;
            u = (nmath::scalar_t)1.0 - (dir.x / dir.z + (nmath::scalar_t)1.0) * (nmath::scalar_t)0.5;
            v = (dir.y / dir.z + (nmath::scalar_t)1.0) * (nmath::scalar_t)0.5;
            return true;
        }
    }

    face = CUBEMAP_FACE_TOP;
    u = (nmath::scalar_t)0.5;
    v = (nmath::scalar_t)0.5;
    return false;
}

nmath::Vector3f Cubemap::face_uv_to_direction(CUBEMAP_FACE face, nmath::scalar_t u, nmath::scalar_t v) const
{
    const nmath::scalar_t uc = clamp_scalar(u, (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
    const nmath::scalar_t vc = clamp_scalar(v, (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
    const nmath::scalar_t sx = (nmath::scalar_t)2.0 * uc - (nmath::scalar_t)1.0;
    const nmath::scalar_t sy = (nmath::scalar_t)2.0 * vc - (nmath::scalar_t)1.0;

    nmath::Vector3f dir;
    switch (face) {
        case CUBEMAP_FACE_RIGHT:  dir = nmath::Vector3f( 1.0f, -(float)sy,  (float)sx); break;
        case CUBEMAP_FACE_LEFT:   dir = nmath::Vector3f(-1.0f, -(float)sy, -(float)sx); break;
        case CUBEMAP_FACE_TOP:    dir = nmath::Vector3f( (float)sx,  1.0f,  (float)(-sy)); break;
        case CUBEMAP_FACE_BOTTOM: dir = nmath::Vector3f((float)(-sx), -1.0f, (float)(-sy)); break;
        case CUBEMAP_FACE_BACK:   dir = nmath::Vector3f((float)(-sx), -(float)sy, 1.0f); break;
        case CUBEMAP_FACE_FRONT:  dir = nmath::Vector3f((float)sx, -(float)sy, -1.0f); break;
        default:                  dir = nmath::Vector3f(0.0f, 1.0f, 0.0f); break;
    }
    return dir.normalized();
}

nimg::ColorRGBf Cubemap::sample(const nmath::Vector3f &tc) const
{
    CUBEMAP_FACE face = CUBEMAP_FACE_TOP;
    nmath::scalar_t u = 0.5;
    nmath::scalar_t v = 0.5;
    direction_to_face_uv(tc, face, u, v);
    nmath::Vector3f coords((float)u, (float)v, 0.0f);
    return m_textures[face].sample(coords);
}

nmath::scalar_t Cubemap::texel_solid_angle(CUBEMAP_FACE face, size_t x, size_t y) const
{
    (void)face;
    const Texture2D &tex = m_textures[face];
    const nmath::scalar_t w = (nmath::scalar_t)tex.width();
    const nmath::scalar_t h = (nmath::scalar_t)tex.height();
    if (w <= (nmath::scalar_t)0.0 || h <= (nmath::scalar_t)0.0) return 0.0;

    const nmath::scalar_t u = (((nmath::scalar_t)x + (nmath::scalar_t)0.5) / w) * (nmath::scalar_t)2.0 - (nmath::scalar_t)1.0;
    const nmath::scalar_t v = (((nmath::scalar_t)y + (nmath::scalar_t)0.5) / h) * (nmath::scalar_t)2.0 - (nmath::scalar_t)1.0;
    const nmath::scalar_t denom = nmath_pow((nmath::scalar_t)1.0 + u * u + v * v, (nmath::scalar_t)1.5);
    return ((nmath::scalar_t)4.0 / (w * h)) / std::max((nmath::scalar_t)EPSILON, denom);
}

void Cubemap::build_distribution() const
{
    if (m_distribution_ready) return;

    m_face_cdf.assign(6, 0.0);
    for (size_t i = 0; i < 6; ++i) m_conditional_cdf[i].clear();
    m_total_weight = 0.0;

    for (size_t fi = 0; fi < 6; ++fi) {
        const CUBEMAP_FACE face = static_cast<CUBEMAP_FACE>(fi);
        const Texture2D &tex = m_textures[face];
        const size_t w = tex.width();
        const size_t h = tex.height();
        m_conditional_cdf[fi].assign(w * h, 0.0);

        nmath::scalar_t face_sum = 0.0;
        for (size_t y = 0; y < h; ++y) {
            for (size_t x = 0; x < w; ++x) {
                const nimg::ColorRGBf texel(tex.pixel_ro(x, y));
                const nmath::scalar_t weight = std::max((nmath::scalar_t)0.0,
                    (nmath::scalar_t)nimg::eval::luminance(texel) * texel_solid_angle(face, x, y));
                face_sum += weight;
                m_conditional_cdf[fi][y * w + x] = face_sum;
            }
        }

        if (face_sum > (nmath::scalar_t)EPSILON) {
            for (size_t i = 0; i < w * h; ++i) {
                m_conditional_cdf[fi][i] /= face_sum;
            }
        }

        m_total_weight += face_sum;
        m_face_cdf[fi] = m_total_weight;
    }

    if (m_total_weight > (nmath::scalar_t)EPSILON) {
        for (size_t fi = 0; fi < 6; ++fi) m_face_cdf[fi] /= m_total_weight;
    }

    m_distribution_ready = true;
}

nmath::scalar_t Cubemap::texel_pmf(CUBEMAP_FACE face, size_t x, size_t y) const
{
    build_distribution();
    const Texture2D &tex = m_textures[face];
    if (tex.width() == 0 || tex.height() == 0 || m_total_weight <= (nmath::scalar_t)EPSILON) return 0.0;

    const nimg::ColorRGBf texel(tex.pixel_ro(x, y));
    const nmath::scalar_t weight = std::max((nmath::scalar_t)0.0,
        (nmath::scalar_t)nimg::eval::luminance(texel) * texel_solid_angle(face, x, y));
    return weight / m_total_weight;
}

bool Cubemap::sample_texel(CUBEMAP_FACE &face, size_t &x, size_t &y, nmath::scalar_t &pmf) const
{
    build_distribution();
    if (m_total_weight <= (nmath::scalar_t)EPSILON) return false;

    const nmath::scalar_t uf = nmath::prng_c(0.0, 1.0);
    size_t fi = std::lower_bound(m_face_cdf.begin(), m_face_cdf.end(), uf) - m_face_cdf.begin();
    if (fi >= 6) fi = 5;
    face = static_cast<CUBEMAP_FACE>(fi);

    const Texture2D &tex = m_textures[face];
    const size_t w = tex.width();
    const size_t h = tex.height();
    if (w == 0 || h == 0) return false;

    const nmath::scalar_t ut = nmath::prng_c(0.0, 1.0);
    const std::vector<nmath::scalar_t> &cdf = m_conditional_cdf[fi];
    size_t idx = std::lower_bound(cdf.begin(), cdf.end(), ut) - cdf.begin();
    if (idx >= w * h) idx = w * h - 1;
    x = idx % w;
    y = idx / w;
    pmf = texel_pmf(face, x, y);
    return pmf > (nmath::scalar_t)EPSILON;
}

bool Cubemap::sample_direction(nmath::Vector3f &direction, nmath::scalar_t &pdf, nimg::ColorRGBf &radiance) const
{
    CUBEMAP_FACE face = CUBEMAP_FACE_TOP;
    size_t x = 0, y = 0;
    nmath::scalar_t pmf = 0.0;
    if (!sample_texel(face, x, y, pmf)) {
        direction = nmath::Vector3f(0.0f, 1.0f, 0.0f);
        pdf = 0.0;
        radiance = nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
        return false;
    }

    const Texture2D &tex = m_textures[face];
    const nmath::scalar_t u = ((nmath::scalar_t)x + nmath::prng_c(0.0, 1.0)) / (nmath::scalar_t)tex.width();
    const nmath::scalar_t v = ((nmath::scalar_t)y + nmath::prng_c(0.0, 1.0)) / (nmath::scalar_t)tex.height();
    direction = face_uv_to_direction(face, u, v);
    pdf = pdf_direction(direction);
    radiance = sample(direction);
    return pdf > (nmath::scalar_t)EPSILON;
}

nmath::scalar_t Cubemap::pdf_direction(const nmath::Vector3f &direction) const
{
    CUBEMAP_FACE face = CUBEMAP_FACE_TOP;
    nmath::scalar_t u = 0.5;
    nmath::scalar_t v = 0.5;
    direction_to_face_uv(direction, face, u, v);

    const Texture2D &tex = m_textures[face];
    const size_t w = tex.width();
    const size_t h = tex.height();
    if (w == 0 || h == 0) return 0.0;

    const size_t x = std::min((size_t)std::floor((double)(clamp_scalar(u, (nmath::scalar_t)0.0, (nmath::scalar_t)0.999999) * (nmath::scalar_t)w)), w - 1);
    const size_t y = std::min((size_t)std::floor((double)(clamp_scalar(v, (nmath::scalar_t)0.0, (nmath::scalar_t)0.999999) * (nmath::scalar_t)h)), h - 1);
    const nmath::scalar_t pmf = texel_pmf(face, x, y);
    return pmf / std::max((nmath::scalar_t)EPSILON, texel_solid_angle(face, x, y));
}

    } /* namespace sampler */
} /* namespace xtcore */
