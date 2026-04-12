#ifndef XTCORE_SAMPLER_ERP_H_INCLUDED
#define XTCORE_SAMPLER_ERP_H_INCLUDED

#include <nmath/vector.h>
#include <vector>
#include "sampler_tex.h"
#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class ERP : public ISampler
{
    public:
    ERP();
    int load(const char *file);

    nimg::ColorRGBf sample(const nmath::Vector3f &tc) const;
    bool sample_direction(nmath::Vector3f &direction, nmath::scalar_t &pdf, nimg::ColorRGBf &radiance) const;
    nmath::scalar_t pdf_direction(const nmath::Vector3f &direction) const;

    private:
    void build_distribution() const;
    bool sample_texel(size_t &x, size_t &y, nmath::scalar_t &pmf) const;
    nmath::scalar_t texel_pmf(size_t x, size_t y) const;

    mutable bool m_distribution_ready;
    mutable std::vector<nmath::scalar_t> m_row_cdf;
    mutable std::vector<nmath::scalar_t> m_conditional_cdf;
    mutable nmath::scalar_t m_total_weight;
    Texture2D m_texture;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_ERP_H_INCLUDED */
