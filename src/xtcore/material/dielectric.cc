#include <nmath/sample.h>
#include "macro.h"
#include "lambert.h"

namespace xtcore {
    namespace asset {
        namespace material {

bool Dielectric::shade(
            ColorRGBf    &intensity
    , const ICamera      *camera
    , const emitter_t    *emitter
    , const hit_record_t &hit_record) const
{
    UNUSED(camera)
    UNUSED(emitter)
    intensity = ColorRGBf(0.f,0.f,0.f);
    return true;
}

bool Dielectric::sample_path(
            hit_result_t &hit_result
    , const hit_record_t &hit_record
) const
{
    float mat_ior = get_scalar("ior");
    hit_result.ray.origin    = hit_record.point + hit_record.normal * EPSILON;
    hit_result.ray.direction = hit_record.incident_direction.refracted(hit_record.normal, hit_result.ior, mat_ior);
    return true;
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
