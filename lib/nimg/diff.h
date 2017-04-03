#ifndef NIMG_DIFF_H_INCLUDED
#define NIMG_DIFF_H_INCLUDED

#include "pixmap.h"

#define EPS 0.001

namespace nimg {
    namespace eval {

int   diff(const Pixmap &src, const Pixmap &dst, const float threshold = EPS);
float diff_euclid(const Pixmap &src, const Pixmap &dst, const float threshold = EPS);

    } /* namespace eval */
} /* namespace nimg */

#endif /* NIMG_DIFF_H_INCLUDED */
