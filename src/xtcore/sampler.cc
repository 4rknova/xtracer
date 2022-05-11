#include "sampler.h"

#define DEFAULT_FILTERING FILTERING_BILINEAR

namespace xtcore {
    namespace sampler {

ISampler::ISampler()
    : filtering(DEFAULT_FILTERING)
{}

ISampler::~ISampler()
{}

    } /* namespace sampler */
} /* namespace xtcore */
