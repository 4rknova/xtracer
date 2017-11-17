#include <cstddef>
#include <cmath>
#include <vector>
#include "luminance.h"
#include "diff.h"

namespace nimg {
    namespace eval {

int diff(const Pixmap &src, const Pixmap &dst, const float threshold)
{
    if (    src.width()  != dst.width()
        ||  src.height() != dst.height()
    ) return -1;

    int res = 0;

    for (size_t j = 0; j < src.height(); ++j) {
        for (size_t i = 0; i < src.width(); ++i) {

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

float diff_euclid(const Pixmap &src, const Pixmap &dst, const float threshold)
{
    if (    src.width()  != dst.width()
        ||  src.height() != dst.height()
    ) return -1;

    std::vector<float> val;

    for (size_t j = 0; j < src.height(); ++j) {
        for (size_t i = 0; i < src.width(); ++i) {

            ColorRGBAf val_src = src.pixel_ro(i, j);
            ColorRGBAf val_dst = dst.pixel_ro(i, j);

            float a = luminance(val_src);
            float b = luminance(val_dst);

            float res = fabs(a - b);

            val.push_back(res);
        }
    }

    std::vector<float>::iterator it = val.begin();
    std::vector<float>::iterator et = val.end();

    float total = 0;
    for (; it != et; ++it) {
        total += (*it);
    }

    float res = sqrtf(total);

    return res;
}

    } /* namespace eval */
} /* namespace nimg */
