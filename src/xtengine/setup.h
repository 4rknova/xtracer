#ifndef XTRACER_SETUP_H_INCLUDED
#define XTRACER_SETUP_H_INCLUDED

#include "outdrv.h"

#define XTRACER_SETUP_DEFAULT_AA				1						/* Default anti-aliasing level. */
#define XTRACER_SETUP_DEFAULT_FB_WIDTH			640						/* Default framebuffer width. */
#define XTRACER_SETUP_DEFAULT_FB_HEIGHT			480						/* Default framebuffer height. */

#define XTRACER_SETUP_DEFAULT_THREAD_COUNT		0						/* Default thread count (0: auto-detect). */

#define XTRACER_SETUP_DEFAULT_OCTREE_MAX_DEPTH	8						/* Default octree max depth. */
#define XTRACER_SETUP_DEFAULT_OCTREE_MAX_IPNDE  1						/* Default octree max items per node. */

#define XTRACER_SETUP_DEFAULT_OUTPUT			XTRACER_OUTPUT_PPM		/* Default output mode. */

#endif /* XTRACER_SETUP_H_INCLUDED */
