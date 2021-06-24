#ifndef XTCORE_AA_SAMPLE_H_INCLUDED
#define XTCORE_AA_SAMPLE_H_INCLUDED

#include <queue>
#include <nmath/vector.h>
#include <nimg/color.h>

namespace xtcore {
    namespace antialiasing {

struct sample_rgba_t
{
    NMath::scalar_t  weight;   // Contribution
    NMath::Vector2f  pixel;    // Pixel indices
    NMath::Vector2f  coords;   // Sample coordinates

    size_t           priority; // Layer
    nimg::ColorRGBAf value;    // Rendered value
};

struct sample_set_t {

    void   push(sample_rgba_t &s);
    void   pop (sample_rgba_t &s);
    void   clear();
    size_t count();

    private:
    std::queue<sample_rgba_t> m_samples;
};

    } /* namespace antialiasing */
} /* namespace xtcore */

#endif /* XTCORE_AA_SAMPLE_H_INCLUDED */
