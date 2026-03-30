#ifndef XT_FILTER_BRIGHTNESS_H_INCLUDED
#define XT_FILTER_BRIGHTNESS_H_INCLUDED

#include <xtcore/filter.h>

namespace xtcore {
    namespace filter {

class Brightness : public IFilter
{
    public:
    Brightness();

    virtual void render(Pixmap *p);

    float amount;
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_BRIGHTNESS_H_INCLUDED */
