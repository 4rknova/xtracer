#ifndef NMATH_PRNG_H_INCLUDED
#define NMATH_PRNG_H_INCLUDED

#include "vector.h"
#include "types.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* returns a random number between min and max using the C built-in PRNG in uniform manner */
static inline scalar_t prng_c(const scalar_t a, const scalar_t b);

/* Multiply with carry method by George Marsaglia */
static inline scalar_t prng_multiplyWithCarry(const scalar_t a, const scalar_t b);

#ifdef __cplusplus
}   /* extern "C" */
#endif

} /* namespace NMath */

#include "prng.inl"

#endif /* NMATH_PRNG_H_INCLUDED */
