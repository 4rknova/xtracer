#ifndef NMATH_MUTIL_H_INCLUDED
#define NMATH_MUTIL_H_INCLUDED

#include "precision.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Angle conversion */
static inline scalar_t degree_to_radian(const scalar_t r);
static inline scalar_t radian_to_degree(const scalar_t d);

/* Clamping */
static inline scalar_t saturate(const scalar_t value);
static inline scalar_t clamp_min(const scalar_t value, const scalar_t min);
static inline scalar_t clamp_max(const scalar_t value, const scalar_t max);
static inline scalar_t clamp(const scalar_t value, const scalar_t a, const scalar_t b);

/* Check if an integer is a power of 2 */
static inline int is_power_of_2(const int v);

#ifdef __cplusplus
}   /* extern "C" */
#endif /* __cplusplus */

} /* namespace NMath */

#include "mutil.inl"

#endif /* NMATH_MUTIL_H_INCLUDED */
