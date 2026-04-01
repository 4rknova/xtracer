#ifndef XT_FILTER_FILM_GRAIN_H_INCLUDED
#define XT_FILTER_FILM_GRAIN_H_INCLUDED

#include <xtcore/filter.h>

namespace xtcore {
    namespace filter {

class FilmGrain : public IFilter
{
    public:
    FilmGrain();

    virtual xtcore::filter::filter_metadata_t metadata() const {
        xtcore::filter::filter_metadata_t meta;
        meta.id = "film_grain";
        meta.name = "Film Grain";
        meta.description = "Adds stylized film-like luminance grain.";
        return meta;
    }
    virtual void render(Pixmap *p);

    float amount;
    float size;
    float seed;
    float luma_weighted;
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_FILM_GRAIN_H_INCLUDED */
