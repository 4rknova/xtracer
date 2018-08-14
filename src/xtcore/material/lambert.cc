#include <nmath/sample.h>
#include "macro.h"
#include "lambert.h"

namespace xtcore {
    namespace asset {
        namespace material {

bool Lambert::shade(
            ColorRGBf &intensity
    , const ICamera   *camera
    , const emitter_t *emitter
    , const HitRecord &info) const
{
    UNUSED(camera)

    Vector3f light_dir = emitter->position - info.point;
    light_dir.normalize();

    NMath::scalar_t d = dot(light_dir, info.normal);

    if (d > 0) {
        intensity += emitter->intensity *
               d * get_sample(MAT_SAMPLER_DIFFUSE , info.texcoord);
    }

    return true;
}

bool Lambert::sample_path(
            Ray       &ray
    ,       ColorRGBf &color
    , const HitRecord &info) const
{
    ray.origin    = info.point + info.normal * EPSILON;
    ray.direction = NMath::Sample::diffuse(info.normal);
    color         = get_sample("diffuse", info.texcoord);
    return true;
}

        } /* namespace material */
    } /* namespace asset */
} /* namespace xtcore */
