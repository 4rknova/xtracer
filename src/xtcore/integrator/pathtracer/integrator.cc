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
    HASH_ID current_medium_object_id = HASH_ID_INVALID;
    nmath::scalar_t current_medium_exit = INFINITY;
    const xtcore::asset::medium::IMedium *current_medium =
        xtcore::medium::find_containing_medium(*ctx->scene, ray.origin, current_medium_object_id, current_medium_exit, ray.direction);

    for (size_t bounce = 0; bounce < depth; ++bounce) {
        xtcore::hit_record_t hit_record;
        const bool hit = ctx->scene->intersection(ray, hit_record);
        const nmath::scalar_t t_surface = hit ? hit_record.t : INFINITY;

        nmath::scalar_t t_exit = INFINITY;
        const xtcore::asset::medium::IMedium *medium = current_medium;
        if (medium) {
            if (!xtcore::medium::distance_to_medium_boundary(*ctx->scene, current_medium_object_id, ray.origin, ray.direction, t_exit)) {
                current_medium_object_id = HASH_ID_INVALID;
                current_medium_exit = INFINITY;
                medium = 0;
                current_medium = 0;
            } else {
                current_medium_exit = t_exit;
            }
        }
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
                current_medium_exit = std::max((nmath::scalar_t)0.0, t_exit - event_dist);
                continue;
            }

            if (exits_before_surface) {
                ray.origin = ray.origin + ray.direction * (segment_dist + (nmath::scalar_t)EPSILON);
                current_medium_object_id = HASH_ID_INVALID;
                current_medium_exit = INFINITY;
                current_medium = 0;
                continue;
            }
        }

        if (!hit) {
            radiance += throughput * ctx->scene->sample_environment(ray.direction);
            break;
        }

        hit_record.ior = ior;
        const xtcore::asset::IMaterial *m = ctx->scene->get_material(hit_record.id_object);
        if (!m) break;
        const xtcore::asset::medium::IMedium *boundary_medium = ctx->scene->get_object_medium(hit_record.id_object);
        const xtcore::asset::material::Boundary *boundary = dynamic_cast<const xtcore::asset::material::Boundary *>(m);

        xtcore::hit_result_t next_hit;
        next_hit.ior = ior;
        const bool path_continues = m->sample_path(next_hit, hit_record);
        throughput *= next_hit.intensity;
        ray = next_hit.ray;
        ior = next_hit.ior;

        if (boundary && boundary_medium) {
            const nmath::scalar_t medium_side = nmath::dot(hit_record.incident_direction.normalized(), hit_record.normal.normalized());
            if (medium_side < (nmath::scalar_t)0.0) {
                current_medium_object_id = hit_record.id_object;
                current_medium = boundary_medium;
                if (!xtcore::medium::distance_to_medium_boundary(*ctx->scene, current_medium_object_id, ray.origin, ray.direction, current_medium_exit)) {
                    current_medium_object_id = HASH_ID_INVALID;
                    current_medium_exit = INFINITY;
                    current_medium = 0;
                }
            } else if (current_medium_object_id == hit_record.id_object) {
                current_medium_object_id = HASH_ID_INVALID;
                current_medium_exit = INFINITY;
                current_medium = 0;
            }
        }

        if (!path_continues) break;
    }

    return radiance;
}

void Integrator::render_tile(xtcore::render::tile_t *tile)
{
    xtcore::asset::ICamera *cam = ctx->active_camera();

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
