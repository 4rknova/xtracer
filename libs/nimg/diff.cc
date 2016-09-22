#include <cmath>
#include "diff.h"

namespace NImg {
    namespace Operator {

int diff(const Pixmap &src, const Pixmap &dst, const float threshold)
{
    if (    src.width()  != dst.width()
        ||  src.height() != dst.height()
    ) return -1;

    int res = 0;

    for (unsigned int j = 0; j < src.height(); ++j) {
        for (unsigned int i = 0; i < src.width(); ++i) {

            ColorRGBAf val_src = src.pixel_ro(i, j);
            ColorRGBAf val_dst = dst.pixel_ro(i, j);

            ColorRGBAf dist = val_src - val_dst;

            if (   fabs(dist.r()) > threshold
                || fabs(dist.g()) > threshold
                || fabs(dist.b()) > threshold
                || fabs(dist.a()) > threshold
            ) ++res;
        }
    }

    return res;
}

    } /* namespace Operator */
} /* namespace NImg */
