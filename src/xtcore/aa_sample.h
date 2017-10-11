#ifndef XTCORE_AA_SAMPLEH_INCLUDED
#define XTCORE_AA_SAMPLEH_INCLUDED

#include <queue>
#include <nmath/vector.h>

namespace xtcore {
    namespace antialiasing {

struct sample_t
{
    NMath::scalar_t weight; // Contribution
    NMath::Vector2f pixel;  // Pixel indices
    NMath::Vector2f coords; // Sample coordinates
};

struct sample_set_t {

    void   push(sample_t &s);
    void   pop (sample_t &s);
    void   clear();
    size_t count();

    private:
    std::queue<sample_t> m_samples;
};

    } /* namespace antialiasing */
} /* namespace xtcore */

#endif /* XTCORE_AA_SAMPLEH_INCLUDED */
