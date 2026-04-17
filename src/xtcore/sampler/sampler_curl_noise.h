#ifndef XTCORE_SAMPLER_CURL_NOISE_H_INCLUDED
#define XTCORE_SAMPLER_CURL_NOISE_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class CurlNoise : public ISampler
{
    public:
    CurlNoise();
    virtual ~CurlNoise();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nimg::ColorRGBf color_a;
    nimg::ColorRGBf color_b;
    nmath::scalar_t scale;
    nmath::scalar_t strength;
    int octaves;
    nmath::scalar_t lacunarity;
    nmath::scalar_t gain;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_CURL_NOISE_H_INCLUDED */
