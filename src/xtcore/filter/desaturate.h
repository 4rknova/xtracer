#ifndef XT_FILTER_DESATURATE_H_INCLUDED
#define XT_FILTER_DESATURATE_H_INCLUDED

#include "filter.h"

namespace xtcore {
    namespace filter {

class Desaturate : public IFilter
{
    public:
    virtual void render(Pixmap *p);
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_DESATURATE_H_INCLUDED */
