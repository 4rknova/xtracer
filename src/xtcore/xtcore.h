#ifndef XT_XTCORE_H_INCLUDED
#define XT_XTCORE_H_INCLUDED

#include "config.h"
#include "proto.h"
#include "scene.h"
#include "timeutil.h"
#include "log.h"
#include "context.h"
#include "parseutil.h"
#include "log.h"
#include "strpool.h"
#include "profiler.h"

#include "renderer/photon_mapper/renderer.h"
#include "renderer/stencil/renderer.h"
#include "renderer/depth/renderer.h"

namespace xtcore {

const char *get_version();
const char *get_license();

int init();   // Initialize subsystems
int deinit(); // Release subsystems

} /* namespace xtcore */

#endif /* XT_XTCORE_H_INCLUDED */
