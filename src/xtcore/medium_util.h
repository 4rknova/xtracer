#ifndef XTCORE_MEDIUM_UTIL_H_INCLUDED
#define XTCORE_MEDIUM_UTIL_H_INCLUDED

#include <algorithm>
#include <cmath>

#include <nmath/precision.h>
#include <nmath/prng.h>
#include <nimg/luminance.h>

#include "scene.h"
#include "math/sampling_util.h"

namespace xtcore {
namespace medium {

struct interval_state_t
{
    interval_state_t()
        : object_id(HASH_ID_INVALID)
        , t_exit(0.0f)
    {}

    HASH_ID object_id;
    nmath::scalar_t t_exit;
};

inline bool active(const interval_state_t &s)
{
    return s.object_id != HASH_ID_INVALID;
}

inline nmath::scalar_t clamp_scalar(nmath::scalar_t v, nmath::scalar_t lo, nmath::scalar_t hi)
{
    return std::max(lo, std::min(v, hi));
}

inline nimg::ColorRGBf sigma_t(const xtcore::asset::medium::IMedium &m)
{
    return m.sigma_t();
}

inline nmath::scalar_t sigma_t_majorant(const xtcore::asset::medium::IMedium &m)
{
    return m.sigma_t_majorant();
}

inline nimg::ColorRGBf transmittance(const xtcore::asset::medium::IMedium &m, nmath::scalar_t dist)
{
    return m.transmittance(dist);
}

inline nimg::ColorRGBf transmittance(const xtcore::asset::medium::IMedium &m,
                                     const nmath::Vector3f &origin,
                                     const nmath::Vector3f &direction,
                                     nmath::scalar_t dist)
{
    return m.transmittance(origin, direction, dist);
}

inline bool sample_distance(const xtcore::asset::medium::IMedium &m,
                            nmath::scalar_t segment_dist,
                            nmath::scalar_t &sampled_dist)
{
    return m.sample_distance(segment_dist, sampled_dist);
}

inline bool sample_distance(const xtcore::asset::medium::IMedium &m,
                            const nmath::Vector3f &origin,
                            const nmath::Vector3f &direction,
                            nmath::scalar_t segment_dist,
                            nmath::scalar_t &sampled_dist)
{
    return m.sample_distance(origin, direction, segment_dist, sampled_dist);
}

inline nimg::ColorRGBf scattering_weight(const xtcore::asset::medium::IMedium &m)
{
    return m.scattering_weight();
}

inline nimg::ColorRGBf scattering_weight(const xtcore::asset::medium::IMedium &m,
                                         const nmath::Vector3f &p)
{
    return m.scattering_weight(p);
}

inline nimg::ColorRGBf emission(const xtcore::asset::medium::IMedium &m,
                                const nmath::Vector3f &p)
{
    return m.emission_at(p);
}

inline nmath::Vector3f sample_hg_direction(const nmath::Vector3f &forward, nmath::scalar_t g)
{
    const nmath::scalar_t gg = clamp_scalar(g, (nmath::scalar_t)-0.999, (nmath::scalar_t)0.999);
    const nmath::scalar_t u1 = nmath::prng_c(0.0, 1.0);
    const nmath::scalar_t u2 = nmath::prng_c(0.0, 1.0);

    nmath::scalar_t cos_theta = 0.0;
    if (std::fabs((double)gg) < 1e-3) {
        cos_theta = 1.0 - 2.0 * u1;
    } else {
        const nmath::scalar_t sq = (1.0 - gg * gg) / (1.0 - gg + 2.0 * gg * u1);
        cos_theta = (1.0 + gg * gg - sq * sq) / (2.0 * gg);
        cos_theta = clamp_scalar(cos_theta, (nmath::scalar_t)-1.0, (nmath::scalar_t)1.0);
    }
    const nmath::scalar_t sin_theta = nmath_sqrt(std::max((nmath::scalar_t)0.0, (nmath::scalar_t)1.0 - cos_theta * cos_theta));
    const nmath::scalar_t phi = (nmath::scalar_t)(2.0 * nmath::PI) * u2;

    const nmath::Vector3f w = forward.normalized();
    const nmath::Vector3f t = xtcore::math::sampling::build_tangent(w);
    nmath::Vector3f b = nmath::cross(w, t);
    if (b.length() <= (nmath::scalar_t)EPSILON) b = nmath::Vector3f(0.0f, 1.0f, 0.0f);
    else b.normalize();

    const nmath::Vector3f dir =
          t * (sin_theta * nmath_cos(phi))
        + b * (sin_theta * nmath_sin(phi))
        + w * cos_theta;

    return dir.normalized();
}

inline nmath::scalar_t phase_hg(nmath::scalar_t cos_theta, nmath::scalar_t g)
{
    const nmath::scalar_t gg = clamp_scalar(g, (nmath::scalar_t)-0.999, (nmath::scalar_t)0.999);
    const nmath::scalar_t denom = (nmath::scalar_t)(1.0 + gg * gg - 2.0 * gg * cos_theta);
    if (denom <= (nmath::scalar_t)EPSILON) return (nmath::scalar_t)0.0;
    return ((nmath::scalar_t)(1.0 - gg * gg)) / ((nmath::scalar_t)(4.0 * nmath::PI) * denom * nmath_sqrt(denom));
}

inline const xtcore::asset::ISurface *get_medium_surface(const xtcore::Scene &scene, HASH_ID object_id)
{
    auto oit = scene.m_objects.find(object_id);
    if (oit == scene.m_objects.end() || !oit->second) return nullptr;
    auto sit = scene.m_surface.find(oit->second->surface);
    if (sit == scene.m_surface.end() || !sit->second) return nullptr;
    return sit->second;
}

inline bool surface_first_hit(const xtcore::asset::ISurface *surface,
                              const nmath::Vector3f &origin,
                              const nmath::Vector3f &direction,
                              xtcore::hit_record_t &out_hit)
{
    if (!surface) return false;
    if (direction.length_squared() <= (nmath::scalar_t)EPSILON) return false;

    const nmath::Vector3f dir = direction.normalized();
    const nmath::scalar_t eps = (nmath::scalar_t)EPSILON;
    for (size_t i = 0; i < 4; ++i) {
        const nmath::scalar_t jitter = eps * (nmath::scalar_t)(1 + i);
        const xtcore::Ray probe(origin + dir * jitter, dir);
        if (surface->intersection(probe, &out_hit) && out_hit.t > eps) return true;
    }
    return false;
}

inline bool is_inside_medium(const xtcore::Scene &scene, HASH_ID object_id, const nmath::Vector3f &p)
{
    const xtcore::asset::ISurface *surface = get_medium_surface(scene, object_id);
    if (!surface) return false;

    // For closed, consistently oriented boundaries, the first hit normal faces with the ray
    // when starting inside and against the ray when starting outside.
    static const nmath::Vector3f k_probe_dirs[] = {
        nmath::Vector3f(1.0f, 0.0f, 0.0f),
        nmath::Vector3f(0.0f, 1.0f, 0.0f),
        nmath::Vector3f(0.0f, 0.0f, 1.0f),
        nmath::Vector3f(0.577f, 0.577f, 0.577f),
        nmath::Vector3f(-0.456f, 0.812f, -0.364f)
    };

    for (size_t i = 0; i < sizeof(k_probe_dirs) / sizeof(k_probe_dirs[0]); ++i) {
        xtcore::hit_record_t hit;
        if (!surface_first_hit(surface, p, k_probe_dirs[i], hit)) continue;
        return nmath::dot(hit.normal, k_probe_dirs[i]) > (nmath::scalar_t)0.0;
    }

    return false;
}

inline bool distance_to_medium_boundary(const xtcore::Scene &scene,
                                        HASH_ID object_id,
                                        const nmath::Vector3f &origin,
                                        const nmath::Vector3f &direction,
                                        nmath::scalar_t &out_dist)
{
    const xtcore::asset::ISurface *surface = get_medium_surface(scene, object_id);
    if (!surface) return false;
    if (direction.length_squared() <= (nmath::scalar_t)EPSILON) return false;

    xtcore::hit_record_t hit;
    if (!surface_first_hit(surface, origin, direction, hit)) return false;
    out_dist = hit.t;
    return out_dist > (nmath::scalar_t)EPSILON;
}

inline bool recompute_exit_distance(const xtcore::Scene &scene,
                                    interval_state_t &state,
                                    const nmath::Vector3f &origin,
                                    const nmath::Vector3f &direction)
{
    if (!active(state)) return false;
    nmath::scalar_t t = INFINITY;
    if (!distance_to_medium_boundary(scene, state.object_id, origin, direction, t)) return false;
    state.t_exit = t;
    return true;
}

inline const xtcore::asset::medium::IMedium *find_containing_medium(
      const xtcore::Scene &scene
    , const nmath::Vector3f &origin
    , HASH_ID &out_object_id
    , nmath::scalar_t &out_exit_dist
    , const nmath::Vector3f &ray_dir
)
{
    out_object_id = HASH_ID_INVALID;
    out_exit_dist = INFINITY;

    const xtcore::asset::medium::IMedium *out_medium = nullptr;
    for (auto it = scene.m_media.begin(); it != scene.m_media.end(); ++it) {
        const HASH_ID oid = it->first;
        if (!is_inside_medium(scene, oid, origin)) continue;

        nmath::scalar_t t_exit = INFINITY;
        if (!distance_to_medium_boundary(scene, oid, origin, ray_dir, t_exit)) continue;
        if (t_exit <= (nmath::scalar_t)EPSILON) continue;

        if (t_exit < out_exit_dist) {
            out_exit_dist = t_exit;
            out_object_id = oid;
            out_medium = it->second;
        }
    }

    return out_medium;
}

inline void clear(interval_state_t &state)
{
    state.object_id = HASH_ID_INVALID;
    state.t_exit = 0.0f;
}

inline bool advance(interval_state_t &state, nmath::scalar_t dist)
{
    if (!active(state)) return false;
    state.t_exit = std::max((nmath::scalar_t)0.0, state.t_exit - std::max((nmath::scalar_t)0.0, dist));
    return true;
}

} // namespace medium
} // namespace xtcore

#endif /* XTCORE_MEDIUM_UTIL_H_INCLUDED */
