#ifndef XTCORE_AA_H_INCLUDED
#define XTCORE_AA_H_INCLUDED

#include <nmath/vector.h>
#include "tile.h"
#include "aa_sample.h"

namespace xtcore {
    namespace antialiasing {

void gen_samples_grid   (sample_set_t &samples, nmath::Vector2f pixel, size_t level);
void gen_samples_random (sample_set_t &samples, nmath::Vector2f pixel, size_t level);

enum SAMPLE_DISTRIBUTION
{
      SAMPLE_DISTRIBUTION_GRID
    , SAMPLE_DISTRIBUTION_RANDOM
};

void produce(xtcore::render::tile_t *tile, SAMPLE_DISTRIBUTION sd, size_t level, size_t samples);

    } /* namespace antialiasing */
} /* namespace xtcore */

#endif /* XTCORE_AA_H_INCLUDED */
