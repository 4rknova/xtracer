#include "sampler_cb.h"

namespace xtracer {

SamplerCubemap::SamplerCubemap(
          const char *px, const char *nx
        , const char *py, const char *ny
        , const char *pz, const char *nz)
{
    m_cubemap.load(px, xtracer::assets::CUBEMAP_FACE_RIGHT);
    m_cubemap.load(nx, xtracer::assets::CUBEMAP_FACE_LEFT);
    m_cubemap.load(py, xtracer::assets::CUBEMAP_FACE_TOP);
    m_cubemap.load(ny, xtracer::assets::CUBEMAP_FACE_BOTTOM);
    m_cubemap.load(pz, xtracer::assets::CUBEMAP_FACE_FRONT);
    m_cubemap.load(nz, xtracer::assets::CUBEMAP_FACE_BACK);
}

nimg::ColorRGBf SamplerCubemap::sample(NMath::Vector3f &uvw) const
{
    return m_cubemap.sample(uvw);
}


} /* namespace xtracer */
