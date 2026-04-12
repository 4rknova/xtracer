#ifndef XT_FILTER_VIGNETTE_H_INCLUDED
#define XT_FILTER_VIGNETTE_H_INCLUDED

#include <xtcore/filter.h>

namespace xtcore {
    namespace filter {

class Vignette : public IFilter
{
    public:
    Vignette();

    virtual xtcore::filter::filter_metadata_t metadata() const {
        xtcore::filter::filter_metadata_t meta;
        meta.id = "vignette";
        meta.name = "Vignette";
        meta.description = "Darkens image edges with radial falloff.";
        return meta;
    }
    virtual void render(Pixmap *p);

    float strength;
    float radius;
    float softness;
    float center_x;
    float center_y;
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_VIGNETTE_H_INCLUDED */
