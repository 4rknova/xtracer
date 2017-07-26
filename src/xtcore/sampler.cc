#include "sampler.h"

#define DEFAULT_FILTERING FILTERING_NEAREST

namespace xtcore {
    namespace assets {

ISampler::ISampler()
    : filtering(DEFAULT_FILTERING)
{}

ISampler::~ISampler()
{}

    } /* namespace assets */
} /* namespace xtcore */
