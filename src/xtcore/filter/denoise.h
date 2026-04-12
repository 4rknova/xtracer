#ifndef XT_FILTER_DENOISE_H_INCLUDED
#define XT_FILTER_DENOISE_H_INCLUDED

#include <xtcore/filter.h>

namespace xtcore {
    namespace filter {

class Denoise : public IFilter
{
    public:
    Denoise();

    virtual xtcore::filter::filter_metadata_t metadata() const {
        xtcore::filter::filter_metadata_t meta;
        meta.id = "denoise";
        meta.name = "Bilateral Denoise";
        meta.description = "Beauty-buffer bilateral denoiser for HDR cleanup.";
        return meta;
    }
    virtual void render(Pixmap *p);

    float strength;
    float radius;
    float sigma;
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_DENOISE_H_INCLUDED */
