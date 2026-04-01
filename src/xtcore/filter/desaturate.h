#ifndef XT_FILTER_DESATURATE_H_INCLUDED
#define XT_FILTER_DESATURATE_H_INCLUDED

#include "filter.h"

namespace xtcore {
    namespace filter {

class Desaturate : public IFilter
{
    public:
    virtual xtcore::filter::filter_metadata_t metadata() const {
        xtcore::filter::filter_metadata_t meta;
        meta.id = "desaturate";
        meta.name = "Desaturate";
        meta.description = "Reduces image saturation toward grayscale.";
        return meta;
    }
    virtual void render(Pixmap *p);
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_DESATURATE_H_INCLUDED */
