#include <algorithm>
#include <vector>
#include <fstream>
#include <cmath>
#include <limits>
#include <chrono>
#include <mutex>
#include <ncf/util.h>
#include <nmath/mutil.h>
#include <nmath/vector.h>
#include <nmesh/transform.h>
#include <nmesh/obj.h>
#include <nmesh/structs.h>
#include <nimg/checkerboard.h>
#include "math/surface.h"
#include "math/sphere.h"
#include "math/plane.h"
#include "math/triangle.h"
#include "mesh.h"
#include "proto.h"
#include "parseutil.h"
#include "log.h"
#include "scene.h"
#include "math/sampling_util.h"
#include "sampler/sampler_cubemap.h"
#include "sampler/sampler_erp.h"
#include "sampler/sampler_rayleigh_sky.h"

using nmath::Vector2f;
using nmath::Vector3f;
using xtcore::Log;

namespace xtcore {

namespace {

std::mutex g_spatial_stats_mut;
spatial_index_stats_t g_spatial_stats = {0, 0, 0, 0, 0, 0.0, 0};

inline nmath::scalar_t clamp_scalar(nmath::scalar_t v, nmath::scalar_t lo, nmath::scalar_t hi)
{
    return std::max(lo, std::min(v, hi));
}

inline nmath::scalar_t rayleigh_sun_exponent(const xtcore::sampler::RayleighSky *sky)
{
    if (!sky) return (nmath::scalar_t)64.0;
    const nmath::scalar_t glow_radius = std::max((nmath::scalar_t)0.25, sky->sun_glow_radius);
    const nmath::scalar_t disk_radius = std::max((nmath::scalar_t)0.1, sky->sun_disk_radius);
    const nmath::scalar_t combined = std::max(glow_radius, disk_radius * (nmath::scalar_t)2.0);
    return clamp_scalar((nmath::scalar_t)320.0 / combined, (nmath::scalar_t)8.0, (nmath::scalar_t)256.0);
}

} // namespace

spatial_index_stats_t get_last_spatial_index_stats()
{
    std::lock_guard<std::mutex> lock(g_spatial_stats_mut);
    return g_spatial_stats;
}

// Private constructor & assignment operator.
// They are not implemented but declared private
// for the time being.
Scene::Scene(const Scene &)
{}

Scene &Scene::operator =(const Scene &)
{
	return *this;
}

Scene::Scene()
    : time(0)
    , m_environment(0)
    , m_spatial_index_dirty(true)
    , m_environment_cached_base(0)
    , m_environment_cubemap(0)
    , m_environment_erp(0)
    , m_environment_rayleigh_sky(0)
    , m_environment_sampler_type(ENV_SAMPLER_UNKNOWN)
{}

Scene::~Scene()
{
	release();
}

const xtcore::asset::Object *Scene::get_object(HASH_ID obj_id)
{
    auto it = m_objects.find(obj_id);
    if (it == m_objects.end()) return 0;
    return (*it).second;
}

const xtcore::asset::IMaterial *Scene::get_material(HASH_ID obj_id, float selector)
{
    const xtcore::asset::Object *obj = get_object(obj_id);
    if (!obj) return 0;

    xtcore::asset::Object *mutable_obj = m_objects[obj_id];

    if (!obj->material_array.empty()) {
        const int N   = (int)obj->material_array.size();
        const int idx = std::min((int)(selector * N), N - 1);
        if (mutable_obj->ptr_material_array[idx]) return mutable_obj->ptr_material_array[idx];
        auto it = m_materials.find(obj->material_array[idx]);
        if (it == m_materials.end()) return 0;
        mutable_obj->ptr_material_array[idx] = it->second;
        return it->second;
    }

    if (obj->ptr_material) return obj->ptr_material;
    auto it = m_materials.find(obj->material);
    if (it == m_materials.end()) return 0;
    mutable_obj->ptr_material = (*it).second;
    return (*it).second;
}

const xtcore::asset::ISurface *Scene::get_surface(HASH_ID obj_id)
{
    const xtcore::asset::Object *obj = get_object(obj_id);
    if (!obj) return 0;
    if (obj->ptr_surface) return obj->ptr_surface;

    xtcore::asset::Object *mutable_obj = m_objects[obj_id];
    auto it = m_surface.find(obj->surface);
    if (it == m_surface.end()) return 0;
    mutable_obj->ptr_surface = (*it).second;
    return (*it).second;
}

void Scene::get_light_sources(std::vector<light_t> &lights)
{
    lights.clear();

    std::map<HASH_UINT64, xtcore::asset::Object*>::iterator oit = m_objects.begin();
    std::map<HASH_UINT64, xtcore::asset::Object*>::iterator oet = m_objects.end();

    for(; oit != oet; ++oit) {
        if (!(*oit).second) continue;

        std::map<HASH_UINT64, xtcore::asset::ISurface* >::iterator
            git = m_surface.find((*oit).second->surface)
          , get = m_surface.end();
        std::map<HASH_UINT64, xtcore::asset::IMaterial*>::iterator
            mit = m_materials.find((*oit).second->material)
          , met = m_materials.end();

        if (git != get && mit != met) {
            if (!(*mit).second->is_emissive()) continue;
            light_t light;
            light.light    = (*git).second;
            light.material = (*mit).second;
            lights.push_back(light);
        }
    }
}

template<typename T>
void purge(std::map<HASH_UINT64, T*> &map)
{
	if(!map.empty()) {
		for (typename std::map<HASH_UINT64, T*>::iterator it = map.begin(); it != map.end(); ++it) {
			Log::handle().post_debug("Releasing %s..", xtcore::pool::str::get((*it).first));
            xtcore::pool::str::del((*it).first);
			delete (*it).second;
            (*it).second = 0;
		}
		map.clear();
	}
}

template<typename T>
int purge(std::map<HASH_UINT64, T*> &map, HASH_UINT64 id)
{
	typename std::map<HASH_UINT64, T*>::iterator it = map.find(id);
	if (it == map.end()) return 1;
	Log::handle().post_debug("Releasing %s..", xtcore::pool::str::get((*it).first));
    xtcore::pool::str::del((*it).first);
	delete (*it).second;
	map.erase(it);
	return 0;
}

void Scene::release()
{
    if (m_environment) {
        delete m_environment;
        m_environment = 0;
    }

    purge(m_cameras);
	purge(m_materials);
	purge(m_surface);
	purge(m_objects);
    if (!m_media.empty()) {
        for (auto it = m_media.begin(); it != m_media.end(); ++it) {
            delete it->second;
            it->second = 0;
        }
        m_media.clear();
    }
    if (!m_exterior_media.empty()) {
        for (auto it = m_exterior_media.begin(); it != m_exterior_media.end(); ++it) {
            delete it->second;
            it->second = 0;
        }
        m_exterior_media.clear();
    }

    m_tlas_items.clear();
    m_tlas_nodes.clear();
    m_infinite_objects.clear();
    m_spatial_index_dirty = true;
}

int Scene::destroy_camera(HASH_UINT64 id)
{
    return purge(m_cameras, id);
}

int Scene::destroy_material(HASH_UINT64 id)
{
    for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        xtcore::asset::Object *obj = (*it).second;
        if (!obj) continue;
        if (obj->material == id) obj->ptr_material = 0;
        for (size_t i = 0; i < obj->material_array.size(); ++i) {
            if (obj->material_array[i] == id) obj->ptr_material_array[i] = nullptr;
        }
    }
    return purge(m_materials, id);
}

int Scene::destroy_surface(HASH_UINT64 id)
{
    for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        xtcore::asset::Object *obj = (*it).second;
        if (!obj) continue;
        if (obj->surface == id) obj->ptr_surface = 0;
    }
    mark_spatial_index_dirty();
    return purge(m_surface, id);
}

int Scene::destroy_object(HASH_UINT64 id)
{
    mark_spatial_index_dirty();
    clear_object_medium(id);
    clear_object_exterior_medium(id);
    return purge(m_objects, id);
}

nimg::ColorRGBf Scene::sample_environment(const Vector3f &direction) const
{
    return m_environment ? m_environment->sample(direction) : nimg::ColorRGBf(0,0,0);
}

void Scene::sync_environment_sampler_cache() const
{
    if (m_environment == m_environment_cached_base && m_environment_sampler_type != ENV_SAMPLER_UNKNOWN) return;

    m_environment_cached_base = m_environment;
    m_environment_cubemap = 0;
    m_environment_erp = 0;
    m_environment_rayleigh_sky = 0;

    if (!m_environment) {
        m_environment_sampler_type = ENV_SAMPLER_GENERIC;
        return;
    }

    m_environment_rayleigh_sky = dynamic_cast<const xtcore::sampler::RayleighSky *>(m_environment);
    if (m_environment_rayleigh_sky) {
        m_environment_sampler_type = ENV_SAMPLER_RAYLEIGH_SKY;
        return;
    }

    m_environment_erp = dynamic_cast<const xtcore::sampler::ERP *>(m_environment);
    if (m_environment_erp) {
        m_environment_sampler_type = ENV_SAMPLER_ERP;
        return;
    }

    m_environment_cubemap = dynamic_cast<const xtcore::sampler::Cubemap *>(m_environment);
    if (m_environment_cubemap) {
        m_environment_sampler_type = ENV_SAMPLER_CUBEMAP;
        return;
    }

    m_environment_sampler_type = ENV_SAMPLER_GENERIC;
}

bool Scene::sample_environment_direction(Vector3f &direction, nmath::scalar_t &pdf, nimg::ColorRGBf &radiance) const
{
    if (!m_environment) {
        direction = Vector3f(0.0f, 1.0f, 0.0f);
        pdf = 0.0;
        radiance = nimg::ColorRGBf(0, 0, 0);
        return false;
    }

    sync_environment_sampler_cache();

    if (m_environment_sampler_type == ENV_SAMPLER_RAYLEIGH_SKY) {
        const xtcore::sampler::RayleighSky *rayleigh = m_environment_rayleigh_sky;
        const nmath::scalar_t p_sun = (nmath::scalar_t)0.65;
        const nmath::scalar_t exponent = rayleigh_sun_exponent(rayleigh);
        if (nmath::prng_c(0.0, 1.0) < p_sun) {
            nmath::scalar_t sun_pdf = 0.0;
            direction = xtcore::math::sampling::sample_power_cosine_lobe(rayleigh->sun_direction.normalized(), exponent, sun_pdf);
        } else {
            direction = xtcore::math::sampling::sample_uniform_sphere(pdf);
        }
        pdf = sample_environment_pdf(direction);
        radiance = sample_environment(direction);
        return pdf > (nmath::scalar_t)EPSILON;
    }

    if (m_environment_sampler_type == ENV_SAMPLER_ERP && m_environment_erp) {
        return m_environment_erp->sample_direction(direction, pdf, radiance);
    }

    if (m_environment_sampler_type == ENV_SAMPLER_CUBEMAP && m_environment_cubemap) {
        return m_environment_cubemap->sample_direction(direction, pdf, radiance);
    }

    direction = xtcore::math::sampling::sample_uniform_sphere(pdf);
    radiance = sample_environment(direction);
    return pdf > (nmath::scalar_t)EPSILON;
}

nmath::scalar_t Scene::sample_environment_pdf(const Vector3f &direction) const
{
    if (!m_environment) return 0.0;

    sync_environment_sampler_cache();

    if (m_environment_sampler_type == ENV_SAMPLER_RAYLEIGH_SKY && m_environment_rayleigh_sky) {
        const xtcore::sampler::RayleighSky *rayleigh = m_environment_rayleigh_sky;
        const nmath::scalar_t p_uniform = (nmath::scalar_t)0.35;
        const nmath::scalar_t p_sun = (nmath::scalar_t)0.65;
        const nmath::scalar_t exponent = rayleigh_sun_exponent(rayleigh);
        const nmath::scalar_t uniform_pdf = xtcore::math::sampling::uniform_sphere_pdf();
        const nmath::scalar_t sun_pdf = xtcore::math::sampling::power_cosine_lobe_pdf(rayleigh->sun_direction.normalized(), direction.normalized(), exponent);
        return p_uniform * uniform_pdf + p_sun * sun_pdf;
    }

    if (m_environment_sampler_type == ENV_SAMPLER_ERP && m_environment_erp) {
        return m_environment_erp->pdf_direction(direction);
    }

    if (m_environment_sampler_type == ENV_SAMPLER_CUBEMAP && m_environment_cubemap) {
        return m_environment_cubemap->pdf_direction(direction);
    }

    return xtcore::math::sampling::uniform_sphere_pdf();
}

const ColorRGBf &Scene::ambient()
{
	return m_ambient;
}

void Scene::ambient(const ColorRGBf &ambient)
{
	m_ambient = ambient;
}

xtcore::asset::ICamera *Scene::get_camera(HASH_UINT64 id)
{
    auto et = m_cameras.end()
       , ft = m_cameras.find(id);

    if (ft != et) return ft->second;

	return 0;
}

const xtcore::asset::medium::IMedium *Scene::get_object_medium(HASH_UINT64 object_id) const
{
    auto it = m_media.find(object_id);
    if (it == m_media.end()) return 0;
    return (*it).second;
}

bool Scene::has_object_medium(HASH_UINT64 object_id) const
{
    return m_media.find(object_id) != m_media.end();
}

void Scene::set_object_medium(HASH_UINT64 object_id, xtcore::asset::medium::IMedium *medium)
{
    clear_object_medium(object_id);
    if (!medium) return;
    m_media[object_id] = medium;
}

void Scene::clear_object_medium(HASH_UINT64 object_id)
{
    auto it = m_media.find(object_id);
    if (it != m_media.end()) {
        delete it->second;
        it->second = 0;
        m_media.erase(it);
    }
}

const xtcore::asset::medium::IMedium *Scene::get_object_exterior_medium(HASH_UINT64 object_id) const
{
    auto it = m_exterior_media.find(object_id);
    if (it == m_exterior_media.end()) return 0;
    return it->second;
}

bool Scene::has_object_exterior_medium(HASH_UINT64 object_id) const
{
    return m_exterior_media.find(object_id) != m_exterior_media.end();
}

void Scene::set_object_exterior_medium(HASH_UINT64 object_id, xtcore::asset::medium::IMedium *medium)
{
    clear_object_exterior_medium(object_id);
    if (!medium) return;
    m_exterior_media[object_id] = medium;
}

void Scene::clear_object_exterior_medium(HASH_UINT64 object_id)
{
    auto it = m_exterior_media.find(object_id);
    if (it != m_exterior_media.end()) {
        delete it->second;
        it->second = 0;
        m_exterior_media.erase(it);
    }
}

nmath::scalar_t Scene::distance(nmath::Vector3f p, HASH_ID &object) const
{
    auto it = m_objects.begin();
    auto et = m_objects.end();

    nmath::scalar_t dist = INFINITY;

    for (; it != et; ++it) {
        HASH_ID obj_id = (*it).first;
        HASH_ID geo_id = (*it).second->surface;
        auto geo = m_surface.find(geo_id);
        if (geo == m_surface.end()) continue;

        asset::ISurface *s = (*geo).second;

        nmath::scalar_t temp_dist = s->distance(p);
        if (temp_dist < dist) {
            dist = temp_dist;
            object = obj_id;
        }
    }
    return dist;
}

void Scene::mark_spatial_index_dirty()
{
    m_spatial_index_dirty = true;
}

bool Scene::is_finite_aabb(const AABB3 &aabb) const
{
    return std::isfinite((double)aabb.min.x) && std::isfinite((double)aabb.min.y) && std::isfinite((double)aabb.min.z)
        && std::isfinite((double)aabb.max.x) && std::isfinite((double)aabb.max.y) && std::isfinite((double)aabb.max.z);
}

bool Scene::intersects_node(const AABB3 &aabb, const Ray &ray) const
{
    const nmath::scalar_t eps = (nmath::scalar_t)EPSILON;
    const nmath::scalar_t inf = std::numeric_limits<nmath::scalar_t>::infinity();

    nmath::scalar_t tmin = -inf;
    nmath::scalar_t tmax = inf;

    for (int axis = 0; axis < 3; ++axis) {
        nmath::scalar_t org = ray.origin[axis];
        nmath::scalar_t dir = ray.direction[axis];
        nmath::scalar_t bmin = aabb.min[axis];
        nmath::scalar_t bmax = aabb.max[axis];

        if (std::fabs((double)dir) <= eps) {
            if (org < bmin || org > bmax) return false;
            continue;
        }

        nmath::scalar_t inv = (nmath::scalar_t)1.0 / dir;
        nmath::scalar_t t0 = (bmin - org) * inv;
        nmath::scalar_t t1 = (bmax - org) * inv;
        if (t0 > t1) std::swap(t0, t1);
        if (t0 > tmin) tmin = t0;
        if (t1 < tmax) tmax = t1;
        if (tmax < tmin) return false;
    }

    return tmax > eps;
}

void Scene::update_tlas_node_bounds(tlas_node_t &node)
{
    Vector3f bmin(INFINITY, INFINITY, INFINITY);
    Vector3f bmax(-INFINITY, -INFINITY, -INFINITY);

    for (size_t i = node.first; i < node.first + node.count; ++i) {
        const AABB3 &aabb = m_tlas_items[i].aabb;
        bmin.x = std::min(bmin.x, aabb.min.x);
        bmin.y = std::min(bmin.y, aabb.min.y);
        bmin.z = std::min(bmin.z, aabb.min.z);
        bmax.x = std::max(bmax.x, aabb.max.x);
        bmax.y = std::max(bmax.y, aabb.max.y);
        bmax.z = std::max(bmax.z, aabb.max.z);
    }

    node.aabb = AABB3(bmin, bmax);
}

int Scene::build_tlas_node(size_t first, size_t count)
{
    const int node_index = (int)m_tlas_nodes.size();
    m_tlas_nodes.push_back(tlas_node_t());

    tlas_node_t node;
    node.left = -1;
    node.right = -1;
    node.first = first;
    node.count = count;
    update_tlas_node_bounds(node);
    m_tlas_nodes[node_index] = node;

    static const size_t max_leaf_items = 4;
    if (count <= max_leaf_items) return node_index;

    const Vector3f ext = node.aabb.max - node.aabb.min;
    int axis = 0;
    if (ext.y > ext.x) axis = 1;
    if (ext.z > ext[axis]) axis = 2;

    const size_t mid = first + count / 2;
    std::nth_element(
        m_tlas_items.begin() + first,
        m_tlas_items.begin() + mid,
        m_tlas_items.begin() + first + count,
        [axis](const tlas_item_t &a, const tlas_item_t &b) {
            return a.center[axis] < b.center[axis];
        }
    );

    const int left = build_tlas_node(first, mid - first);
    const int right = build_tlas_node(mid, count - (mid - first));

    m_tlas_nodes[node_index].left = left;
    m_tlas_nodes[node_index].right = right;
    m_tlas_nodes[node_index].first = 0;
    m_tlas_nodes[node_index].count = 0;

    return node_index;
}

void Scene::rebuild_spatial_index()
{
    auto t0 = std::chrono::steady_clock::now();

    m_tlas_items.clear();
    m_tlas_nodes.clear();
    m_infinite_objects.clear();

    m_tlas_items.reserve(m_objects.size());
    m_infinite_objects.reserve(m_objects.size());
    m_tlas_nodes.reserve(m_objects.size() * 2 + 1);

    for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
        const HASH_ID object_id = it->first;
        const xtcore::asset::Object *obj = it->second;
        if (!obj) continue;

        auto surf_it = m_surface.find(obj->surface);
        if (surf_it == m_surface.end() || !surf_it->second) continue;

        const AABB3 &aabb = surf_it->second->aabb;
        if (!is_finite_aabb(aabb)) {
            m_infinite_objects.push_back(object_id);
            continue;
        }

        tlas_item_t item;
        item.object_id = object_id;
        item.aabb = aabb;
        item.center = aabb.center();
        m_tlas_items.push_back(item);
    }

    if (!m_tlas_items.empty()) {
        build_tlas_node(0, m_tlas_items.size());
    }

    size_t leaf_count = 0;
    for (size_t i = 0; i < m_tlas_nodes.size(); ++i) {
        if (m_tlas_nodes[i].count > 0) ++leaf_count;
    }

    auto t1 = std::chrono::steady_clock::now();
    const double build_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    {
        std::lock_guard<std::mutex> lock(g_spatial_stats_mut);
        g_spatial_stats.total_objects = m_objects.size();
        g_spatial_stats.finite_objects = m_tlas_items.size();
        g_spatial_stats.infinite_objects = m_infinite_objects.size();
        g_spatial_stats.tlas_nodes = m_tlas_nodes.size();
        g_spatial_stats.tlas_leaves = leaf_count;
        g_spatial_stats.build_ms = build_ms;
        g_spatial_stats.build_count += 1ULL;
    }

    m_spatial_index_dirty = false;
}

void Scene::collect_tlas_aabbs(std::vector<AABB3> &out)
{
    if (m_spatial_index_dirty) rebuild_spatial_index();
    out.clear();
    out.reserve(m_tlas_nodes.size());
    for (size_t i = 0; i < m_tlas_nodes.size(); ++i) {
        out.push_back(m_tlas_nodes[i].aabb);
    }
}

bool Scene::intersection(const Ray &ray, hit_record_t &hit_record)
{
    if (m_spatial_index_dirty) rebuild_spatial_index();

	hit_record_t test, res;
    bool hit = false;

    if (!m_tlas_nodes.empty()) {
        std::vector<int> stack;
        stack.reserve(64);
        stack.push_back(0);

        while (!stack.empty()) {
            const int node_idx = stack.back();
            stack.pop_back();

            const tlas_node_t &node = m_tlas_nodes[node_idx];
            if (!intersects_node(node.aabb, ray)) continue;

            if (node.count > 0) {
                for (size_t i = node.first; i < node.first + node.count; ++i) {
                    const HASH_ID object_id = m_tlas_items[i].object_id;

                    auto obj_it = m_objects.find(object_id);
                    if (obj_it == m_objects.end() || !obj_it->second) continue;
                    xtcore::asset::Object *obj = obj_it->second;
                    xtcore::asset::ISurface *surface = obj->ptr_surface;
                    if (!surface) {
                        auto surf_it = m_surface.find(obj->surface);
                        if (surf_it == m_surface.end() || !surf_it->second) continue;
                        surface = surf_it->second;
                        obj->ptr_surface = surface;
                    }

                    if (surface->intersection(ray, &test) && res.t > test.t) {
                        hit = true;
                        res = test;
                        res.id_object = object_id;
                    }
                }
                continue;
            }

            if (node.left >= 0) stack.push_back(node.left);
            if (node.right >= 0) stack.push_back(node.right);
        }
    }

    for (size_t i = 0; i < m_infinite_objects.size(); ++i) {
        const HASH_ID object_id = m_infinite_objects[i];

        auto obj_it = m_objects.find(object_id);
        if (obj_it == m_objects.end() || !obj_it->second) continue;
        xtcore::asset::Object *obj = obj_it->second;
        xtcore::asset::ISurface *surface = obj->ptr_surface;
        if (!surface) {
            auto surf_it = m_surface.find(obj->surface);
            if (surf_it == m_surface.end() || !surf_it->second) continue;
            surface = surf_it->second;
            obj->ptr_surface = surface;
        }

        if (surface->intersection(ray, &test) && res.t > test.t) {
            hit = true;
            res = test;
            res.id_object = object_id;
        }
    }

    hit_record = res;
    return hit ? true : false;
}

const std::vector<Scene::tlas_item_t> &Scene::tlas_items() const       { return m_tlas_items; }
const std::vector<Scene::tlas_node_t> &Scene::tlas_nodes() const       { return m_tlas_nodes; }
const std::vector<HASH_ID>            &Scene::infinite_objects() const  { return m_infinite_objects; }

} /* namespace xtcore */
