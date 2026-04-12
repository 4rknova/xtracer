#ifndef XT_FILTER_H_INCLUDED
#define XT_FILTER_H_INCLUDED

#include <string>
#include <nimg/pixmap.h>

using nimg::Pixmap;

namespace xtcore {
    namespace filter {

enum filter_status_t
{
    FILTER_STATUS_STABLE = 0,
    FILTER_STATUS_EXPERIMENTAL,
    FILTER_STATUS_LEGACY,
    FILTER_STATUS_HIDDEN
};

struct filter_metadata_t
{
    std::string id;
    std::string name;
    filter_status_t status;
    std::string description;
    std::string replacement_id;

    filter_metadata_t()
        : id()
        , name()
        , status(FILTER_STATUS_STABLE)
        , description()
        , replacement_id()
    {}
};

class IFilter
{
    public:
    virtual ~IFilter() {}
    virtual filter_metadata_t metadata() const = 0;
    virtual void render(Pixmap *p) = 0;
};

    } /* namespace filter */
} /* namespace xtcore */

#endif /* XT_FILTER_H_INCLUDED */
