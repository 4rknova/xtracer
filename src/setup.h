#ifndef XTRACER_SETUP_H_INCLUDED
#define XTRACER_SETUP_H_INCLUDED

#include "outdrv.h"

#define XTRACER_SETUP_DEFAULT_AA				1						/* Default anti-aliasing level. */
#define XTRACER_SETUP_DEFAULT_FB_WIDTH			640						/* Default framebuffer width. */
#define XTRACER_SETUP_DEFAULT_FB_HEIGHT			480						/* Default framebuffer height. */

#define XTRACER_SETUP_DEFAULT_THREAD_COUNT		0						/* Default thread count (0: auto-detect). */

#define XTRACER_SETUP_DEFAULT_MAX_RDEPTH		3						/* Default maximum recursion depth. */

#define XTRACER_SETUP_DEFAULT_DOF_SAMPLES		1						/* Default sample count for DOF. */
#define XTRACER_SETUP_DEFAULT_LIGHT_SAMPLES		1						/* Default sample count for lights. */
#define XTRACER_SETUP_DEFAULT_REFLEC_SAMPLES	1						/* Default sample count for reflection. */

#define XTRACER_SETUP_DEFAULT_GI				false					/* Default gi flag value. */
#define XTRACER_SETUP_DEFAULT_GIVIZ             false                   /* Default giviz flag value. */
#define XTRACER_SETUP_DEFAULT_PHOTON_COUNT		100000					/* Default photon count for gi. */
#define XTRACER_SETUP_DEFAULT_PHOTON_SAMPLES	1000					/* Default photon samples. */
#define XTRACER_SETUP_DEFAULT_PHOTON_SRADIUS	1.0						/* Default photon sampling radius. */
#define XTRACER_SETUP_DEFAULT_PHOTON_POWERSC	1.0						/* Default photon power scaling factor. */

#define XTRACER_SETUP_DEFAULT_OCTREE_MAX_DEPTH	8						/* Default octree max depth. */
#define XTRACER_SETUP_DEFAULT_OCTREE_MAX_IPNDE  1						/* Default octree max items per node. */

#define XTRACER_SETUP_DEFAULT_OUTPUT			XTRACER_OUTPUT_PPM		/* Default output mode. */

#endif /* XTRACER_SETUP_H_INCLUDED */
