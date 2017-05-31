#ifndef XTRACER_ARGDEFS_H_INCLUDED
#define XTRACER_ARGDEFS_H_INCLUDED

#define XT_SETUP_DEFAULT_MAX_RDEPTH 3

#define XT_ARG_PREFIX "-"

#define XT_ARG_RENDERER      XT_ARG_PREFIX "renderer"  /* Select renderer */
#define XT_ARG_THREADS		 XT_ARG_PREFIX "threads"   /* Number of threads to use. */
#define XT_ARG_RESOLUTION	 XT_ARG_PREFIX "res"	   /* Framebuffer resolution. */
#define XT_ARG_MAX_RDEPTH	 XT_ARG_PREFIX "rdepth"    /* Maximum recursion depth. */
#define XT_ARG_SAMPLES       XT_ARG_PREFIX "samples"   /* Number of samples for reflections. */
#define XT_ARG_TILESIZE      XT_ARG_PREFIX "tile_size" /* Size of square tile unit to render */
#define XT_ARG_ANTIALIASING	 XT_ARG_PREFIX "aa"	       /* Antialiasing level. */
#define XT_ARG_ACTIVE_CAMERA XT_ARG_PREFIX "cam"	   /* Active camera name. */
#define XT_ARG_OUTDIR		 XT_ARG_PREFIX "outdir"    /* Output directory. */
#define XT_ARG_MOD			 XT_ARG_PREFIX "mod"	   /* Apply a modifier */

#endif /* XTRACER_ARGDEFS_H_INCLUDED */
