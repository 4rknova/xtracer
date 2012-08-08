#ifndef XTRACER_SETUP_H_INCLUDED
#define XTRACER_SETUP_H_INCLUDED

#include "outdrv.h"

#define XTRACER_SETUP_DEFAULT_FB_WIDTH			640						/* Default framebuffer width. */
#define XTRACER_SETUP_DEFAULT_FB_HEIGHT			480						/* Default framebuffer height. */
#define XTRACER_SETUP_DEFAULT_THREAD_COUNT		0						/* Default thread count (0: auto-detect). */
#define XTRACER_SETUP_DEFAULT_MAX_RDEPTH		10						/* Default maximum recursion depth. */
#define XTRACER_SETUP_DEFAULT_DOF_SAMPLES		1						/* Default sample count for DOF. */
#define XTRACER_SETUP_DEFAULT_LIGHT_SAMPLES		1						/* Default sample count for lights. */
#define XTRACER_SETUP_DEFAULT_REFLEC_SAMPLES	1						/* Default sample count for reflection. */
#define XTRACER_SETUP_DEFAULT_AA				1						/* Default anti-aliasing level. */
#define XTRACER_SETUP_DEFAULT_OUTPUT			XTRACER_OUTPUT_PPM		/* Default output mode. */

#endif /* XTRACER_SETUP_H_INCLUDED */
