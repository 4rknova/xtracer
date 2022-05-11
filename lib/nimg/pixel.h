#ifndef NIMG_PIXEL_H_INCLUDED
#define NIMG_PIXEL_H_INCLUDED

#include <stdint.h>
#include "precision.h"
#include "defs.h"

namespace nimg {
    namespace util {

#ifdef __cplusplus
	extern "C" {
#endif  /* __cplusplus */

/*
	NOTES:
	All the functions below operate on a big endian format.
*/

typedef uint32_t pixel32_t;

static inline pixel32_t rgba_c_to_pixel32(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
static inline pixel32_t rgba_f_to_pixel32(scalar_t r, scalar_t g, scalar_t b, scalar_t a);

static inline unsigned char get_pixel32_r(pixel32_t c);
static inline unsigned char get_pixel32_g(pixel32_t c);
static inline unsigned char get_pixel32_b(pixel32_t c);
static inline unsigned char get_pixel32_a(pixel32_t c);

#ifdef __cplusplus
	}   /* extern "C" */
#endif /* __cplusplus */

    } /* namespace util */
} /* namespace nimg */

#include "pixel.inl"

#endif /* XTRACER_PIXEL_H_INCLUDED */
