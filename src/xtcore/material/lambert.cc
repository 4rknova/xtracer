#include <nmath/sample.h>
#include "macro.h"
#include "lambert.h"

namespace xtcore {
    namespace asset {
        namespace material {

bool Lambert::shade(
            ColorRGBf    &intensity
    , const ICamera      *camera
    , const emitter_t    *emitter
    , const hit_record_t &hit_record) const
{
    UNUSED(camera)

    Vector3f light_dir = emitter->position - hit_record.point;
    light_dir.normalize();

    nmath::scalar_t d = dot(light_dir, hit_record.normal);

    if (d > 0) {
        intensity += emitter->intensity *
               d * get_sample(MAT_SAMPLER_DIFFUSE , hit_record.texcoord);
    }

    return true;
}

bool Lambert::sample_path(
            hit_result_t &hit_result
    , const hit_record_t &hit_record
) const
{
    hit_result.ray.origin    = hit_record.point + hit_record.normal * EPSILON;
    hit_result.ray.direction = nmath::sample::diffuse(hit_record.normal).normalized();
    hit_result.intensity     = get_sample("diffuse", hit_record.texcoord)
                             * dot(hit_record.normal, hit_result.ray.direction);
    return true;
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
