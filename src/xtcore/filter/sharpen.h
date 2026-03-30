#ifndef XT_FILTER_SHARPEN_H_INCLUDED
#define XT_FILTER_SHARPEN_H_INCLUDED

#include <xtcore/filter.h>

namespace xtcore {
    namespace filter {

class Sharpen : public IFilter
{
    public:
    Sharpen();

    virtual void render(Pixmap *p);

    float amount;
    float radius;
    float threshold;
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_SHARPEN_H_INCLUDED */
