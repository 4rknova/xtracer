#ifndef NMATH_PRIME_H_INCLUDED
#define NMATH_PRIME_H_INCLUDED

#include "defs.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

static inline int isPrime(unsigned long i);
static inline unsigned long getNextPrime(unsigned long i);
static inline unsigned long getPrevPrime(unsigned long i);

#ifdef __cplusplus
}   /* extern "C" */
#endif

} /* namespace NMath */

#include "prime.inl"

#endif /* NMATH_PRIME_H_INCLUDED */
