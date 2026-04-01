#ifndef XT_FILTER_BRIGHTNESS_H_INCLUDED
#define XT_FILTER_BRIGHTNESS_H_INCLUDED

#include <xtcore/filter.h>

namespace xtcore {
    namespace filter {

class Brightness : public IFilter
{
    public:
    Brightness();

    virtual xtcore::filter::filter_metadata_t metadata() const {
        xtcore::filter::filter_metadata_t meta;
        meta.id = "brightness";
        meta.name = "Brightness";
        meta.description = "Adds or subtracts uniform brightness.";
        return meta;
    }
    virtual void render(Pixmap *p);

    float amount;
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_BRIGHTNESS_H_INCLUDED */
