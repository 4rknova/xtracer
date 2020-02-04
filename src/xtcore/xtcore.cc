#include "xtcore.h"
#include "config.h"
#include "profiler.h"
#include "strpool.h"
#include "midi.h"
#include "res/license.h"

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
//xtcore::midi::init();
    return 0;
}

int deinit()
{
    xtcore::profiler::deinit();
    xtcore::pool::str::release();
    xtcore::midi::deinit();
    return 0;
}

} /* namespace xtcore */
