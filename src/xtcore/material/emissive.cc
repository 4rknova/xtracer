#include "emissive.h"

namespace xtcore {
    namespace asset {
        namespace material {

bool Emissive::shade(
                ColorRGBf &intensity
        , const ICamera   *camera
        , const emitter_t *emitter
        , const HitRecord &info) const
{
    intensity = get_sample(MAT_SAMPLER_DIFFUSE , info.texcoord);
    return false;
}

bool Emissive::sample_path(
                Ray       &ray
        ,       ColorRGBf &color
        , const HitRecord &info) const
{
    color = get_sample("emissive", info.texcoord);
    return false;
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */