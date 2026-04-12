#ifndef XT_FILTER_SHARPEN_H_INCLUDED
#define XT_FILTER_SHARPEN_H_INCLUDED

#include <xtcore/filter.h>

namespace xtcore {
    namespace filter {

class Sharpen : public IFilter
{
    public:
    Sharpen();

    virtual xtcore::filter::filter_metadata_t metadata() const {
        xtcore::filter::filter_metadata_t meta;
        meta.id = "sharpen";
        meta.name = "Sharpen";
        meta.description = "Local contrast enhancement for crisper edges.";
        return meta;
    }
    virtual void render(Pixmap *p);

    float amount;
    float radius;
    float threshold;
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_SHARPEN_H_INCLUDED */
