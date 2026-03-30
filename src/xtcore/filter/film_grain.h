#ifndef XT_FILTER_FILM_GRAIN_H_INCLUDED
#define XT_FILTER_FILM_GRAIN_H_INCLUDED

#include <xtcore/filter.h>

namespace xtcore {
    namespace filter {

class FilmGrain : public IFilter
{
    public:
    FilmGrain();

    virtual void render(Pixmap *p);

    float amount;
    float size;
    float seed;
    float luma_weighted;
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_FILM_GRAIN_H_INCLUDED */
