#ifndef XTRACER_SAMPLER_TEX_H_INCLUDED
#define XTRACER_SAMPLER_TEX_H_INCLUDED

#include "texture.h"
#include "sampler.h"

namespace xtracer {

class SamplerTexture
{
    public:
    SamplerTexture(const char *file);

    nimg::ColorRGBf sample(NMath::Vector3f &uvw) const;

    private:
    xtracer::assets::Texture2D m_texture;
};

} /* namespace xtracer */

#endif /* XTRACER_SAMPLER_TEX_H_INCLUDED */
