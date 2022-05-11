#include <algorithm>
#include "quadratic.h"

namespace nmath {

bool solve_quadratic(const scalar_t a, const scalar_t b, const scalar_t c, scalar_t &x0, scalar_t &x1)
{
    scalar_t d = b * b - 4.f * a * c;

         if (d <  0.f) return false;
    else if (d == 0.f) x0 = x1 = - 0.5 * b / a;
    else {
        scalar_t q = (b > 0)
            ? -0.5 * (b + nmath_sqrt(d))
            : -0.5 * (b - nmath_sqrt(d));

        x0 = q / a;
        x1 = c / q;
    }

    if (x0 > x1) std::swap(x0, x1);

    return true;
}

} /* namespace nmath */
