#ifndef NIMG_DIFF_H_INCLUDED
#define NIMG_DIFF_H_INCLUDED

#include "pixmap.h"

#define EPS 0.001

namespace NImg {
    namespace Operator {

int   diff(const Pixmap &src, const Pixmap &dst, const float threshold = EPS);
float diff_euclid(const Pixmap &src, const Pixmap &dst, const float threshold = EPS);

    } /* namespace Operator */
} /* namespace NImg */

#endif /* NIMG_DIFF_H_INCLUDED */
