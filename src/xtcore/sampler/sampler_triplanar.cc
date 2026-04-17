#include <cmath>
#include <algorithm>

#include "sampler_triplanar.h"

namespace xtcore {
    namespace sampler {

Triplanar::Triplanar()
    : child(0)
    , scale((nmath::scalar_t)1.0)
    , blend_sharpness((nmath::scalar_t)4.0)
{}

Triplanar::~Triplanar()
{
    delete child;
}

nimg::ColorRGBf Triplanar::sample(const nmath::Vector3f &uvw) const
{
    if (!child) return nimg::ColorRGBf(0.f, 0.f, 0.f);

    const nmath::scalar_t s = (std::fabs((double)scale) > 1e-8) ? scale : (nmath::scalar_t)1.0;
    const nmath::scalar_t sp = (blend_sharpness > (nmath::scalar_t)0.1) ? blend_sharpness : (nmath::scalar_t)0.1;

    // Treat uvw as the 3D object-space position.
    // Sample the child with each axis pair as UV coordinates.
    const nmath::Vector3f px(uvw.y * s, uvw.z * s, (nmath::scalar_t)0.0); // YZ plane (X-normal)
    const nmath::Vector3f py(uvw.x * s, uvw.z * s, (nmath::scalar_t)0.0); // XZ plane (Y-normal)
    const nmath::Vector3f pz(uvw.x * s, uvw.y * s, (nmath::scalar_t)0.0); // XY plane (Z-normal)

    const nimg::ColorRGBf cx = child->sample(px);
    const nimg::ColorRGBf cy = child->sample(py);
    const nimg::ColorRGBf cz = child->sample(pz);

    // Blend weights derived from the absolute magnitudes of uvw axes.
    // A sharp power curve controls the transition between projections.
    nmath::scalar_t wx = (nmath::scalar_t)std::pow((double)std::fabs((double)uvw.x), (double)sp);
    nmath::scalar_t wy = (nmath::scalar_t)std::pow((double)std::fabs((double)uvw.y), (double)sp);
    nmath::scalar_t wz = (nmath::scalar_t)std::pow((double)std::fabs((double)uvw.z), (double)sp);

    const nmath::scalar_t wsum = wx + wy + wz + (nmath::scalar_t)1e-8;
    wx /= wsum; wy /= wsum; wz /= wsum;

    return cx * wx + cy * wy + cz * wz;
}

    } /* namespace sampler */
} /* namespace xtcore */
