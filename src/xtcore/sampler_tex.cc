#include "sampler_tex.h"

namespace xtracer {

SamplerTexture::SamplerTexture(const char *file)
{
    m_texture.load(file);
}

nimg::ColorRGBf SamplerTexture::sample(NMath::Vector3f &uvw) const
{
    return m_texture.sample(uvw.x, uvw.y);
}


} /* namespace xtracer */
