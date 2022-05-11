#ifndef XT_PROFILER_H_INCLUDED
#define XT_PROFILER_H_INCLUDED

#include "config.h"

namespace xtcore {
    namespace profiler {

void init();
void log(const char *tag);
void start(const char *message);
void end();
void deinit();

    } /* namespace profiler */
} /* namespace xtcore */

#endif /* XT_PROFILER_H_INCLUDED */
