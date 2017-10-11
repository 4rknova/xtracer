#include <nmath/prng.h>
#include "aa.h"

namespace xtcore {
    namespace antialiasing {

void gen_samples_grid(sample_set_t &samples, NMath::Vector2f pixel, size_t level)
{
    if (level == 0) return;

    if (level == 1) {
        sample_t s;
        s.weight = 1.f;
        s.pixel  = pixel;
        s.coords = pixel + NMath::Vector2f(0.5f, 0.5f);
        samples.push(s);
        return;
    }

    float sub_pixel = 1.f / level;
    float offset    = sub_pixel * 0.5f;

    sample_t s;
    s.pixel  = pixel;
    s.weight = 1.f / (level * level);
    for (size_t y = 0; y < level; ++y) {
        for (size_t x = 0; x < level; ++x) {
            s.coords = pixel + NMath::Vector2f(x, y) * sub_pixel + offset;
            samples.push(s);
        }
    }
}

void gen_samples_random(sample_set_t &samples, NMath::Vector2f pixel, size_t count)
{
    if (count < 1) return;

    sample_t s;
    s.weight = 1.f / count;
    s.pixel  = pixel;

    for (size_t i = 0; i < count; ++i) {
        float x = NMath::prng_c(0.0, 1.0);
        float y = NMath::prng_c(0.0, 1.0);
        s.coords = pixel + NMath::Vector2f(x, y);
        samples.push(s);
    }
}

MSAA::MSAA()
    : distribution(SAMPLE_DISTRIBUTION_GRID)
{}

MSAA::~MSAA()
{}

void MSAA::produce(xtcore::render::tile_t *tile, size_t level)
{
    if (!tile) return;

    for (size_t x = 0; x < tile->width(); ++x) {
        for (size_t y = 0; y < tile->height(); ++y) {
            sample_set_t offsets;

            NMath::Vector2f p(x + tile->x0(),y + tile->y0());

            switch (distribution) {
                case SAMPLE_DISTRIBUTION_GRID   : gen_samples_grid   (offsets, p, level); break;
                case SAMPLE_DISTRIBUTION_RANDOM : gen_samples_random (offsets, p, level); break;
            }

            while (offsets.count() > 0) {
                sample_t s;
                offsets.pop(s);
                tile->samples.push(s);
            }
        }
    }
}

    } /* namespace antialiasing */
} /* namespace xtcore */
