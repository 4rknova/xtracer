#include <nmath/vector.h>
#include <cmath>
#include <algorithm>

#include "math/hitrecord.h"
#include "mesh.h"

namespace xtcore {
    namespace surface {

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
    OctreeItem<Triangle> *hit = m_octree.intersection(ray, i_hit_record);
    if (!hit) return false;

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

    return true;
}

void Mesh::calc_aabb()
{
    aabb = m_octree.bbox();
}

void Mesh::build_octree(shape_t &shape, attrib_t &attributes)
{
    m_octree.clear();
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
        m_octree.add(p.aabb, p);
    }

    m_octree.max_items_per_node(LIMITS_MAX_ITEMS_PER_NODE);
    m_octree.max_depth(LIMITS_MAX_DEPTH);
    m_octree.build();
    aabb = m_octree.bbox();
}

void Mesh::build_octree(object_t &object)
{
    m_octree.clear();
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
    		m_octree.add(p.aabb, p);
        }
    }
	m_octree.max_items_per_node(LIMITS_MAX_ITEMS_PER_NODE);
	m_octree.max_depth(LIMITS_MAX_DEPTH);
	m_octree.build();
    aabb = m_octree.bbox();
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

    } /* namespace surface */
} /* namespace xtcore */
