#ifndef XTCORE_RESOLUTION_PRESET_H_INCLUDED
#define XTCORE_RESOLUTION_PRESET_H_INCLUDED

#include <stddef.h>

namespace xtcore {
namespace render {

struct resolution_preset_t
{
    size_t width;
    size_t height;
    const char *description;
};

const resolution_preset_t *resolution_presets(size_t &count);

} /* namespace render */
} /* namespace xtcore */

#endif /* XTCORE_RESOLUTION_PRESET_H_INCLUDED */
