#include <nmath/vector.h>
#include <cmath>
#include <algorithm>
#include <limits>
#include <chrono>
#include <mutex>

#include "math/hitrecord.h"
#include "log.h"
#include "mesh.h"

namespace xtcore {

namespace {

std::mutex g_mesh_bvh_stats_mut;
mesh_bvh_stats_t g_mesh_bvh_stats = {0, 0, 0, 0.0, 0, 0, 0, 0, 0};

} // namespace

mesh_bvh_stats_t get_last_mesh_bvh_stats()
{
    std::lock_guard<std::mutex> lock(g_mesh_bvh_stats_mut);
    return g_mesh_bvh_stats;
}

void reset_mesh_bvh_stats()
{
    std::lock_guard<std::mutex> lock(g_mesh_bvh_stats_mut);
    g_mesh_bvh_stats = {0, 0, 0, 0.0, 0, 0, 0, 0, 0};
}

    namespace surface {

namespace {

static bool ray_aabb_intersection_tmin(const AABB3 &aabb,
                                       const Ray &ray,
                                       nmath::scalar_t t_max,
                                       nmath::scalar_t &out_tmin)
{
    const nmath::scalar_t eps = (nmath::scalar_t)EPSILON;
    const nmath::scalar_t inf = std::numeric_limits<nmath::scalar_t>::infinity();

    nmath::scalar_t tmin = (nmath::scalar_t)0.0;
    nmath::scalar_t tmax = t_max;

    for (int axis = 0; axis < 3; ++axis) {
        const nmath::scalar_t org = ray.origin[axis];
        const nmath::scalar_t dir = ray.direction[axis];
        const nmath::scalar_t bmin = aabb.min[axis];
        const nmath::scalar_t bmax = aabb.max[axis];

        if (std::fabs((double)dir) <= eps) {
            if (org < bmin || org > bmax) return false;
            continue;
        }

        const nmath::scalar_t inv = (nmath::scalar_t)1.0 / dir;
        nmath::scalar_t t0 = (bmin - org) * inv;
        nmath::scalar_t t1 = (bmax - org) * inv;
        if (t0 > t1) std::swap(t0, t1);
        if (t0 > tmin) tmin = t0;
        if (t1 < tmax) tmax = t1;
        if (tmax < tmin) return false;
    }

    if (tmax <= eps || tmin > t_max || tmin == inf) return false;
    out_tmin = tmin;
    return true;
}

static Vector3f triangle_centroid(const Triangle &tri)
{
    return (tri.v[0] + tri.v[1] + tri.v[2]) / 3.0f;
}

static AABB3 merge_aabb(const AABB3 &a, const AABB3 &b)
{
    return AABB3(
        Vector3f(std::min(a.min.x, b.min.x), std::min(a.min.y, b.min.y), std::min(a.min.z, b.min.z)),
        Vector3f(std::max(a.max.x, b.max.x), std::max(a.max.y, b.max.y), std::max(a.max.z, b.max.z))
    );
}

} // namespace

Mesh::Mesh()
    : m_uv_projection(UV_PROJECTION_SOURCE)
{}

Mesh::~Mesh()
{}

nmath::scalar_t Mesh::distance(nmath::Vector3f) const
{
    return INFINITY;
}

bool Mesh::intersection(const Ray &ray, hit_record_t* i_hit_record) const
{
    if (m_bvh_nodes.empty()) return false;

    hit_record_t best_hit;
    bool found_hit = false;
    nmath::scalar_t best_t = std::numeric_limits<nmath::scalar_t>::infinity();
    unsigned long long visited_nodes = 0;
    unsigned long long tested_triangles = 0;

    struct stack_item_t {
        uint32_t node_idx;
        nmath::scalar_t tmin;
    };

    std::vector<stack_item_t> stack;
    stack.reserve(m_bvh_nodes.size() > 0 ? m_bvh_nodes.size() : 1);
    nmath::scalar_t root_tmin = (nmath::scalar_t)0.0;
    if (!ray_aabb_intersection_tmin(m_bvh_nodes[0].aabb, ray, best_t, root_tmin)) return false;
    stack.push_back(stack_item_t{0, root_tmin});

    while (!stack.empty()) {
        const stack_item_t current = stack.back();
        stack.pop_back();
        if (current.tmin > best_t) continue;

        const bvh_node_t &node = m_bvh_nodes[current.node_idx];
        ++visited_nodes;
        if (node.count > 0) {
            const uint32_t end = node.first + node.count;
            for (uint32_t i = node.first; i < end; ++i) {
                const Triangle &tri = m_triangles[m_bvh_indices[i]];
                hit_record_t test_hit;
                ++tested_triangles;
                if (tri.intersection(ray, &test_hit) && test_hit.t < best_t) {
                    best_t = test_hit.t;
                    best_hit = test_hit;
                    found_hit = true;
                }
            }
            continue;
        }

        nmath::scalar_t left_tmin = (nmath::scalar_t)0.0;
        nmath::scalar_t right_tmin = (nmath::scalar_t)0.0;
        const bool has_left = ray_aabb_intersection_tmin(m_bvh_nodes[node.left].aabb, ray, best_t, left_tmin);
        const bool has_right = ray_aabb_intersection_tmin(m_bvh_nodes[node.right].aabb, ray, best_t, right_tmin);

        if (has_left && has_right) {
            const bool left_first = left_tmin <= right_tmin;
            const stack_item_t near_item = left_first ? stack_item_t{node.left, left_tmin} : stack_item_t{node.right, right_tmin};
            const stack_item_t far_item = left_first ? stack_item_t{node.right, right_tmin} : stack_item_t{node.left, left_tmin};
            stack.push_back(far_item);
            stack.push_back(near_item);
        } else if (has_left) {
            stack.push_back(stack_item_t{node.left, left_tmin});
        } else if (has_right) {
            stack.push_back(stack_item_t{node.right, right_tmin});
        }
    }

    {
        std::lock_guard<std::mutex> lock(g_mesh_bvh_stats_mut);
        g_mesh_bvh_stats.rays_tested += 1ULL;
        g_mesh_bvh_stats.nodes_visited += visited_nodes;
        g_mesh_bvh_stats.triangle_tests += tested_triangles;
        if (found_hit) g_mesh_bvh_stats.rays_hit += 1ULL;
    }

    if (!found_hit) return false;

    if (i_hit_record) *i_hit_record = best_hit;

    if (i_hit_record && m_uv_projection != UV_PROJECTION_SOURCE) {
        const nmath::scalar_t cx = (aabb.min.x + aabb.max.x) * 0.5f;
        const nmath::scalar_t cz = (aabb.min.z + aabb.max.z) * 0.5f;
        const nmath::scalar_t dx = i_hit_record->point.x - cx;
        const nmath::scalar_t dz = i_hit_record->point.z - cz;

        const nmath::scalar_t h = std::max((nmath::scalar_t)EPSILON, aabb.max.y - aabb.min.y);
        nmath::scalar_t u = (nmath::scalar_t)0.5 + (nmath::scalar_t)(std::atan2((double)dz, (double)dx) / (2.0 * nmath::PI_DOUBLE));
        u = u - (nmath::scalar_t)std::floor((double)u);
        const nmath::scalar_t v = (i_hit_record->point.y - aabb.min.y) / h;
        i_hit_record->texcoord = Vector3f(u, v, 0.0f);
    }
    if (i_hit_record) {
        const nmath::scalar_t sx = (uv_scale.x != 0.0f) ? uv_scale.x : 1.0f;
        const nmath::scalar_t sy = (uv_scale.y != 0.0f) ? uv_scale.y : 1.0f;
        i_hit_record->texcoord.x *= sx;
        i_hit_record->texcoord.y *= sy;
    }

    return true;
}

void Mesh::calc_aabb()
{
    if (m_bvh_nodes.empty()) {
        aabb = AABB3(Vector3f(0, 0, 0), Vector3f(0, 0, 0));
        return;
    }
    aabb = m_bvh_nodes[0].aabb;
}

void Mesh::build_bvh()
{
    const auto build_t0 = std::chrono::steady_clock::now();
    m_bvh_nodes.clear();
    m_bvh_indices.clear();

    if (m_triangles.empty()) {
        aabb = AABB3(Vector3f(0, 0, 0), Vector3f(0, 0, 0));
        std::lock_guard<std::mutex> lock(g_mesh_bvh_stats_mut);
        g_mesh_bvh_stats.triangles = 0;
        g_mesh_bvh_stats.bvh_nodes = 0;
        g_mesh_bvh_stats.bvh_leaves = 0;
        g_mesh_bvh_stats.build_ms = 0.0;
        g_mesh_bvh_stats.build_count += 1ULL;
        return;
    }

    m_bvh_indices.resize(m_triangles.size());
    for (uint32_t i = 0; i < m_bvh_indices.size(); ++i) {
        m_bvh_indices[i] = i;
    }

    m_bvh_nodes.reserve(m_triangles.size() * 2);
    build_bvh_node(0, (uint32_t)m_bvh_indices.size(), 0);
    aabb = m_bvh_nodes[0].aabb;

    size_t leaf_count = 0;
    for (size_t i = 0; i < m_bvh_nodes.size(); ++i) {
        if (m_bvh_nodes[i].count > 0) ++leaf_count;
    }

    const auto build_t1 = std::chrono::steady_clock::now();
    const double build_ms = std::chrono::duration<double, std::milli>(build_t1 - build_t0).count();
    std::lock_guard<std::mutex> lock(g_mesh_bvh_stats_mut);
    g_mesh_bvh_stats.triangles = m_triangles.size();
    g_mesh_bvh_stats.bvh_nodes = m_bvh_nodes.size();
    g_mesh_bvh_stats.bvh_leaves = leaf_count;
    g_mesh_bvh_stats.build_ms = build_ms;
    g_mesh_bvh_stats.build_count += 1ULL;
}

uint32_t Mesh::build_bvh_node(uint32_t first, uint32_t count, uint32_t depth)
{
    const uint32_t node_idx = (uint32_t)m_bvh_nodes.size();
    m_bvh_nodes.push_back(bvh_node_t());
    m_bvh_nodes[node_idx].left = 0;
    m_bvh_nodes[node_idx].right = 0;
    m_bvh_nodes[node_idx].first = first;
    m_bvh_nodes[node_idx].count = count;

    AABB3 bounds = m_triangles[m_bvh_indices[first]].aabb;
    Vector3f centroid_min = triangle_centroid(m_triangles[m_bvh_indices[first]]);
    Vector3f centroid_max = centroid_min;

    for (uint32_t i = first + 1; i < first + count; ++i) {
        const Triangle &tri = m_triangles[m_bvh_indices[i]];
        bounds = merge_aabb(bounds, tri.aabb);
        const Vector3f c = triangle_centroid(tri);
        centroid_min.x = std::min(centroid_min.x, c.x);
        centroid_min.y = std::min(centroid_min.y, c.y);
        centroid_min.z = std::min(centroid_min.z, c.z);
        centroid_max.x = std::max(centroid_max.x, c.x);
        centroid_max.y = std::max(centroid_max.y, c.y);
        centroid_max.z = std::max(centroid_max.z, c.z);
    }

    m_bvh_nodes[node_idx].aabb = bounds;

    if (count <= LIMITS_MAX_ITEMS_PER_LEAF || depth >= LIMITS_MAX_BVH_DEPTH) {
        return node_idx;
    }

    const Vector3f ext = centroid_max - centroid_min;
    int axis = 0;
    if (ext.y > ext.x) axis = 1;
    if (ext.z > ext[axis]) axis = 2;
    if (ext[axis] <= (nmath::scalar_t)EPSILON) {
        return node_idx;
    }

    const uint32_t mid = first + count / 2;
    std::nth_element(
        m_bvh_indices.begin() + first,
        m_bvh_indices.begin() + mid,
        m_bvh_indices.begin() + first + count,
        [this, axis](uint32_t a, uint32_t b) {
            return triangle_centroid(m_triangles[a])[axis] < triangle_centroid(m_triangles[b])[axis];
        }
    );

    if (mid == first || mid == first + count) {
        return node_idx;
    }

    m_bvh_nodes[node_idx].left = build_bvh_node(first, mid - first, depth + 1);
    m_bvh_nodes[node_idx].right = build_bvh_node(mid, count - (mid - first), depth + 1);
    m_bvh_nodes[node_idx].first = 0;
    m_bvh_nodes[node_idx].count = 0;
    return node_idx;
}

void Mesh::build_bvh(shape_t &shape, attrib_t &attributes)
{
    m_triangles.clear();

    auto read_vec3 = [](const std::vector<float> &buf, int idx, Vector3f &out) -> bool {
        if (idx < 0) return false;
        const size_t i = (size_t)idx;
        const size_t base = i * 3;
        if (base + 2 >= buf.size()) return false;
        out = Vector3f(buf[base], buf[base + 1], buf[base + 2]);
        return true;
    };

    auto read_uv = [](const std::vector<float> &buf, int idx, Vector2f &out) -> bool {
        if (idx < 0) return false;
        const size_t i = (size_t)idx;
        const size_t base = i * 2;
        if (base + 1 >= buf.size()) return false;
        out = Vector2f(buf[base], buf[base + 1]);
        return true;
    };

    for (size_t i = 0; i < shape.mesh.indices.size()/3; ++i) {
        int a_v = shape.mesh.indices[3*i  ].v;
        int b_v = shape.mesh.indices[3*i+1].v;
        int c_v = shape.mesh.indices[3*i+2].v;
        int a_n = shape.mesh.indices[3*i  ].n;
        int b_n = shape.mesh.indices[3*i+1].n;
        int c_n = shape.mesh.indices[3*i+2].n;
        int a_t = shape.mesh.indices[3*i  ].uv;
        int b_t = shape.mesh.indices[3*i+1].uv;
        int c_t = shape.mesh.indices[3*i+2].uv;

        attrib_t *d = &(attributes);

        xtcore::surface::Triangle p;

        read_vec3(d->v, a_v, p.v[0]);
        read_vec3(d->v, b_v, p.v[1]);
        read_vec3(d->v, c_v, p.v[2]);
        read_vec3(d->n, a_n, p.n[0]);
        read_vec3(d->n, b_n, p.n[1]);
        read_vec3(d->n, c_n, p.n[2]);

        read_uv(d->uv, a_t, p.tc[0]);
        read_uv(d->uv, b_t, p.tc[1]);
        read_uv(d->uv, c_t, p.tc[2]);

	    p.calc_aabb();
        m_triangles.push_back(p);
    }

    Log::handle().post_message("Mesh: %zu vertices, %zu triangles",
        attributes.v.size() / 3, m_triangles.size());
    build_bvh();
}

void Mesh::build_bvh(object_t &object)
{
    m_triangles.clear();

    auto read_vec3 = [](const std::vector<float> &buf, int idx, Vector3f &out) -> bool {
        if (idx < 0) return false;
        const size_t i = (size_t)idx;
        const size_t base = i * 3;
        if (base + 2 >= buf.size()) return false;
        out = Vector3f(buf[base], buf[base + 1], buf[base + 2]);
        return true;
    };

    auto read_uv = [](const std::vector<float> &buf, int idx, Vector2f &out) -> bool {
        if (idx < 0) return false;
        const size_t i = (size_t)idx;
        const size_t base = i * 2;
        if (base + 1 >= buf.size()) return false;
        out = Vector2f(buf[base], buf[base + 1]);
        return true;
    };

    std::vector<shape_t>::iterator it = object.shapes.begin();
    std::vector<shape_t>::iterator et = object.shapes.end();

    for (; it != et; ++it) {
     	for (size_t i = 0; i < (*it).mesh.indices.size()/3; ++i) {

            int a_v = (*it).mesh.indices[3*i  ].v;
            int b_v = (*it).mesh.indices[3*i+1].v;
            int c_v = (*it).mesh.indices[3*i+2].v;
            int a_n = (*it).mesh.indices[3*i  ].n;
            int b_n = (*it).mesh.indices[3*i+1].n;
            int c_n = (*it).mesh.indices[3*i+2].n;
            int a_t = (*it).mesh.indices[3*i  ].uv;
            int b_t = (*it).mesh.indices[3*i+1].uv;
            int c_t = (*it).mesh.indices[3*i+2].uv;

            attrib_t *d = &(object.attributes);

    		Triangle p;

            read_vec3(d->v, a_v, p.v[0]);
            read_vec3(d->v, b_v, p.v[1]);
            read_vec3(d->v, c_v, p.v[2]);
            read_vec3(d->n, a_n, p.n[0]);
            read_vec3(d->n, b_n, p.n[1]);
            read_vec3(d->n, c_n, p.n[2]);

            read_uv(d->uv, a_t, p.tc[0]);
            read_uv(d->uv, b_t, p.tc[1]);
            read_uv(d->uv, c_t, p.tc[2]);

		    p.calc_aabb();
            m_triangles.push_back(p);
        }
    }

    Log::handle().post_message("Mesh: %zu vertices, %zu triangles",
        object.attributes.v.size() / 3, m_triangles.size());
    build_bvh();
}

void Mesh::set_uv_projection(uv_projection_t projection)
{
    m_uv_projection = projection;
}

Mesh::uv_projection_t Mesh::uv_projection() const
{
    return m_uv_projection;
}

const std::vector<xtcore::surface::Triangle> &Mesh::triangles() const
{
    return m_triangles;
}

void Mesh::collect_bvh_aabbs(std::vector<AABB3> &out) const
{
    out.clear();
    out.reserve(m_bvh_nodes.size());
    for (size_t i = 0; i < m_bvh_nodes.size(); ++i) {
        out.push_back(m_bvh_nodes[i].aabb);
    }
}

Vector3f Mesh::point_sample() const
{
    // Not yet implemented
    return Vector3f(0,0,0);
}

Ray Mesh::ray_sample() const
{
    // Not yet implemented
    Ray ray;

    ray.origin    = Vector3f(0,0,0);
    ray.direction = Vector3f(0,0,0);

    return ray;
}

Vector3f Mesh::emitter_position() const
{
    if (std::isfinite((double)aabb.min.x) && std::isfinite((double)aabb.min.y) && std::isfinite((double)aabb.min.z) &&
        std::isfinite((double)aabb.max.x) && std::isfinite((double)aabb.max.y) && std::isfinite((double)aabb.max.z)) {
        return (aabb.min + aabb.max) * 0.5f;
    }
    if (!m_triangles.empty()) {
        const xtcore::surface::Triangle &t = m_triangles[0];
        return (t.v[0] + t.v[1] + t.v[2]) / 3.0f;
    }
    return Vector3f(0, 0, 0);
}

const std::vector<Mesh::bvh_node_t> &Mesh::bvh_nodes()  const { return m_bvh_nodes; }
const std::vector<uint32_t>         &Mesh::bvh_indices() const { return m_bvh_indices; }

    } /* namespace surface */
} /* namespace xtcore */
