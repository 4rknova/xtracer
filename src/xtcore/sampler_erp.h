#ifndef XTCORE_SAMPLER_ERP_H_INCLUDED
#define XTCORE_SAMPLER_ERP_H_INCLUDED

#include <nmath/vector.h>
#include "sampler_tex.h"
#include "sampler.h"

namespace xtcore {
    namespace sampler {

class ERP : public ISampler
{
    public:
    int load(const char *file);

    nimg::ColorRGBf sample(const nmath::Vector3f &tc) const;

    private:
    Texture2D m_texture;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_ERP_H_INCLUDED */
