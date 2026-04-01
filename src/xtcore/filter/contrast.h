#ifndef XT_FILTER_CONTRAST_H_INCLUDED
#define XT_FILTER_CONTRAST_H_INCLUDED

#include <xtcore/filter.h>

namespace xtcore {
    namespace filter {

class Contrast : public IFilter
{
    public:
    Contrast();

    virtual xtcore::filter::filter_metadata_t metadata() const {
        xtcore::filter::filter_metadata_t meta;
        meta.id = "contrast";
        meta.name = "Contrast";
        meta.description = "Adjusts contrast around a configurable pivot.";
        return meta;
    }
    virtual void render(Pixmap *p);

    float amount;
    float pivot;
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_CONTRAST_H_INCLUDED */
