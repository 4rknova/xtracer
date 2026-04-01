#ifndef XTRACER_FRONTEND_WEB_POST_FILTERS_H_INCLUDED
#define XTRACER_FRONTEND_WEB_POST_FILTERS_H_INCLUDED

#include <string>
#include <vector>

#include <xtcore/filter.h>

namespace xtracer {
namespace frontend {
namespace web {

struct post_filter_param_option_t
{
    const char *value;
    const char *label;
};

struct post_filter_param_info_t
{
    const char *id;
    const char *label;
    const char *type;
    const char *description;
    const char *default_value;
    const char *min_value;
    const char *max_value;
    const char *step_value;
    const post_filter_param_option_t *options;
    size_t options_count;
};

struct post_filter_info_t
{
    xtcore::filter::filter_metadata_t metadata;
    const post_filter_param_info_t *params;
    size_t params_count;
    bool allow_before_tm;
    bool allow_after_tm;
};

std::vector<post_filter_info_t> list_post_filters();
const post_filter_info_t *find_post_filter_info(const std::string &id);

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */

#endif /* XTRACER_FRONTEND_WEB_POST_FILTERS_H_INCLUDED */
