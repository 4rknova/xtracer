#ifndef XT_FILTER_CONTRAST_H_INCLUDED
#define XT_FILTER_CONTRAST_H_INCLUDED

#include <xtcore/filter.h>

namespace xtcore {
    namespace filter {

class Contrast : public IFilter
{
    public:
    Contrast();

    virtual void render(Pixmap *p);

    float amount;
    float pivot;
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_CONTRAST_H_INCLUDED */
