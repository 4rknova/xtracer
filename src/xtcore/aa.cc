#include <nmath/prng.h>
#include "aa.h"

namespace xtcore {
    namespace antialiasing {

void gen_samples_grid(sample_set_t &samples, NMath::Vector2f pixel, size_t level)
{
    if (level == 0) return;

    if (level == 1) {
        sample_rgba_t s;
        s.weight = 1.f;
        s.pixel  = pixel;
        s.coords = pixel + NMath::Vector2f(0.5f, 0.5f);
        samples.push(s);
        return;
    }

    float sub_pixel = 1.f / level;
    float offset    = sub_pixel * 0.5f;

    sample_rgba_t s;
    s.pixel  = pixel;
    s.weight = 1.f / (level * level);
    for (size_t y = 0; y < level; ++y) {
        for (size_t x = 0; x < level; ++x) {
            s.coords = pixel + NMath::Vector2f(x, y) * sub_pixel + offset;
            samples.push(s);
        }
    }
}

void gen_samples_random(sample_set_t &samples, NMath::Vector2f pixel, size_t level)
{
    if (level < 1) return;

    sample_rgba_t s;
    s.weight = 1.f / level;
    s.pixel  = pixel;

    for (size_t i = 0; i < level; ++i) {
        float x = NMath::prng_c(0.0, 1.0);
        float y = NMath::prng_c(0.0, 1.0);
        s.coords = pixel + NMath::Vector2f(x, y);
        samples.push(s);
    }
}


void produce(xtcore::render::tile_t *tile, SAMPLE_DISTRIBUTION sd, size_t aa_level, size_t samples)
{
    if (!tile) return;

    for (size_t x = 0; x < tile->width(); ++x) {
        for (size_t y = 0; y < tile->height(); ++y) {
            sample_set_t offsets;

            NMath::Vector2f p(x + tile->x0(),y + tile->y0());

            switch (sd) {
                case SAMPLE_DISTRIBUTION_GRID   : gen_samples_grid   (offsets, p, aa_level); break;
                case SAMPLE_DISTRIBUTION_RANDOM : gen_samples_random (offsets, p, aa_level); break;
            }

            while (offsets.count() > 0) {
                sample_rgba_t s;
                offsets.pop(s);
                s.weight *= 1./samples;

                for (size_t i = 0; i < samples; ++i) {
                    tile->samples.push(s);
                }
            }
        }
    }
}

    } /* namespace antialiasing */
} /* namespace xtcore */
