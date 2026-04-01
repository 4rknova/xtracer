#ifndef XT_FILTER_FXAA_H_INCLUDED
#define XT_FILTER_FXAA_H_INCLUDED

#include <xtcore/filter.h>

namespace xtcore {
    namespace filter {

class FXAA : public IFilter
{
    public:
    FXAA();

    virtual xtcore::filter::filter_metadata_t metadata() const {
        xtcore::filter::filter_metadata_t meta;
        meta.id = "fxaa";
        meta.name = "FXAA";
        meta.description = "Fast approximate anti-aliasing post filter.";
        return meta;
    }
    virtual void render(Pixmap *p);

    float subpix;
    float edge_threshold;
    float edge_threshold_min;
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_FXAA_H_INCLUDED */
