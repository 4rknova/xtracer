#include <cstddef>
#include <remotery/lib/Remotery.h>
#include "macro.h"
#include "profiler.h"

namespace xtcore {
    namespace profiler {

#if FEATURE_IS_INCLUDED(FEATURE_PROFILER)
Remotery* g_rmt;
#endif /* FEATURE_PROFILER */

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
#else
    UNUSED(message);
#endif /* FEATURE_PROFILER */
}

void start(const char *tag)
{
#if FEATURE_IS_INCLUDED(FEATURE_PROFILER)
    rmt_BeginCPUSampleDynamic(tag, 0);
#else
    UNUSED(tag);
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
#if FEATURE_IS_INCLUDED(FEATURE_PROFILER)
    rmt_DestroyGlobalInstance(g_rmt);
#endif /* FEATURE_PROFILER */
}

    } /* namespace profiler */
} /* namespace xtcore */

