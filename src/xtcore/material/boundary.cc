#include "macro.h"
#include "boundary.h"

namespace xtcore {
    namespace asset {
        namespace material {

bool Boundary::shade(
            ColorRGBf    &intensity
    , const ICamera      *camera
    , const emitter_t    *emitter
    , const hit_record_t &hit_record) const
{
    UNUSED(camera)
    UNUSED(emitter)
    UNUSED(hit_record)
    intensity = ColorRGBf(0.0f, 0.0f, 0.0f);
    return true;
}

bool Boundary::sample_path(
            hit_result_t &hit_result
    , const hit_record_t &hit_record
) const
{
    hit_result.ray.direction = hit_record.incident_direction.normalized();
    hit_result.ray.origin = hit_record.point + hit_result.ray.direction * EPSILON;
    hit_result.intensity = ColorRGBf(1.0f, 1.0f, 1.0f);
    hit_result.ior = hit_record.ior;
    return true;
}

bool Boundary::bsdf_is_delta() const
{
    return true;
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
