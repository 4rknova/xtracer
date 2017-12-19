#include "xtcore.h"
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

int init()
{
    xtcore::pool::str::init();
    xtcore::profiler::init();
}

int deinit()
{
    xtcore::profiler::deinit();
    xtcore::pool::str::release();
}

} /* namespace xtcore */
