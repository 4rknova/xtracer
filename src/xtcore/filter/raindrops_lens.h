#ifndef XT_FILTER_RAINDROPS_LENS_H_INCLUDED
#define XT_FILTER_RAINDROPS_LENS_H_INCLUDED

#include <xtcore/filter.h>

namespace xtcore {
    namespace filter {

class RaindropsLens : public IFilter
{
    public:
    RaindropsLens();

    virtual xtcore::filter::filter_metadata_t metadata() const {
        xtcore::filter::filter_metadata_t meta;
        meta.id = "raindrops_lens";
        meta.name = "Raindrops on Lens";
        meta.description = "Applies stylized raindrop lens distortion.";
        return meta;
    }
    virtual void render(Pixmap *p);

    float density;
    float size;
    float distortion;
    float seed;
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_RAINDROPS_LENS_H_INCLUDED */
