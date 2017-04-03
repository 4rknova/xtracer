#ifndef NIMG_GENETIC_H_INCLUDED
#define NIMG_GENETIC_H_INCLUDED

#include <vector>
#include "pixmap.h"
#include "rasteriser.h"

namespace nimg {
    namespace genetic_algorithms {

int init_seed(unsigned int seed);
int tri(const Pixmap &src, Pixmap &dst, const float threshold);
int tri(const Pixmap &src, Pixmap &dst
    , std::vector<painter::triangle_t> triangles
    , const float threshold
    , const size_t budget);

    } /* namespace genetic_algorithms */
} /* namespace nimg */

#endif /* NIMG_GENETIC_H_INCLUDED */
