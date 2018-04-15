#include "xtcore.h"
#include "license.h"
#include "config.h"
#include "profiler.h"
#include "strpool.h"
#include "midi.h"

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
    xtcore::midi::init();
}

int deinit()
{
    xtcore::profiler::deinit();
    xtcore::pool::str::release();
    xtcore::midi::deinit();
}

} /* namespace xtcore */
