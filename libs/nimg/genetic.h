#ifndef NIMG_GENETIC_H_INCLUDED
#define NIMG_GENETIC_H_INCLUDED

#include <vector>
#include "pixmap.h"
#include "rasteriser.h"

namespace NImg {
    namespace Genetic {

int init_seed(unsigned int seed);
int tri(const Pixmap &src, Pixmap &dst, const float threshold);
int tri(const Pixmap &src, Pixmap &dst
    , std::vector<Painter::triangle_t> triangles
    , const float threshold
    , const size_t budget);

    } /* namespace Genetic */
} /* namespace NImg */

#endif /* NIMG_GENETIC_H_INCLUDED */
