#ifndef XT_FILTER_CHROMATIC_ABERRATION_H_INCLUDED
#define XT_FILTER_CHROMATIC_ABERRATION_H_INCLUDED

#include <xtcore/filter.h>

namespace xtcore {
    namespace filter {

class ChromaticAberration : public IFilter
{
    public:
    ChromaticAberration();

    virtual xtcore::filter::filter_metadata_t metadata() const {
        xtcore::filter::filter_metadata_t meta;
        meta.id = "chromatic_aberration";
        meta.name = "Chromatic Aberration";
        meta.description = "Shifts color channels radially for lens-fringe styling.";
        return meta;
    }
    virtual void render(Pixmap *p);

    float amount;
    float center_x;
    float center_y;
    float falloff;
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_CHROMATIC_ABERRATION_H_INCLUDED */
