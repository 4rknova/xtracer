#include "license.h"
#include "config.h"

namespace xtcore {

const char *get_version()
{
    return XTCORE_VERSION;
}

const char *get_license()
{
    return __LICENSE;
}

} /* namespace xtcore */
