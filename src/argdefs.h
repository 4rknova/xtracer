#ifndef XTRACER_ARGDEFS_H_INCLUDED
#define XTRACER_ARGDEFS_H_INCLUDED

#define XTRACER_ARGDEFS_VERSION				"-version"			/* Display version */
#define XTRACER_ARGDEFS_HELP				"-help"				/* Display usage information */
#define XTRACER_ARGDEFS_THREADS				"-threads"			/* Number of threads to use */
#define XTRACER_ARGDEFS_RESOLUTION			"-res"				/* Framebuffer resolution */
#define XTRACER_ARGDEFS_MAX_RDEPTH			"-rdepth"			/* Maximum recursion depth */
#define XTRACER_ARGDEFS_SAMPLES_GLOBAL		"-s_global" 		/* Number of samples for all monter carlo integrations */
#define XTRACER_ARGDEFS_SAMPLES_LIGHT		"-s_light"			/* Number of samples for area lights */
#define XTRACER_ARGDEFS_SAMPLES_DOF			"-s_dof"			/* Number of samples for depth of field */
#define XTRACER_ARGDEFS_SAMPLES_REFLEC		"-s_refl"			/* Number of samples for reflections */
#define XTRACER_ARGDEFS_ANTIALIASING		"-aa"				/* Antialiasing level */
#define XTRACER_ARGDEFS_RESUMEFILE			"-resume"			/* Resume file */
#define XTRACER_ARGDEFS_REGION				"-region"			/* Render region */
#define XTRACER_ARGDEFS_ACTIVE_CAMERA		"-cam"				/* Active camera name */
#define XTRACER_ARGDEFS_OUTDRV				"-outdrv"			/* Output mode */
#define XTRACER_ARGDEFS_OUTDIR				"-outdir"			/* Output directory */
#define XTRACER_ARGDEFS_GI					"-gi"				/* Flag to compute indirect lighting */
#define XTRACER_ARGDEFS_GIVIZ				"-giviz"			/* Flag to output a direct visualization of the photon map */
#define XTRACER_ARGDEFS_OCTREE_MAX_DEPTH	"-octree_depth"		/* Set the maximum octree depth */
#define XTRACER_ARGDEFS_OCTREE_MAX_IPN		"-octree_ipn"		/* Set the maximum octree items per node */

#define XTRACER_OUTPUT_STR_NUL 				"nul"				/* Output driver: dummy */
#define XTRACER_OUTPUT_STR_PPM 				"ppm"				/* Output driver: ppm image */

#endif /* XTRACER_ARGDEFS_H_INCLUDED */
