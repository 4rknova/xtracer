#ifndef NMATH_QUADRATIC_H_INCLUDED
#define NMATH_QUADRATIC_H_INCLUDED

#include <algorithm>
#include "precision.h"

namespace nmath {

bool solve_quadratic(const scalar_t a, const scalar_t b, const scalar_t c, scalar_t &x0, scalar_t &x1);

} /* namespace nmath */

#endif /* NMATH_QUADRATIC_H_INCLUDED */
