#include <nmath/prng.h>
#include "aa.h"

namespace xtracer {
    namespace antialiasing {

void gen_samples_noaa(SampleSet &samples, size_t level)
{
    samples.clear();
    NMath::Vector2f v(0.5f, 0.5f);
    samples.push_back(v);
}

void gen_samples_ssaa(SampleSet &samples, size_t level)
{
    samples.clear();

    if (level < 2) {
        NMath::Vector2f v(0.5f, 0.5f);
        samples.push_back(v);
        return;
    }

    float sub_pixel = 1.f / level;
    float offset    = sub_pixel * 0.5f;

    for (size_t x = 0; x < level; ++x) {
        for (size_t y = 0; y < level; ++y) {
            NMath::Vector2f uv = NMath::Vector2f(x, y) * sub_pixel + offset;
            samples.push_back(uv);
        }
    }
}

void gen_samples_msaa(SampleSet &samples, size_t level)
{
    samples.clear();

    if (level < 1) level = 1;

    for (size_t i = 0; i < level; ++i) {
        float x = NMath::prng_c(0.0, 1.0);
        float y = NMath::prng_c(0.0, 1.0);
        NMath::Vector2f uv(x, y);
        samples.push_back(uv);
    }
}

    } /* namespace antialiasing */
} /* namespace xtracer */
