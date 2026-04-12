#ifndef XTCORE_SAMPLER_CUBEMAP_H_INCLUDED
#define XTCORE_SAMPLER_CUBEMAP_H_INCLUDED

#include <nmath/vector.h>
#include <vector>
#include "sampler_tex.h"
#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

enum CUBEMAP_FACE
{
      CUBEMAP_FACE_LEFT
    , CUBEMAP_FACE_RIGHT
    , CUBEMAP_FACE_TOP
    , CUBEMAP_FACE_BOTTOM
    , CUBEMAP_FACE_FRONT
    , CUBEMAP_FACE_BACK
};

class Cubemap : public ISampler
{
    public:
    Cubemap();
    int load(const char *file, CUBEMAP_FACE face);

    nimg::ColorRGBf sample(const nmath::Vector3f &tc) const;
    bool sample_direction(nmath::Vector3f &direction, nmath::scalar_t &pdf, nimg::ColorRGBf &radiance) const;
    nmath::scalar_t pdf_direction(const nmath::Vector3f &direction) const;

    private:
    void build_distribution() const;
    bool sample_texel(CUBEMAP_FACE &face, size_t &x, size_t &y, nmath::scalar_t &pmf) const;
    nmath::scalar_t texel_pmf(CUBEMAP_FACE face, size_t x, size_t y) const;
    nmath::scalar_t texel_solid_angle(CUBEMAP_FACE face, size_t x, size_t y) const;
    bool direction_to_face_uv(const nmath::Vector3f &dir, CUBEMAP_FACE &face, nmath::scalar_t &u, nmath::scalar_t &v) const;
    nmath::Vector3f face_uv_to_direction(CUBEMAP_FACE face, nmath::scalar_t u, nmath::scalar_t v) const;

    mutable bool m_distribution_ready;
    mutable std::vector<nmath::scalar_t> m_face_cdf;
    mutable std::vector<nmath::scalar_t> m_conditional_cdf[6];
    mutable nmath::scalar_t m_total_weight;
    Texture2D m_textures[6]; // The 6 faces for the cubemap
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_CUBEMAP_H_INCLUDED */
