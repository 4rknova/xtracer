#ifndef NMATH_INTERPOLATION_H_INCLUDED
#define NMATH_INTERPOLATION_H_INCLUDED

#include "types.h"

namespace nmath {
	namespace interpolation {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* 
	Smoothstep

	Notes:
	The inputs are values edge0, edge1, x with edge0 < x < edge1.
	Unlike the rest interpolation methods, the returned value 
	for smoothstep is in the 0 - 1 range.
*/
static inline scalar_t smoothstep(const scalar_t edge0, const scalar_t edge1, const scalar_t x);
static inline scalar_t smoothstep_perlin(const scalar_t edge0, const scalar_t edge1, const scalar_t x);

/* Simple */
static inline scalar_t step(const scalar_t a, const scalar_t b, const scalar_t t);
static inline scalar_t linear(const scalar_t a, const scalar_t b, const scalar_t t);
static inline scalar_t cosine(const scalar_t a, const scalar_t b, const scalar_t t);
static inline scalar_t acceleration(const scalar_t a, const scalar_t b, const scalar_t t);
static inline scalar_t deceleration(const scalar_t a, const scalar_t b, const scalar_t t);

/* 
	Splines

	Notes:
	We are interpolating between the 2 middle values.
*/
static inline scalar_t cubic(const scalar_t a, const scalar_t b, const scalar_t c, const scalar_t d, const scalar_t t);
static inline scalar_t hermite(const scalar_t tang1, const scalar_t a, const scalar_t b, const scalar_t tang2, const scalar_t t);
static inline scalar_t cardinal(const scalar_t a, const scalar_t b, const scalar_t c, const scalar_t d, const scalar_t p, const scalar_t t);
static inline scalar_t catmullrom(const scalar_t a, const scalar_t b, const scalar_t c, const scalar_t d, const scalar_t t);

/*
	Bezier

	Notes:
	We are interpolating between the first and last values.
*/
static inline scalar_t bezier_quadratic(const scalar_t a, const scalar_t b, const scalar_t c, const scalar_t t);
static inline scalar_t bezier_cubic(const scalar_t a, const scalar_t b, const scalar_t c, const scalar_t d, const scalar_t t);

#ifdef __cplusplus
}   /* extern "C" */
#endif

	} /* namespace interpolation */
} /* namespace nmath */

#include "interpolation.inl"

#endif /* NMATH_INTERPOLATION_H_INCLUDED */
