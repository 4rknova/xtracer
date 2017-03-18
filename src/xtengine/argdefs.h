#ifndef XTRACER_ARGDEFS_H_INCLUDED
#define XTRACER_ARGDEFS_H_INCLUDED

#define XTRACER_SETUP_DEFAULT_MAX_RDEPTH 3

#define XTRACER_ARGDEFS_PREFIX          "-"

#define XTRACER_ARGDEFS_VERSION			XTRACER_ARGDEFS_PREFIX "version"   /* Display version. */
#define XTRACER_ARGDEFS_HELP			XTRACER_ARGDEFS_PREFIX "help"	   /* Display usage information. */
#define XTRACER_ARGDEFS_RENDERER      	XTRACER_ARGDEFS_PREFIX "renderer"  /* Use GUI */
#define XTRACER_ARGDEFS_THREADS			XTRACER_ARGDEFS_PREFIX "threads"   /* Number of threads to use. */
#define XTRACER_ARGDEFS_RESOLUTION		XTRACER_ARGDEFS_PREFIX "res"	   /* Framebuffer resolution. */
#define XTRACER_ARGDEFS_MAX_RDEPTH		XTRACER_ARGDEFS_PREFIX "rdepth"    /* Maximum recursion depth. */
#define XTRACER_ARGDEFS_SAMPLES      	XTRACER_ARGDEFS_PREFIX "samples"   /* Number of samples for reflections. */
#define XTRACER_ARGDEFS_TILESIZE      	XTRACER_ARGDEFS_PREFIX "tile_size" /* Size of square tile unit to render */
#define XTRACER_ARGDEFS_ANTIALIASING	XTRACER_ARGDEFS_PREFIX "aa"	       /* Antialiasing level. */
#define XTRACER_ARGDEFS_ACTIVE_CAMERA	XTRACER_ARGDEFS_PREFIX "cam"	   /* Active camera name. */
#define XTRACER_ARGDEFS_OUTDIR			XTRACER_ARGDEFS_PREFIX "outdir"    /* Output directory. */
#define XTRACER_ARGDEFS_MOD				XTRACER_ARGDEFS_PREFIX "mod"	   /* Apply a modifier */

#endif /* XTRACER_ARGDEFS_H_INCLUDED */
