#ifndef XT_FILTER_H_INCLUDED
#define XT_FILTER_H_INCLUDED

#include <nimg/pixmap.h>

using nimg::Pixmap;

namespace xtcore {
    namespace filter {

class IFilter
{
    public:
    virtual void render(Pixmap *p) = 0;
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_H_INCLUDED */
