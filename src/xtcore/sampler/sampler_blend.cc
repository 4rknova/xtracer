#include <algorithm>

#include "sampler_blend.h"

namespace xtcore {
    namespace sampler {

Blend::Blend()
    : a(0)
    , b(0)
    , t((nmath::scalar_t)0.5)
{}

Blend::~Blend()
{
    delete a;
    delete b;
}

nimg::ColorRGBf Blend::sample(const nmath::Vector3f &uvw) const
{
    const nmath::scalar_t tc = std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, t));

    const nimg::ColorRGBf ca = a ? a->sample(uvw) : nimg::ColorRGBf(0.f, 0.f, 0.f);
    const nimg::ColorRGBf cb = b ? b->sample(uvw) : nimg::ColorRGBf(0.f, 0.f, 0.f);

    return ca * ((nmath::scalar_t)1.0 - tc) + cb * tc;
}

    } /* namespace sampler */
} /* namespace xtcore */
