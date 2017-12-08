#include <cstddef>
#include <remotery/lib/Remotery.h>
#include "profiler.h"

namespace xtcore {
    namespace profiler {

Remotery* g_rmt;

void init()
{
#if FEATURE_IS_INCLUDED(FEATURE_PROFILER)
    rmt_CreateGlobalInstance(&g_rmt);
#endif /* FEATURE_PROFILER */

}

void log(const char *message)
{
#if FEATURE_IS_INCLUDED(FEATURE_PROFILER)
    if (message) rmt_LogText(message);
#endif /* FEATURE_PROFILER */
}

void start(const char *tag)
{
#if FEATURE_IS_INCLUDED(FEATURE_PROFILER)
    rmt_BeginCPUSampleDynamic(tag, 0);
#endif /* FEATURE_PROFILER */
}

void end()
{
#if FEATURE_IS_INCLUDED(FEATURE_PROFILER)
    rmt_EndCPUSample();
#endif /* FEATURE_PROFILER */
}

void deinit()
{
    rmt_DestroyGlobalInstance(g_rmt);
}

    } /* namespace profiler */
} /* namespace xtcore */

