#ifndef XTRACER_AA_H_INCLUDED
#define XTRACER_AA_H_INCLUDED

#include <vector>
#include <nmath/vector.h>

namespace xtracer {
    namespace antialiasing {

typedef std::vector<NMath::Vector2f> SampleSet;

void gen_samples_noaa   (SampleSet &samples, size_t level);
void gen_samples_ssaa   (SampleSet &samples, size_t level);
void gen_samples_jitter (SampleSet &samples, size_t level);

    } /* namespace antialiasing */
} /* namespace xtracer */

#endif /* XTRACER_AA_H_INCLUDED */
