#include <iostream>
#include <vector>
#include <iomanip>

#include <nmath/precision.h>
#include <nmath/mutil.h>
#include <nmath/prng.h>
#include <nmath/sample.h>
#include <xtcore/math/plane.h>
#include <nimg/luminance.h>
#include <ncf/util.h>
#include <xtcore/tile.h>
#include <xtcore/aa.h>
#include <xtcore/material/boundary.h>
#include <xtcore/medium_util.h>

#include "integrator.h"

namespace xtcore {
    namespace integrator {
        namespace pathtracer {

nimg::ColorRGBf Integrator::eval(size_t depth, hit_result_t &in)
{
    if (depth == 0) return nimg::ColorRGBf(0,0,0);

    nimg::ColorRGBf radiance(0.0f, 0.0f, 0.0f);
    nimg::ColorRGBf throughput = in.intensity;
    xtcore::Ray ray = in.ray;
    nmath::scalar_t ior = in.ior;

    for (size_t bounce = 0; bounce < depth; ++bounce) {
        xtcore::hit_record_t hit_record;
        const bool hit = ctx->scene.intersection(ray, hit_record);
        const nmath::scalar_t t_surface = hit ? hit_record.t : INFINITY;

        HASH_ID medium_object_id = HASH_ID_INVALID;
        nmath::scalar_t t_exit = INFINITY;
        const xtcore::asset::medium::IMedium *medium = xtcore::medium::find_containing_medium(ctx->scene, ray.origin, medium_object_id, t_exit, ray.direction);
        if (medium) {
            const nmath::scalar_t segment_dist = std::min(t_surface, t_exit);
            const bool exits_before_surface = t_exit <= t_surface + (nmath::scalar_t)EPSILON;

            nmath::scalar_t event_dist = segment_dist;
            const bool scatter = xtcore::medium::sample_distance(*medium, ray.origin, ray.direction, segment_dist, event_dist);
            const nimg::ColorRGBf tr = xtcore::medium::transmittance(*medium, ray.origin, ray.direction, event_dist);
            const nmath::Vector3f event_pos = ray.origin + ray.direction * event_dist;
            radiance += throughput * xtcore::medium::emission(*medium, event_pos) * event_dist;
            throughput *= tr;

            if (scatter) {
                const nmath::Vector3f scatter_pos = event_pos;
                throughput *= xtcore::medium::scattering_weight(*medium, scatter_pos);
                const nmath::Vector3f new_dir = xtcore::medium::sample_hg_direction(ray.direction, medium->asymmetry());
                ray.origin = scatter_pos + new_dir * EPSILON;
                ray.direction = new_dir;
                continue;
            }

            if (exits_before_surface) {
                ray.origin = ray.origin + ray.direction * (segment_dist + (nmath::scalar_t)EPSILON);
                continue;
            }
        }

        if (!hit) {
            radiance += throughput * ctx->scene.sample_environment(ray.direction);
            break;
        }

        hit_record.ior = ior;
        const xtcore::asset::IMaterial *m = ctx->scene.get_material(hit_record.id_object);
        if (!m) break;

        xtcore::hit_result_t next_hit;
        next_hit.ior = ior;
        const bool path_continues = m->sample_path(next_hit, hit_record);
        throughput *= next_hit.intensity;
        ray = next_hit.ray;
        ior = next_hit.ior;
        if (!path_continues) break;
    }

    return radiance;
}

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->scene.get_camera(ctx->params.camera);

    while (tile->samples.count() > 0) {
        xtcore::antialiasing::sample_rgba_t aa_sample;
        tile->samples.pop(aa_sample);

        nimg::ColorRGBAf color_pixel;
        tile->read(aa_sample.pixel.x, aa_sample.pixel.y, color_pixel);

        hit_result_t hit_result;

        hit_result.intensity = ColorRGBf(1,1,1);
        hit_result.ior = 1.f;
        hit_result.ray = cam->get_primary_ray(
              aa_sample.coords.x, aa_sample.coords.y
            , (float)(ctx->params.width)
            , (float)(ctx->params.height)
        );

        color_pixel += eval(ctx->params.rdepth, hit_result) * aa_sample.weight;
        color_pixel.a(1);
        tile->write(floor(aa_sample.pixel.x), floor(aa_sample.pixel.y), color_pixel);
    }
}

        } /* namespace pathtracer */
    } /* namespace integrator */
} /* namespace xtcore */
