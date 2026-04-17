#include <algorithm>
#include <cmath>

#include "sampler_mix_masked.h"

namespace xtcore {
    namespace sampler {

MixMasked::MixMasked()
    : base(0)
    , overlay(0)
    , mask(0)
    , t((nmath::scalar_t)1.0)
    , mode(MODE_LERP)
{}

MixMasked::~MixMasked()
{
    delete base;
    delete overlay;
    delete mask;
}

nimg::ColorRGBf MixMasked::sample(const nmath::Vector3f &uvw) const
{
    const nimg::ColorRGBf cb = base    ? base->sample(uvw)    : nimg::ColorRGBf(0.f, 0.f, 0.f);
    const nimg::ColorRGBf co = overlay ? overlay->sample(uvw) : nimg::ColorRGBf(0.f, 0.f, 0.f);

    nmath::scalar_t m;
    if (mask) {
        const nimg::ColorRGBf cm = mask->sample(uvw);
        m = (cm.r() + cm.g() + cm.b()) / (nmath::scalar_t)3.0;
    } else {
        m = t;
    }
    m = std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, m));

    nimg::ColorRGBf result;
    switch (mode) {
        case MODE_MULTIPLY:
            result = nimg::ColorRGBf(cb.r() * co.r(), cb.g() * co.g(), cb.b() * co.b());
            result = cb * ((nmath::scalar_t)1.0 - m) + result * m;
            break;
        case MODE_ADD:
            result = nimg::ColorRGBf(
                std::min((nmath::scalar_t)1.0, cb.r() + co.r() * m),
                std::min((nmath::scalar_t)1.0, cb.g() + co.g() * m),
                std::min((nmath::scalar_t)1.0, cb.b() + co.b() * m)
            );
            break;
        case MODE_SCREEN: {
            const nimg::ColorRGBf sc(
                (nmath::scalar_t)1.0 - ((nmath::scalar_t)1.0 - cb.r()) * ((nmath::scalar_t)1.0 - co.r()),
                (nmath::scalar_t)1.0 - ((nmath::scalar_t)1.0 - cb.g()) * ((nmath::scalar_t)1.0 - co.g()),
                (nmath::scalar_t)1.0 - ((nmath::scalar_t)1.0 - cb.b()) * ((nmath::scalar_t)1.0 - co.b())
            );
            result = cb * ((nmath::scalar_t)1.0 - m) + sc * m;
            break;
        }
        case MODE_LERP:
        default:
            result = cb * ((nmath::scalar_t)1.0 - m) + co * m;
            break;
    }
    return result;
}

    } /* namespace sampler */
} /* namespace xtcore */
