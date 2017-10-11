#ifndef XTCORE_AA_H_INCLUDED
#define XTCORE_AA_H_INCLUDED

#include <nmath/vector.h>
#include "tile.h"
#include "aa_sample.h"

namespace xtcore {
    namespace antialiasing {

void gen_samples_grid   (sample_set_t &samples, NMath::Vector2f pixel, size_t level);
void gen_samples_random (sample_set_t &samples, NMath::Vector2f pixel, size_t count);

enum SAMPLE_DISTRIBUTION
{
      SAMPLE_DISTRIBUTION_GRID
    , SAMPLE_DISTRIBUTION_RANDOM
};

class AA
{
    public:
    virtual void produce(xtcore::render::tile_t *tile, size_t level) = 0;
};

class MSAA : public AA
{
    public:
    MSAA();
    virtual ~MSAA();
    virtual void produce(xtcore::render::tile_t *tile, size_t level);
    SAMPLE_DISTRIBUTION distribution;
};

    } /* namespace antialiasing */
} /* namespace xtcore */

#endif /* XTCORE_AA_H_INCLUDED */
