#ifndef XTRACER_OUTDRV_HPP_INCLUDED
#define XTRACER_OUTDRV_HPP_INCLUDED

#define XTRACER_OUTPUT_STR_NUL "nul"
#define XTRACER_OUTPUT_STR_PPM "ppm"
#define XTRACER_OUTPUT_STR_WIN "win"

enum XTRACER_OUTPUT_TYPE
{
	XTRACER_OUTPUT_NUL,		/* No output - Benchmark mode */
	XTRACER_OUTPUT_PPM		/* Output to ppm image file */
};

#endif /* XTRACER_OUTDRV_HPP_INCLUDED */
