#include "macro.h"
#include "emissive.h"

namespace xtcore {
    namespace asset {
        namespace material {

bool Emissive::shade(
                ColorRGBf    &intensity
        , const ICamera      *camera
        , const emitter_t    *emitter
        , const hit_record_t &hit_record) const
{
    UNUSED(camera)
    UNUSED(emitter)
    intensity = get_sample(MAT_SAMPLER_DIFFUSE , hit_record.texcoord);
    return false;
}

bool Emissive::sample_path(
            hit_result_t &hit_result
    , const hit_record_t &hit_record
) const
{
    hit_result.intensity = get_sample("emissive", hit_record.texcoord);
    return false;
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
