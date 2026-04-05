#include <cstdio>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <vector>
#include <chrono>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <fstream>
#include <random>

#include <nmath/precision.h>
#include <ncf/util.h>

#include <nmesh/transform.h>
#include <nmesh/invnormals.h>
#include <nmesh/icosahedron.h>
#include <nmesh/plane.h>
#include <nmesh/extras.h>
#include <nmesh/polyhedra.h>
#include <nmesh/ring.h>
#include <nmesh/snowflake.h>
#include <nimg/checkerboard.h>
#include <nimg/transform.h>

#include "math/plane.h"
#include "math/sphere.h"
#include "math/triangle.h"
#include "math/fractal.h"
#include "math/csg.h"
#include "mesh.h"
#include "proto.h"
#include "obj.h"

#include "strpool.h"
#include "log.h"
#include "camera.h"
#include "material.h"
#include "sampler.h"
#include "import_asset.h"
#include "sampler/sampler_erp.h"
#include "sampler/sampler_graphpaper.h"
#include "sampler/sampler_checker.h"
#include "sampler/sampler_weave.h"
#include "sampler/sampler_fbm_marble.h"
#include "sampler/sampler_rayleigh_sky.h"
#include "sampler/sampler_voronoi_normal.h"
#include "sampler/sampler_scenery_heightfield.h"
#include "macro.h"

#include "extrude.h"
#include "asset_fetcher.h"

#include "parseutil.h"

namespace xtcore {
    namespace io {
        namespace scn {

bool deserialize_bool(const char *val, const bool def)
{
	if (!val) return def;
	std::string s = val;
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return ( (s == "1") || (s == "yes") || (s == "true") ) ? true : false;
}

int deserialize_numi(const char *val, const int def)
{
    return val ? ncf::util::to_int(val) : def;
}

nmath::scalar_t deserialize_numf(const char *val, const nmath::scalar_t def)
{
	return val ? (nmath::scalar_t)ncf::util::to_double(val) : def;
}

std::string deserialize_cstr(const char *val, const char* def)
{
	return val ? val : def;
}

nmath::Vector2f deserialize_tex2(const ncf::NCF *node, const char *name, const nmath::Vector2f def)
{
	nmath::Vector2f res = def;

	if (node) {
        bool has_property = node->query_property(name);
        bool has_group    = node->query_group(name);

        if (has_group) {
            ncf::NCF *group = node->get_group_by_name(name);
            const char *u = group->get_property_by_name(XTPROTO_PROP_CRD_U);
            const char *v = group->get_property_by_name(XTPROTO_PROP_CRD_V);

    		res = nmath::Vector2f(
                    u ? (nmath::scalar_t)ncf::util::to_double(u) : def.x,
                    v ? (nmath::scalar_t)ncf::util::to_double(v) : def.y
    		);
        }
        else if (has_property) {
            float u, v;
            std::string val = node->get_property_by_name(name);
            if (2 == sscanf(val.c_str(), XTPROTO_FORMAT_TEX2, &u, &v)) res = nmath::Vector2f(u, v);
        }
	}

	return res;
}

nimg::ColorRGBf deserialize_col3(const ncf::NCF *node, const char *name, const nimg::ColorRGBf def)
{
    nimg::ColorRGBf res = def;

    if (node) {
        bool has_property = node->query_property(name);
        bool has_group    = node->query_group(name);

        if (has_group) {
            ncf::NCF *group = node->get_group_by_name(name);
            const char *r = group->get_property_by_name(XTPROTO_PROP_COL_R);
    		const char *g = group->get_property_by_name(XTPROTO_PROP_COL_G);
	    	const char *b = group->get_property_by_name(XTPROTO_PROP_COL_B);

		    res = nimg::ColorRGBf(
                    r ? (nmath::scalar_t)ncf::util::to_double(r) : def.r(),
                    g ? (nmath::scalar_t)ncf::util::to_double(g) : def.g(),
                    b ? (nmath::scalar_t)ncf::util::to_double(b) : def.b()
    		);
        }
        else if (has_property) {
            float r, g, b;
            std::string val = node->get_property_by_name(name);
            if (3 == sscanf(val.c_str(), XTPROTO_FORMAT_COL3, &r, &g, &b)) res = nimg::ColorRGBf(r, g, b);
        }
    }

    return res;
}

nmath::Vector3f deserialize_vec3(const ncf::NCF *node, const char *name, const nmath::Vector3f def)
{
	nmath::Vector3f res = def;

	if (node) {
        bool has_property = node->query_property(name);
        bool has_group    = node->query_group(name);

        if (has_group) {
            ncf::NCF *group = node->get_group_by_name(name);
    		const char *x = group->get_property_by_name(XTPROTO_PROP_CRD_X);
	    	const char *y = group->get_property_by_name(XTPROTO_PROP_CRD_Y);
		    const char *z = group->get_property_by_name(XTPROTO_PROP_CRD_Z);

            res = nmath::Vector3f(
                    x ? (nmath::scalar_t)ncf::util::to_double(x) : def.x,
                    y ? (nmath::scalar_t)ncf::util::to_double(y) : def.y,
                    z ? (nmath::scalar_t)ncf::util::to_double(z) : def.z
            );
        }
        else if (has_property) {
            float x, y, z;
            std::string val = node->get_property_by_name(name);
            if (3 == sscanf(val.c_str(), XTPROTO_FORMAT_VEC3, &x, &y, &z)) res = nmath::Vector3f(x, y, z);
        }
	}

	return res;
}

xtcore::sampler::ISampler *create_sampler(const char *base,
                                          const char *texture,
                                          float value[3],
                                          bool white_fallback_on_missing = false);
xtcore::sampler::Texture2D *deserialize_texture(const char *source, const ncf::NCF *p);
xtcore::sampler::Cubemap *deserialize_cubemap(const char *source, const ncf::NCF *p);
xtcore::sampler::ERP *deserialize_erp(const char *source, const ncf::NCF *p);
xtcore::sampler::Gradient *deserialize_gradient(const ncf::NCF *p);
xtcore::sampler::RayleighSky *deserialize_rayleigh_sky(const ncf::NCF *p);
xtcore::sampler::ISampler *deserialize_rgba(const ncf::NCF *p);
xtcore::sampler::ISampler *deserialize_graphpaper(const ncf::NCF *p);
xtcore::sampler::ISampler *deserialize_checker(const ncf::NCF *p);
xtcore::sampler::ISampler *deserialize_weave(const ncf::NCF *p);
xtcore::sampler::ISampler *deserialize_fbm_marble(const ncf::NCF *p);
xtcore::sampler::ISampler *deserialize_voronoi_normal(const ncf::NCF *p);
xtcore::sampler::ISampler *deserialize_scenery_heightfield(const ncf::NCF *p);
xtcore::sampler::ISampler *deserialize_sampler_node(const char *source, const ncf::NCF *p);

static void apply_heightfield_to_mesh(nmesh::object_t &obj,
                                      const xtcore::sampler::ISampler *height_sampler,
                                      const nmath::Vector3f &dimensions)
{
    if (!height_sampler) return;
    const size_t vertex_count = obj.attributes.v.size() / 3;
    if (vertex_count == 0) return;

    const size_t side = (size_t)(std::sqrt((double)vertex_count) + 0.5);
    if (side < 2 || side * side != vertex_count) return;

    const size_t seg = side - 1;
    const nmath::scalar_t width = std::max((nmath::scalar_t)0.1, dimensions.x);
    const nmath::scalar_t amplitude = std::max((nmath::scalar_t)0.0, dimensions.y);
    const nmath::scalar_t depth = std::max((nmath::scalar_t)0.1, dimensions.z);

    std::vector<nmath::scalar_t> heights(vertex_count, (nmath::scalar_t)0.0);

    for (size_t z = 0; z <= seg; ++z) {
        const nmath::scalar_t vz = (nmath::scalar_t)z / (nmath::scalar_t)seg;
        for (size_t x = 0; x <= seg; ++x) {
            const nmath::scalar_t vx = (nmath::scalar_t)x / (nmath::scalar_t)seg;
            const nmath::scalar_t px = (vx - (nmath::scalar_t)0.5) * width;
            const nmath::scalar_t pz = (vz - (nmath::scalar_t)0.5) * depth;
            const nimg::ColorRGBf sample = height_sampler->sample(nmath::Vector3f(px, (nmath::scalar_t)0.0, pz));
            const nmath::scalar_t h = (((nmath::scalar_t)sample.r() + (nmath::scalar_t)sample.g() + (nmath::scalar_t)sample.b()) / (nmath::scalar_t)3.0);
            heights[z * side + x] = (h * (nmath::scalar_t)2.0 - (nmath::scalar_t)1.0) * amplitude;
            obj.attributes.v[(z * side + x) * 3 + 0] = (float)px;
            obj.attributes.v[(z * side + x) * 3 + 1] = (float)heights[z * side + x];
            obj.attributes.v[(z * side + x) * 3 + 2] = (float)pz;
            if (obj.attributes.uv.size() >= (z * side + x) * 2 + 2) {
                obj.attributes.uv[(z * side + x) * 2 + 0] = (float)vx;
                obj.attributes.uv[(z * side + x) * 2 + 1] = (float)vz;
            }
        }
    }

    const nmath::scalar_t dx = width / (nmath::scalar_t)seg;
    const nmath::scalar_t dz = depth / (nmath::scalar_t)seg;
    for (size_t z = 0; z <= seg; ++z) {
        for (size_t x = 0; x <= seg; ++x) {
            const size_t xl = (x > 0) ? x - 1 : x;
            const size_t xr = (x < seg) ? x + 1 : x;
            const size_t zd = (z > 0) ? z - 1 : z;
            const size_t zu = (z < seg) ? z + 1 : z;

            const nmath::scalar_t h_l = heights[z * side + xl];
            const nmath::scalar_t h_r = heights[z * side + xr];
            const nmath::scalar_t h_d = heights[zd * side + x];
            const nmath::scalar_t h_u = heights[zu * side + x];

            nmath::Vector3f normal(-(h_r - h_l) / std::max((nmath::scalar_t)1e-6, (nmath::scalar_t)2.0 * dx),
                                    (nmath::scalar_t)1.0,
                                   -(h_u - h_d) / std::max((nmath::scalar_t)1e-6, (nmath::scalar_t)2.0 * dz));
            if (normal.length() <= (nmath::scalar_t)EPSILON) normal = nmath::Vector3f(0.0f, 1.0f, 0.0f);
            normal.normalize();

            obj.attributes.n[(z * side + x) * 3 + 0] = (float)normal.x;
            obj.attributes.n[(z * side + x) * 3 + 1] = (float)normal.y;
            obj.attributes.n[(z * side + x) * 3 + 2] = (float)normal.z;
        }
    }
}

namespace {

struct async_load_job_t {
    unsigned long long id;
    async_load_state_t state;
    std::string filename;
    std::string error;
    std::shared_ptr<Scene> scene;
    std::list<std::string> modifiers;
    bool has_modifiers;
    std::string variant;
    bool has_variant;

    async_load_job_t()
        : id(0)
        , state(ASYNC_LOAD_QUEUED)
        , filename()
        , error()
        , scene()
        , modifiers()
        , has_modifiers(false)
        , variant()
        , has_variant(false)
    {}
};

static std::mutex g_async_jobs_mut;
static std::mutex g_async_load_exec_mut;
static std::map<unsigned long long, std::shared_ptr<async_load_job_t> > g_async_jobs;
static std::atomic<unsigned long long> g_async_next_id(1ULL);

static const char *async_load_state_name(async_load_state_t state)
{
    switch (state) {
        case ASYNC_LOAD_QUEUED: return "queued";
        case ASYNC_LOAD_RUNNING: return "running";
        case ASYNC_LOAD_DONE: return "done";
        case ASYNC_LOAD_ERROR: return "error";
        default: return "unknown";
    }
}

static bool parse_trailing_index(const std::string &name, int &index)
{
    if (name.empty()) return false;
    int i = (int)name.size() - 1;
    while (i >= 0 && std::isdigit((unsigned char)name[(size_t)i])) --i;
    if (i == (int)name.size() - 1) return false;
    const std::string tail = name.substr((size_t)(i + 1));
    if (tail.empty()) return false;
    index = ncf::util::to_int(tail.c_str());
    return true;
}

static std::vector<nmath::Vector2f> deserialize_lathe_profile(const ncf::NCF *profile)
{
    struct point_entry_t {
        std::string name;
        int index;
        bool has_index;
        nmath::Vector2f value;
    };

    std::vector<point_entry_t> entries;
    if (!profile) return std::vector<nmath::Vector2f>();

    const size_t prop_count = profile->count_properties();
    for (size_t i = 0; i < prop_count; ++i) {
        const char *name_c = profile->get_property_name_by_index(i);
        if (!name_c || !*name_c) continue;
        const std::string name = name_c;

        point_entry_t e;
        e.name = name;
        e.has_index = parse_trailing_index(name, e.index);
        e.value = deserialize_tex2(profile, name.c_str(), nmath::Vector2f(0.0f, 0.0f));
        entries.push_back(e);
    }

    const size_t group_count = profile->count_groups();
    for (size_t i = 0; i < group_count; ++i) {
        ncf::NCF *g = profile->get_group_by_index(i);
        if (!g) continue;
        const char *name_c = g->get_name();
        if (!name_c || !*name_c) continue;
        const std::string name = name_c;

        point_entry_t e;
        e.name = name;
        e.has_index = parse_trailing_index(name, e.index);

        if (g->query_property(XTPROTO_PROP_CRD_U) || g->query_property(XTPROTO_PROP_CRD_V)) {
            e.value = deserialize_tex2(profile, name.c_str(), nmath::Vector2f(0.0f, 0.0f));
        } else {
            const float x = (float)deserialize_numf(g->get_property_by_name(XTPROTO_PROP_CRD_X), 0.0f);
            const float y = (float)deserialize_numf(g->get_property_by_name(XTPROTO_PROP_CRD_Y), 0.0f);
            e.value = nmath::Vector2f(x, y);
        }

        entries.push_back(e);
    }

    std::sort(entries.begin(), entries.end(), [](const point_entry_t &a, const point_entry_t &b) {
        if (a.has_index && b.has_index && a.index != b.index) return a.index < b.index;
        if (a.has_index != b.has_index) return a.has_index;
        return a.name < b.name;
    });

    std::vector<nmath::Vector2f> profile_points;
    profile_points.reserve(entries.size());
    for (size_t i = 0; i < entries.size(); ++i) profile_points.push_back(entries[i].value);
    return profile_points;
}

static std::vector<nmath::Vector3f> deserialize_spline_points(const ncf::NCF *spline)
{
    struct point_entry_t {
        std::string name;
        int index;
        bool has_index;
        nmath::Vector3f value;
    };

    std::vector<point_entry_t> entries;
    if (!spline) return std::vector<nmath::Vector3f>();

    const size_t prop_count = spline->count_properties();
    for (size_t i = 0; i < prop_count; ++i) {
        const char *name_c = spline->get_property_name_by_index(i);
        if (!name_c || !*name_c) continue;
        const std::string name = name_c;

        point_entry_t e;
        e.name = name;
        e.has_index = parse_trailing_index(name, e.index);
        e.value = deserialize_vec3(spline, name.c_str(), nmath::Vector3f(0.0f, 0.0f, 0.0f));
        entries.push_back(e);
    }

    const size_t group_count = spline->count_groups();
    for (size_t i = 0; i < group_count; ++i) {
        ncf::NCF *g = spline->get_group_by_index(i);
        if (!g) continue;
        const char *name_c = g->get_name();
        if (!name_c || !*name_c) continue;
        const std::string name = name_c;

        point_entry_t e;
        e.name = name;
        e.has_index = parse_trailing_index(name, e.index);
        const float x = (float)deserialize_numf(g->get_property_by_name(XTPROTO_PROP_CRD_X), 0.0f);
        const float y = (float)deserialize_numf(g->get_property_by_name(XTPROTO_PROP_CRD_Y), 0.0f);
        const float z = (float)deserialize_numf(g->get_property_by_name(XTPROTO_PROP_CRD_Z), 0.0f);
        e.value = nmath::Vector3f(x, y, z);
        entries.push_back(e);
    }

    std::sort(entries.begin(), entries.end(), [](const point_entry_t &a, const point_entry_t &b) {
        if (a.has_index && b.has_index && a.index != b.index) return a.index < b.index;
        if (a.has_index != b.has_index) return a.has_index;
        return a.name < b.name;
    });

    std::vector<nmath::Vector3f> points;
    points.reserve(entries.size());
    for (size_t i = 0; i < entries.size(); ++i) points.push_back(entries[i].value);
    return points;
}

static nmath::scalar_t clampf(nmath::scalar_t v, nmath::scalar_t lo, nmath::scalar_t hi)
{
    return std::max(lo, std::min(v, hi));
}

static bool path_is_absolute(const std::string &path)
{
    if (path.empty()) return false;
    if (path[0] == '/' || path[0] == '\\') return true;
    return path.size() > 1
        && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z'))
        && path[1] == ':';
}

static int create_random_sphere_grid(Scene *scene, const ncf::NCF *p)
{
    if (!scene || !p) return -1;

    const char *seed_prop = p->get_property_by_name("seed");
    if (!seed_prop) {
        Log::handle().post_error("random.%s requires seed for repeatable generation", p->get_name());
        return 1;
    }

    const int seed = deserialize_numi(seed_prop, 0);
    const int x_min = deserialize_numi(p->get_property_by_name("x_min"), -11);
    const int x_max = deserialize_numi(p->get_property_by_name("x_max"), 11);
    const int z_min = deserialize_numi(p->get_property_by_name("z_min"), -11);
    const int z_max = deserialize_numi(p->get_property_by_name("z_max"), 11);
    const nmath::scalar_t y = deserialize_numf(p->get_property_by_name("y"), (nmath::scalar_t)0.2);
    const nmath::scalar_t radius = deserialize_numf(p->get_property_by_name("radius"), (nmath::scalar_t)0.2);
    const nmath::scalar_t jitter = deserialize_numf(p->get_property_by_name("jitter"), (nmath::scalar_t)0.9);
    const nmath::Vector3f avoid_center = deserialize_vec3(p, "avoid_center", nmath::Vector3f(4.0f, 0.2f, 0.0f));
    const nmath::scalar_t avoid_radius = deserialize_numf(p->get_property_by_name("avoid_radius"), (nmath::scalar_t)0.9);

    const nmath::scalar_t lambert_ratio = clampf(deserialize_numf(p->get_property_by_name("lambert_ratio"), (nmath::scalar_t)0.8), 0.0f, 1.0f);
    const nmath::scalar_t metal_ratio = clampf(deserialize_numf(p->get_property_by_name("metal_ratio"), (nmath::scalar_t)0.95), lambert_ratio, 1.0f);
    const nmath::scalar_t metal_fuzz_min = clampf(deserialize_numf(p->get_property_by_name("metal_fuzz_min"), (nmath::scalar_t)0.0), 0.0f, 1.0f);
    const nmath::scalar_t metal_fuzz_max = clampf(deserialize_numf(p->get_property_by_name("metal_fuzz_max"), (nmath::scalar_t)0.5), metal_fuzz_min, 1.0f);
    const nmath::scalar_t dielectric_ior = std::max((nmath::scalar_t)1.0, deserialize_numf(p->get_property_by_name("ior"), (nmath::scalar_t)1.5));
    const nmath::scalar_t dielectric_reflectance = clampf(deserialize_numf(p->get_property_by_name("glass_reflectance"), (nmath::scalar_t)0.04), 0.0f, 1.0f);

    if (x_max <= x_min || z_max <= z_min) {
        Log::handle().post_error("random.%s has invalid grid bounds", p->get_name());
        return 1;
    }
    if (radius <= (nmath::scalar_t)EPSILON) {
        Log::handle().post_error("random.%s requires radius > 0", p->get_name());
        return 1;
    }

    std::string prefix = deserialize_cstr(p->get_property_by_name("prefix"), p->get_name());
    if (prefix.empty()) prefix = "random";

    std::mt19937 rng((uint32_t)seed);
    std::uniform_real_distribution<nmath::scalar_t> u01(0.0f, 1.0f);
    const auto rand01 = [&]() -> nmath::scalar_t { return u01(rng); };
    const auto rand_range = [&](nmath::scalar_t lo, nmath::scalar_t hi) -> nmath::scalar_t {
        return lo + (hi - lo) * rand01();
    };

    HASH_UINT64 dielectric_mat_id = HASH_ID_INVALID;
    int created = 0;
    int skipped = 0;

    for (int a = x_min; a < x_max; ++a) {
        for (int b = z_min; b < z_max; ++b) {
            const nmath::Vector3f center(
                (nmath::scalar_t)a + jitter * rand01(),
                y,
                (nmath::scalar_t)b + jitter * rand01()
            );
            if ((center - avoid_center).length() <= avoid_radius) {
                ++skipped;
                continue;
            }

            const nmath::scalar_t choose_mat = rand01();
            const std::string suffix = std::to_string(created);
            const std::string gname = prefix + "_geo_" + suffix;
            const std::string mname = prefix + "_mat_" + suffix;
            const std::string oname = prefix + "_obj_" + suffix;

            HASH_UINT64 gid = xtcore::pool::str::add(gname.c_str());
            HASH_UINT64 mid = xtcore::pool::str::add(mname.c_str());
            HASH_UINT64 oid = xtcore::pool::str::add(oname.c_str());

            xtcore::surface::Sphere *surf = new (std::nothrow) xtcore::surface::Sphere();
            if (!surf) return 1;
            surf->origin = center;
            surf->radius = radius;
            scene->destroy_surface(gid);
            scene->m_surface[gid] = surf;

            HASH_UINT64 object_mat_id = mid;
            xtcore::asset::IMaterial *mat = 0;

            if (choose_mat < lambert_ratio) {
                xtcore::asset::material::Lambert *lm = new (std::nothrow) xtcore::asset::material::Lambert();
                if (!lm) return 1;
                float kd[3] = {
                    (float)(rand01() * rand01()),
                    (float)(rand01() * rand01()),
                    (float)(rand01() * rand01())
                };
                lm->add_sampler(MAT_SAMPLER_DIFFUSE, create_sampler("", "", kd, false));
                mat = lm;
            } else if (choose_mat < metal_ratio) {
                xtcore::asset::material::BlinnPhong *bm = new (std::nothrow) xtcore::asset::material::BlinnPhong();
                if (!bm) return 1;
                float ks[3] = {
                    (float)rand_range(0.5f, 1.0f),
                    (float)rand_range(0.5f, 1.0f),
                    (float)rand_range(0.5f, 1.0f)
                };
                const nmath::scalar_t fuzz = rand_range(metal_fuzz_min, metal_fuzz_max);
                const nmath::scalar_t norm = (metal_fuzz_max > metal_fuzz_min + (nmath::scalar_t)EPSILON)
                    ? ((fuzz - metal_fuzz_min) / (metal_fuzz_max - metal_fuzz_min))
                    : 0.0f;
                const nmath::scalar_t exponent = 16.0f + (1.0f - norm) * 1008.0f;
                bm->add_sampler(MAT_SAMPLER_DIFFUSE, create_sampler("", "", ks, false));
                bm->add_sampler(MAT_SAMPLER_SPECULAR, create_sampler("", "", ks, false));
                bm->add_scalar(MAT_SCALART_REFLECTANCE, clampf(1.0f - fuzz, 0.0f, 1.0f));
                bm->add_scalar(MAT_SCALART_EXPONENT, exponent);
                mat = bm;
            } else {
                if (dielectric_mat_id == HASH_ID_INVALID) {
                    const std::string dname = prefix + "_glass";
                    dielectric_mat_id = xtcore::pool::str::add(dname.c_str());
                    if (scene->m_materials.find(dielectric_mat_id) == scene->m_materials.end()) {
                        xtcore::asset::material::Dielectric *dm = new (std::nothrow) xtcore::asset::material::Dielectric();
                        if (!dm) return 1;
                        dm->add_scalar(MAT_SCALART_IOR, dielectric_ior);
                        dm->add_scalar(MAT_SCALART_TRANSPARENCY, 1.0f);
                        dm->add_scalar(MAT_SCALART_REFLECTANCE, dielectric_reflectance);
                        scene->m_materials[dielectric_mat_id] = dm;
                    }
                }
                object_mat_id = dielectric_mat_id;
            }

            if (mat) {
                scene->destroy_material(mid);
                scene->m_materials[mid] = mat;
            }

            xtcore::asset::Object *obj = new (std::nothrow) xtcore::asset::Object();
            if (!obj) return 1;
            obj->surface = gid;
            obj->material = object_mat_id;
            scene->destroy_object(oid);
            scene->m_objects[oid] = obj;
            ++created;
        }
    }

    scene->mark_spatial_index_dirty();
    Log::handle().post_message("Created random sphere grid %s (%d spheres, %d skipped) [seed=%d]",
                               p->get_name(), created, skipped, seed);
    return 0;
}

static int create_random(Scene *scene, const ncf::NCF *p)
{
    if (!scene || !p) return -1;
    const std::string type = deserialize_cstr(p->get_property_by_name("type"));
    if (type.empty()) {
        Log::handle().post_error("random.%s is missing type", p->get_name());
        return 1;
    }
    if (!type.compare("sphere_grid")) return create_random_sphere_grid(scene, p);
    Log::handle().post_error("Unsupported random generator type %s in random.%s", type.c_str(), p->get_name());
    return 1;
}

static void merge_ncf_group(ncf::NCF *dst, const ncf::NCF *src)
{
    if (!dst || !src) return;

    const size_t prop_count = src->count_properties();
    for (size_t i = 0; i < prop_count; ++i) {
        const char *name = src->get_property_name_by_index(i);
        const char *value = src->get_property_by_index(i);
        if (!name || !*name) continue;
        if (!value) value = "";

        if (dst->query_group(name)) dst->remove_group(name);
        dst->set_property(name, value);
    }

    const size_t group_count = src->count_groups();
    for (size_t i = 0; i < group_count; ++i) {
        ncf::NCF *src_child = src->get_group_by_index(i);
        if (!src_child) continue;
        const char *name = src_child->get_name();
        if (!name || !*name) continue;
        if (dst->query_property(name)) dst->remove_property(name);
        ncf::NCF *dst_child = dst->get_group_by_name(name);
        merge_ncf_group(dst_child, src_child);
    }
}

static void apply_ncf_remove_overlay(ncf::NCF *target, const ncf::NCF *remove_node)
{
    if (!target || !remove_node) return;

    const size_t prop_count = remove_node->count_properties();
    for (size_t i = 0; i < prop_count; ++i) {
        const char *name = remove_node->get_property_name_by_index(i);
        if (!name || !*name) continue;
        target->remove_property(name);
        target->remove_group(name);
    }

    const size_t group_count = remove_node->count_groups();
    for (size_t i = 0; i < group_count; ++i) {
        ncf::NCF *remove_child = remove_node->get_group_by_index(i);
        if (!remove_child) continue;
        const char *name = remove_child->get_name();
        if (!name || !*name) continue;

        if (remove_child->count_properties() == 0 && remove_child->count_groups() == 0) {
            target->remove_property(name);
            target->remove_group(name);
            continue;
        }

        if (target->query_group(name)) {
            ncf::NCF *target_child = target->get_group_by_name(name);
            apply_ncf_remove_overlay(target_child, remove_child);
            continue;
        }

        if (target->query_property(name)) {
            target->remove_property(name);
            continue;
        }
    }
}

static bool apply_variant_overlay(ncf::NCF *root, const char *variant_name)
{
    if (!root || !variant_name || !*variant_name) return true;

    if (!root->query_group(XTPROTO_NODE_VARIANTS)) {
        Log::handle().post_error("Variant '%s' was requested but no '%s' node exists",
                                 variant_name, XTPROTO_NODE_VARIANTS);
        return false;
    }

    ncf::NCF *variants_node = root->get_group_by_name(XTPROTO_NODE_VARIANTS);
    if (!variants_node->query_group(variant_name)) {
        Log::handle().post_error("Variant '%s' was requested but is not defined", variant_name);
        return false;
    }

    ncf::NCF *variant_node = variants_node->get_group_by_name(variant_name);
    Log::handle().post_message("Applying variant: %s", variant_name);

    if (variant_node->query_group(XTPROTO_NODE_VARIANT_REMOVE)) {
        ncf::NCF *remove_node = variant_node->get_group_by_name(XTPROTO_NODE_VARIANT_REMOVE);
        apply_ncf_remove_overlay(root, remove_node);
    }

    if (variant_node->query_group(XTPROTO_NODE_VARIANT_SET)) {
        ncf::NCF *set_node = variant_node->get_group_by_name(XTPROTO_NODE_VARIANT_SET);
        merge_ncf_group(root, set_node);
    }

    return true;
}

} /* namespace */

xtcore::sampler::Gradient *deserialize_gradient(const ncf::NCF *p)
{
    if (!p) return 0;

    xtcore::sampler::Gradient *data = new xtcore::sampler::Gradient;

    if (data) {
        data->a = deserialize_col3(p, "a");
        data->b = deserialize_col3(p, "b");
    }

    return data;
}

xtcore::sampler::RayleighSky *deserialize_rayleigh_sky(const ncf::NCF *p)
{
    xtcore::sampler::RayleighSky *data = new (std::nothrow) xtcore::sampler::RayleighSky();
    if (!data || !p) return data;

    data->sun_direction = deserialize_vec3(p, "sun_direction", data->sun_direction);
    data->sun_intensity = deserialize_col3(p, "sun_intensity", data->sun_intensity);
    data->beta_rayleigh = deserialize_col3(p, "beta_rayleigh", data->beta_rayleigh);
    data->ground_color = deserialize_col3(p, "ground_color", data->ground_color);
    data->density = deserialize_numf(p->get_property_by_name("density"), data->density);
    data->horizon_falloff = deserialize_numf(p->get_property_by_name("horizon_falloff"), data->horizon_falloff);
    data->sun_disk_radius = deserialize_numf(p->get_property_by_name("sun_disk_radius"), data->sun_disk_radius);
    data->sun_disk_intensity = deserialize_numf(p->get_property_by_name("sun_disk_intensity"), data->sun_disk_intensity);
    data->sun_glow_radius = deserialize_numf(p->get_property_by_name("sun_glow_radius"), data->sun_glow_radius);
    data->sun_glow_intensity = deserialize_numf(p->get_property_by_name("sun_glow_intensity"), data->sun_glow_intensity);
    data->sun_glow_falloff = deserialize_numf(p->get_property_by_name("sun_glow_falloff"), data->sun_glow_falloff);
    return data;
}

xtcore::sampler::Cubemap *deserialize_cubemap(const char *source, const ncf::NCF *p)
{
    if (!p) return 0;

    xtcore::sampler::Cubemap *data = new xtcore::sampler::Cubemap;

    if (data) {
        std::string posx = asset_fetcher::resolve(deserialize_cstr(p->get_property_by_name(XTPROTO_LTRL_POSX)));
        std::string posy = asset_fetcher::resolve(deserialize_cstr(p->get_property_by_name(XTPROTO_LTRL_POSY)));
        std::string posz = asset_fetcher::resolve(deserialize_cstr(p->get_property_by_name(XTPROTO_LTRL_POSZ)));
        std::string negx = asset_fetcher::resolve(deserialize_cstr(p->get_property_by_name(XTPROTO_LTRL_NEGX)));
        std::string negy = asset_fetcher::resolve(deserialize_cstr(p->get_property_by_name(XTPROTO_LTRL_NEGY)));
        std::string negz = asset_fetcher::resolve(deserialize_cstr(p->get_property_by_name(XTPROTO_LTRL_NEGZ)));

        std::string base, file, fsource = source;
    	ncf::util::path_comp(fsource, base, file);

        auto resolve_face = [&](const std::string &face) -> std::string {
            if (face.empty() || path_is_absolute(face) || asset_fetcher::is_url(face)) return face;
            return base + face;
        };
        data->load(resolve_face(posx).c_str(), xtcore::sampler::CUBEMAP_FACE_RIGHT);
        data->load(resolve_face(posy).c_str(), xtcore::sampler::CUBEMAP_FACE_TOP);
        data->load(resolve_face(posz).c_str(), xtcore::sampler::CUBEMAP_FACE_FRONT);
        data->load(resolve_face(negx).c_str(), xtcore::sampler::CUBEMAP_FACE_LEFT);
        data->load(resolve_face(negy).c_str(), xtcore::sampler::CUBEMAP_FACE_BOTTOM);
        data->load(resolve_face(negz).c_str(), xtcore::sampler::CUBEMAP_FACE_BACK);
    }
    return data;
}

xtcore::sampler::ERP *deserialize_erp(const char *source, const ncf::NCF *p)
{
    if (!p) return 0;

    xtcore::sampler::ERP *data = new xtcore::sampler::ERP;

    if (data) {
        std::string src = asset_fetcher::resolve(
            deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_SOURCE)));

        std::string base, file, fsource = source;
        ncf::util::path_comp(fsource, base, file);

        std::string erp_path = (src.empty() || path_is_absolute(src) || asset_fetcher::is_url(src)) ? src : base + src;
        data->load(erp_path.c_str());
    }
    return data;
}

xtcore::asset::ICamera *deserialize_camera_tlp(const ncf::NCF *p)
{
    if (!p) return 0;

    asset::ICamera *data = new (std::nothrow) xtcore::camera::Perspective();

    xtcore::camera::Perspective *cam = (xtcore::camera::Perspective *)data;
    cam->position = deserialize_vec3(p, XTPROTO_PROP_POSITION);
    cam->target   = deserialize_vec3(p, XTPROTO_PROP_TARGET);
    cam->up       = deserialize_vec3(p, XTPROTO_PROP_UP);
    cam->fov      = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_FOV));
    cam->flength  = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_FLENGTH));
    cam->aperture = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_APERTURE));
    cam->aperture_blades = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_APERTURE_BLADES), cam->aperture_blades);
    cam->aperture_rotation = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_APERTURE_ROTATION), cam->aperture_rotation);

    return data;
}

xtcore::asset::ICamera *deserialize_camera_cbm(const ncf::NCF *p)
{
    if (!p) return 0;

    asset::ICamera *data = new (std::nothrow) xtcore::camera::Cubemap();

    xtcore::camera::Cubemap *cam = (xtcore::camera::Cubemap *)data;
    cam->position = deserialize_vec3(p, XTPROTO_PROP_POSITION);

    return data;
}

xtcore::asset::ICamera *deserialize_camera_ods(const ncf::NCF *p)
{
    if (!p) return 0;

    asset::ICamera *data = new (std::nothrow) xtcore::camera::ODS();

    xtcore::camera::ODS *cam = (xtcore::camera::ODS *)data;
    cam->position    = deserialize_vec3(p, XTPROTO_PROP_POSITION);
    cam->orientation = deserialize_vec3(p, XTPROTO_PROP_ORIENTATION);
    cam->ipd         = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_IPD));

    return data;
}

xtcore::asset::ICamera *deserialize_camera_erp(const ncf::NCF *p)
{
    if (!p) return 0;

    asset::ICamera *data = new (std::nothrow) xtcore::camera::ERP();

    xtcore::camera::ERP *cam = (xtcore::camera::ERP *)data;
    cam->position    = deserialize_vec3(p, XTPROTO_PROP_POSITION);
    cam->orientation = deserialize_vec3(p, XTPROTO_PROP_ORIENTATION);

    return data;
}

xtcore::asset::ICamera *deserialize_camera(const ncf::NCF *p)
{
    if (!p) return 0;

    xtcore::asset::ICamera *data = 0;

    std::string type = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_TYPE));

         if (!type.compare(XTPROTO_LTRL_CAM_THINLENS)) data = deserialize_camera_tlp(p);
    else if (!type.compare(XTPROTO_LTRL_CAM_ODS)     ) data = deserialize_camera_ods(p);
    else if (!type.compare(XTPROTO_LTRL_CAM_ERP)     ) data = deserialize_camera_erp(p);
    else if (!type.compare(XTPROTO_LTRL_CAM_CUBEMAP) ) data = deserialize_camera_cbm(p);
    else Log::handle().post_warning("Unsupported camera type %s [%s]. Skipping..", p->get_name(), type.c_str());

	return data;
}

xtcore::asset::ISurface *deserialize_geometry_mesh(const char *source, const ncf::NCF *p)
{
	if (!p) return 0;

    xtcore::asset::ISurface *data = new (std::nothrow) xtcore::surface::Mesh;

    nmesh::object_t obj;

    std::string f = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_SOURCE));

    std::string token;
    void* buffer = malloc(200 * sizeof(char));
    memset(buffer, 0, 200 * sizeof(char));
    int res = sscanf(f.c_str(), XTPROTO_FORMAT_GENERATE, (char*)buffer);
    if (buffer && *((char*)buffer)) token = (char*)buffer;
    free(buffer);
    std::transform(token.begin(), token.end(), token.begin(), [](unsigned char c) {
        return (char)std::tolower(c);
    });
    token.erase(std::remove_if(token.begin(), token.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }), token.end());

    // Procedural meshes
    if (res > 0) {
        if (!token.compare(XTPROTO_LTRL_ICOSAHEDRON)) {
            nmesh::generator::icosahedron(&obj);
        }
        else if (!token.compare(XTPROTO_LTRL_TETRAHEDRON)) {
            nmesh::generator::tetrahedron(&obj);
        }
        else if (!token.compare(XTPROTO_LTRL_CUBE) || !token.compare(XTPROTO_LTRL_HEXAHEDRON)) {
            nmesh::generator::cube(&obj);
        }
        else if (!token.compare(XTPROTO_LTRL_OCTAHEDRON)) {
            nmesh::generator::octahedron(&obj);
        }
        else if (!token.compare(XTPROTO_LTRL_DODECAHEDRON)) {
            nmesh::generator::dodecahedron(&obj);
        }
        else if (!token.compare(XTPROTO_LTRL_PYRAMID)) {
            float base_size = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_BASE_SIZE) : 0, 1.0f);
            if (base_size <= 0.0f) base_size = 1.0f;
            float height = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_HEIGHT) : 0, 1.0f);
            if (height <= 0.0f) height = 1.0f;
            nmesh::generator::pyramid(&obj, base_size, height);
        }
        else if (!token.compare(XTPROTO_LTRL_CAPSULE)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            if (i < 12) i = 12;
            nmesh::generator::capsule(&obj, (size_t)i);
        }
        else if (!token.compare(XTPROTO_LTRL_CYLINDER)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            if (i < 12) i = 12;
            nmesh::generator::cylinder(&obj, (size_t)i);
        }
        else if (!token.compare(XTPROTO_LTRL_CAPPED_CYLINDER)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            if (i < 12) i = 12;
            nmesh::generator::capped_cylinder(&obj, (size_t)i);
        }
        else if (!token.compare(XTPROTO_LTRL_CONE)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            if (i < 12) i = 12;
            nmesh::generator::cone(&obj, (size_t)i);
        }
        else if (!token.compare(XTPROTO_LTRL_TRUNCATED_CONE)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            if (i < 12) i = 12;
            nmesh::generator::truncated_cone(&obj, (size_t)i);
        }
        else if (!token.compare(XTPROTO_LTRL_RING)) {
            int i = 0;
            float radius = 1.0f;
            float height = 0.64f;
            float thickness = -1.0f;
            int hres = 1;

            if (p) {
                i = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_RESOLUTION));
                if (i < 16) i = 16;

                radius = (float)deserialize_numf(p->get_property_by_name(XTPROTO_PROP_RADIUS), radius);
                if (radius <= 0.0f) radius = 1.0f;

                height = (float)deserialize_numf(p->get_property_by_name(XTPROTO_PROP_HEIGHT), height);
                if (height <= 0.0f) height = 0.64f;

                thickness = (float)deserialize_numf(p->get_property_by_name(XTPROTO_PROP_THICKNESS), thickness);
                if (thickness <= 0.0f) thickness = -1.0f;

                hres = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_HEIGHT_RESOLUTION), hres);
                if (hres < 1) hres = 1;
            }

            nmesh::generator::ring(&obj,
                                   (size_t)i,
                                   radius,
                                   height,
                                   thickness,
                                   (size_t)hres);
        }
        else if (!token.compare(XTPROTO_LTRL_TORUS)) {
            int i = 0;
            float radius = 1.0f;
            float height = 0.64f;
            float thickness = -1.0f;
            int hres = 1;

            if (p) {
                i = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_RESOLUTION));
                if (i < 16) i = 16;

                radius = (float)deserialize_numf(p->get_property_by_name(XTPROTO_PROP_RADIUS), radius);
                if (radius <= 0.0f) radius = 1.0f;

                height = (float)deserialize_numf(p->get_property_by_name(XTPROTO_PROP_HEIGHT), height);
                if (height <= 0.0f) height = 0.64f;

                thickness = (float)deserialize_numf(p->get_property_by_name(XTPROTO_PROP_THICKNESS), thickness);
                if (thickness <= 0.0f) thickness = -1.0f;

                hres = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_HEIGHT_RESOLUTION), hres);
                if (hres < 1) hres = 1;
            }

            nmesh::generator::ring(&obj,
                                   (size_t)i,
                                   radius,
                                   height,
                                   thickness,
                                   (size_t)hres);
        }
        else if (!token.compare(XTPROTO_LTRL_ROUNDED_RING)) {
            int i = 0;
            float radius = 1.0f;
            float height = 0.64f;
            float thickness = -1.0f;
            int pres = 18;

            if (p) {
                i = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_RESOLUTION));
                if (i < 24) i = 24;

                radius = (float)deserialize_numf(p->get_property_by_name(XTPROTO_PROP_RADIUS), radius);
                if (radius <= 0.0f) radius = 1.0f;

                height = (float)deserialize_numf(p->get_property_by_name(XTPROTO_PROP_HEIGHT), height);
                if (height <= 0.0f) height = 0.64f;

                thickness = (float)deserialize_numf(p->get_property_by_name(XTPROTO_PROP_THICKNESS), thickness);
                if (thickness <= 0.0f) thickness = -1.0f;

                pres = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_PROFILE_RESOLUTION), pres);
                if (pres < 8) {
                    pres = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_CAP_RESOLUTION), pres);
                }
                if (pres < 8) pres = 8;
            }

            nmesh::generator::rounded_ring(&obj,
                                           (size_t)i,
                                           radius,
                                           height,
                                           thickness,
                                           (size_t)pres);
        }
        else if (!token.compare(XTPROTO_LTRL_TORUS_KNOT)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 48);
            if (i < 24) i = 24;
            nmesh::generator::torus_knot(&obj, (size_t)i);
        }
        else if (!token.compare(XTPROTO_LTRL_ICOSPHERE)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            if (i < 4) i = 4;
            nmesh::generator::icosphere(&obj, (size_t)i);
        }
        else if (!token.compare(XTPROTO_LTRL_GEODESIC_DOME)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            if (i < 4) i = 4;
            nmesh::generator::geodesic_dome(&obj, (size_t)i);
        }
        else if (!token.compare(XTPROTO_LTRL_ICOSA_CAGE)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            if (i < 8) i = 8;
            nmesh::generator::icosa_cage(&obj, (size_t)i);
        }
        else if (!token.compare(XTPROTO_LTRL_MENGER_SPONGE)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 2);
            if (i < 1) i = 1;
            nmesh::generator::menger_sponge(&obj, (size_t)i);
        }
        else if (!token.compare(XTPROTO_LTRL_MENGER_SPONGE_IMPLICIT)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 2);
            if (i < 1) i = 1;
            nmesh::generator::menger_sponge_implicit(&obj, (size_t)i);
        }
        else if (!token.compare(XTPROTO_LTRL_SIERPINSKI_TETRAHEDRON)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 2);
            if (i < 1) i = 1;
            nmesh::generator::sierpinski_tetrahedron(&obj, (size_t)i);
        }
        else if (!token.compare(XTPROTO_LTRL_SIERPINSKI_TETRAHEDRON_IMPLICIT)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 2);
            if (i < 1) i = 1;
            nmesh::generator::sierpinski_tetrahedron_implicit(&obj, (size_t)i);
        }
        else if (!token.compare(XTPROTO_LTRL_MOBIUS_STRIP)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 64);
            if (i < 24) i = 24;
            float radius = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_RADIUS) : 0, 1.0f);
            if (radius <= 0.0f) radius = 1.0f;
            float width = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_WIDTH) : 0, 0.64f);
            if (width <= 0.0f) width = 0.64f;
            nmesh::generator::mobius_strip(&obj, (size_t)i, radius, width);
        }
        else if (!token.compare(XTPROTO_LTRL_KLEIN_BOTTLE)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 64);
            if (i < 24) i = 24;
            nmesh::generator::klein_bottle(&obj, (size_t)i);
        }
        else if (!token.compare(XTPROTO_LTRL_HAIRBALL)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            if (i < 8) i = 8;
            if (i > 2048) i = 2048;

            int seed = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_SEED) : 0, 1337);
            float radius = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_RADIUS) : 0, 1.0f);
            if (radius <= 0.0f) radius = 1.0f;

            int fibers_i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_FIBERS) : 0, 0);
            if (fibers_i < 0) fibers_i = 0;
            size_t fibers = (size_t)fibers_i;

            nmesh::generator::hairball(&obj, (size_t)i, seed, radius, fibers);
        }
        else if (!token.compare(XTPROTO_LTRL_SHELL_SPIRAL)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 64);
            if (i < 16) i = 16;
            if (i > 4096) i = 4096;

            float turns = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_TURNS) : 0, 4.0f);
            if (turns < 0.5f) turns = 0.5f;
            if (turns > 24.0f) turns = 24.0f;

            float growth = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_GROWTH) : 0, 0.22f);
            if (growth < 0.01f) growth = 0.01f;
            if (growth > 1.0f) growth = 1.0f;

            float tube_radius = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_TUBE_RADIUS) : 0, 0.14f);
            if (tube_radius <= 0.0f) tube_radius = 0.14f;
            if (tube_radius > 2.0f) tube_radius = 2.0f;

            nmesh::generator::shell_spiral(&obj, (size_t)i, turns, growth, tube_radius);
        }
        else if (!token.compare(XTPROTO_LTRL_ROCK)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 48);
            if (i < 8) i = 8;
            if (i > 2048) i = 2048;

            int seed = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_SEED) : 0, 1337);
            float radius = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_RADIUS) : 0, 1.0f);
            if (radius <= 0.0f) radius = 1.0f;

            float roughness = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_ROUGH) : 0, 0.35f);
            if (roughness < 0.0f) roughness = 0.0f;
            if (roughness > 2.0f) roughness = 2.0f;

            int octaves_i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_OCTAVES) : 0, 4);
            if (octaves_i < 1) octaves_i = 1;
            if (octaves_i > 8) octaves_i = 8;

            nmesh::generator::rock(&obj, (size_t)i, seed, radius, roughness, (size_t)octaves_i);
        }
        else if (!token.compare(XTPROTO_LTRL_TERRAIN)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 128);
            if (i < 2) i = 2;
            if (i > 1024) i = 1024;

            nmath::Vector3f dimensions = deserialize_vec3(p, XTPROTO_PROP_DIMENSIONS, nmath::Vector3f(8.0f, 1.5f, 8.0f));
            if (dimensions.x <= 0.0f) dimensions.x = 8.0f;
            if (dimensions.y < 0.0f) dimensions.y = 1.5f;
            if (dimensions.z <= 0.0f) dimensions.z = 8.0f;
            nmesh::generator::plane(&obj, (size_t)i);

            xtcore::sampler::ISampler *height_sampler = 0;
            if (p && p->query_group(XTPROTO_PROP_HEIGHT_SAMPLER)) {
                height_sampler = deserialize_sampler_node(source, p->get_group_by_name(XTPROTO_PROP_HEIGHT_SAMPLER));
            }
            if (!height_sampler) {
                height_sampler = new (std::nothrow) xtcore::sampler::SceneryHeightfield();
            }
            apply_heightfield_to_mesh(obj, height_sampler, dimensions);
            delete height_sampler;
        }
        else if (!token.compare(XTPROTO_LTRL_DRAPED_CLOTH_STRIP)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 96);
            if (i < 8) i = 8;
            if (i > 512) i = 512;

            nmath::Vector3f dimensions = deserialize_vec3(p, XTPROTO_PROP_DIMENSIONS, nmath::Vector3f(2.0f, 0.9f, 3.2f));
            if (dimensions.x <= 0.0f) dimensions.x = 2.0f;
            if (dimensions.y < 0.0f) dimensions.y = 0.9f;
            if (dimensions.z <= 0.0f) dimensions.z = 3.2f;

            float folds = (float)deserialize_numf(p ? p->get_property_by_name("folds") : 0, 3.0f);
            float edge_lift = (float)deserialize_numf(p ? p->get_property_by_name("edge_lift") : 0, 0.18f);
            float curl = (float)deserialize_numf(p ? p->get_property_by_name("curl") : 0, 0.28f);
            float taper = (float)deserialize_numf(p ? p->get_property_by_name("taper") : 0, 0.12f);
            float sway = (float)deserialize_numf(p ? p->get_property_by_name("sway") : 0, 0.20f);
            float asymmetry = (float)deserialize_numf(p ? p->get_property_by_name("asymmetry") : 0, 0.0f);
            float pinned = (float)deserialize_numf(p ? p->get_property_by_name("pinned") : 0, 0.55f);
            nmesh::generator::draped_cloth_strip(&obj, (size_t)i, dimensions, folds, edge_lift, curl, taper, sway, asymmetry, pinned);
        }
        else if (!token.compare(XTPROTO_LTRL_CHAIN_LINK)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 64);
            if (i < 12) i = 12;
            if (i > 2048) i = 2048;

            int count_i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_COUNT) : 0, 6);
            if (count_i < 1) count_i = 1;
            if (count_i > 128) count_i = 128;

            float major_radius = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_MAJOR_RADIUS) : 0, 0.55f);
            if (major_radius <= 0.0f) major_radius = 0.55f;

            float minor_radius = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_MINOR_RADIUS) : 0, 0.16f);
            if (minor_radius <= 0.0f) minor_radius = 0.16f;

            float spacing = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_SPACING) : 0, 1.05f);
            if (spacing < 0.5f) spacing = 0.5f;
            if (spacing > 2.0f) spacing = 2.0f;

            std::vector<nmath::Vector3f> spline;
            if (p && p->query_group(XTPROTO_PROP_SPLINE)) {
                spline = deserialize_spline_points(p->get_group_by_name(XTPROTO_PROP_SPLINE));
            }

            nmesh::generator::chain_link(&obj, (size_t)i, (size_t)count_i, major_radius, minor_radius, spacing, spline);
        }
        else if (!token.compare(XTPROTO_LTRL_LATHE)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 64);
            if (i < 12) i = 12;
            if (i > 4096) i = 4096;

            bool cap_ends = deserialize_bool(p ? p->get_property_by_name(XTPROTO_PROP_CAP_ENDS) : 0, true);

            std::vector<nmath::Vector2f> profile;
            if (p && p->query_group(XTPROTO_PROP_PROFILE)) {
                profile = deserialize_lathe_profile(p->get_group_by_name(XTPROTO_PROP_PROFILE));
            }

            nmesh::generator::lathe(&obj, profile, (size_t)i, cap_ends);
        }
        else if (!token.compare(XTPROTO_LTRL_PLANE)) {
            int i = 0;

            if (p) {
                i = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_RESOLUTION));
                if (i < 1) i = 1;
            }

            nmesh::generator::plane(&obj, i);
        }
        else if (!token.compare(XTPROTO_LTRL_SNOWFLAKE)) {
            int i = 0;

            if (p) {
                i = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_RESOLUTION));
                if (i < 0) i = 0;
            }

            nmesh::generator::snowflake(&obj, (size_t)i);
        }

        else if (!token.compare(XTPROTO_LTRL_GEAR)) {
            int res = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            res = nmath::clamp(res, 8, 512);
            int tc = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_TOOTH_COUNT) : 0, 12);
            tc = nmath::clamp(tc, 3, 128);
            float tooth_depth   = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_TOOTH_DEPTH)   : 0, 0.1);
            float inner_radius  = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_INNER_RADIUS)  : 0, 0.2);
            float outer_radius  = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_OUTER_RADIUS)  : 0, 0.5);
            float height        = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_HEIGHT)        : 0, 0.2);
            nmesh::generator::gear(&obj, (size_t)res, (size_t)tc, tooth_depth, inner_radius, outer_radius, height);
        }
        else if (!token.compare(XTPROTO_LTRL_SPRING)) {
            int res = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            res = nmath::clamp(res, 8, 512);
            float coils         = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_COILS)         : 0, 6.0);
            float wire_radius   = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_WIRE_RADIUS)   : 0, 0.05);
            float spring_radius = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_SPRING_RADIUS) : 0, 0.3);
            float height        = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_HEIGHT)        : 0, 1.2);
            nmesh::generator::spring(&obj, (size_t)res, coils, wire_radius, spring_radius, height);
        }
        else if (!token.compare(XTPROTO_LTRL_HEMISPHERE)) {
            int res = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            res = nmath::clamp(res, 8, 512);
            nmesh::generator::hemisphere(&obj, (size_t)res);
        }
        else if (!token.compare(XTPROTO_LTRL_DISC)) {
            int res = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            res = nmath::clamp(res, 8, 512);
            float inner_radius  = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_INNER_RADIUS)  : 0, 0.0);
            float outer_radius  = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_OUTER_RADIUS)  : 0, 1.0);
            nmesh::generator::disc(&obj, (size_t)res, inner_radius, outer_radius);
        }
        else if (!token.compare(XTPROTO_LTRL_STAR)) {
            int res = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            res = nmath::clamp(res, 8, 512);
            int pts = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_POINTS) : 0, 5);
            pts = nmath::clamp(pts, 3, 32);
            float inner_radius  = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_INNER_RADIUS)  : 0, 0.4);
            float outer_radius  = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_OUTER_RADIUS)  : 0, 1.0);
            float height        = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_HEIGHT)        : 0, 0.2);
            nmesh::generator::star(&obj, (size_t)res, (size_t)pts, inner_radius, outer_radius, height);
        }
        else if (!token.compare(XTPROTO_LTRL_SUPERELLIPSOID)) {
            int res = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            res = nmath::clamp(res, 8, 512);
            float e1 = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_E1) : 0, 1.0);
            float e2 = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_E2) : 0, 1.0);
            nmesh::generator::superellipsoid(&obj, (size_t)res, e1, e2);
        }
        else if (!token.compare(XTPROTO_LTRL_CRYSTAL)) {
            int res = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            res = nmath::clamp(res, 8, 512);
            int count = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_COUNT) : 0, 5);
            count = nmath::clamp(count, 1, 32);
            float radius     = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_RADIUS)     : 0, 0.8);
            float height     = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_HEIGHT)     : 0, 1.5);
            float tip_height = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_TIP_HEIGHT) : 0, 0.6);
            int seed = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_SEED) : 0, 1337);
            nmesh::generator::crystal(&obj, (size_t)res, (size_t)count, radius, height, tip_height, seed);
        }
        else if (!token.compare(XTPROTO_LTRL_TREE)) {
            int res = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            res = nmath::clamp(res, 8, 256);
            int depth        = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_DEPTH)        : 0, 4);
            int branch_count = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_BRANCH_COUNT) : 0, 3);
            float branch_angle  = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_BRANCH_ANGLE)  : 0, 0.6);
            float trunk_height  = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_TRUNK_HEIGHT)  : 0, 1.2);
            float trunk_radius  = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_TRUNK_RADIUS)  : 0, 0.08);
            int seed = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_SEED) : 0, 1337);
            nmesh::generator::tree(&obj, (size_t)res, depth, branch_count, branch_angle, trunk_height, trunk_radius, seed);
        }
        else if (!token.compare(XTPROTO_LTRL_CORAL)) {
            int res = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
            res = nmath::clamp(res, 8, 256);
            int depth        = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_DEPTH)        : 0, 4);
            int branch_count = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_BRANCH_COUNT) : 0, 4);
            float branch_angle  = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_BRANCH_ANGLE)  : 0, 0.7);
            float height        = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_HEIGHT)        : 0, 1.0);
            float branch_radius = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_BRANCH_RADIUS) : 0, 0.05);
            int seed = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_SEED) : 0, 1337);
            nmesh::generator::coral(&obj, (size_t)res, depth, branch_count, branch_angle, height, branch_radius, seed);
        }

        else Log::handle().post_message("Invalid mesh generator: %s (%s)", token.c_str(), f.c_str());
    }
    // External sources
    else {
        // Open source file from relative path
	    std::string base, file, fsource = source;
		ncf::util::path_comp(fsource, base, file);
        if (!path_is_absolute(f)) base.append(f);
        else base = f;

	    Log::handle().post_message("Loading data from %s", base.c_str());
        auto t_import_0 = std::chrono::steady_clock::now();
        xtcore::imported_asset_t imported;
        std::string import_error;
        if (!xtcore::import_asset_file(base.c_str(), imported, import_error)) {
            Log::handle().post_warning("Failed to load mesh from %s (%s)", f.c_str(), import_error.c_str());
            delete data;
            return 0;
        }
        obj.attributes = imported.attributes;
        obj.shapes.clear();
        for (size_t i = 0; i < imported.shapes.size(); ++i) {
            obj.shapes.push_back(imported.shapes[i].shape);
        }
        auto t_import_1 = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_import_1 - t_import_0).count();
        Log::handle().post_message("Asset import done: %s (%.0f ms)", base.c_str(), ms);
    }

    if (p->query_group(XTPROTO_MODIFIERS)) {
        auto t_mod_0 = std::chrono::steady_clock::now();
        ncf::NCF *mods = p->get_group_by_name(XTPROTO_MODIFIERS);

    	nmath::Vector3f xform_rot = deserialize_vec3(mods, XTPROTO_PROP_ROTATION    , nmath::Vector3f(0, 0, 0));
    	nmath::Vector3f xform_scl = deserialize_vec3(mods, XTPROTO_PROP_SCALE       , nmath::Vector3f(1, 1, 1));
    	nmath::Vector3f xform_tsl = deserialize_vec3(mods, XTPROTO_PROP_TRANSLATION , nmath::Vector3f(0, 0, 0));
    	nmesh::mutator::rotate   (obj, xform_rot.x, xform_rot.y, xform_rot.z);
    	nmesh::mutator::scale    (obj, xform_scl.x, xform_scl.y, xform_scl.z);
    	nmesh::mutator::translate(obj, xform_tsl.x, xform_tsl.y, xform_tsl.z);

        if (mods->query_property(XTPROTO_FLIP_NORMALS)) {
            bool flag = deserialize_bool(mods->get_property_by_name(XTPROTO_FLIP_NORMALS));
            Log::handle().post_message("Applying modifier: flip normals..");
            if (flag) nmesh::mutator::invert_normals(&obj);
        }

        if (mods->query_group(XTPROTO_EXTRUDE)) {
            xtcore::sampler::Cubemap *cb = deserialize_cubemap(source, mods->get_group_by_name(XTPROTO_EXTRUDE));
            Log::handle().post_message("Applying modifier: extrude..");
            xtcore::auxiliary::extrude(&obj, cb);
        }
        auto t_mod_1 = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_mod_1 - t_mod_0).count();
        Log::handle().post_message("Mesh modifiers done: %s (%.0f ms)", f.c_str(), ms);
    }

    if (obj.shapes.empty()) {
        Log::handle().post_warning("Mesh source produced no shape data: %s", f.c_str());
        delete data;
        return 0;
    }

    Log::handle().post_message("Building octree..");
    auto t_oct_0 = std::chrono::steady_clock::now();
    if (obj.shapes.size() == 1) {
        ((xtcore::surface::Mesh *)data)->build_octree(obj.shapes[0], obj.attributes);
    } else {
	    ((xtcore::surface::Mesh *)data)->build_octree(obj);
    }
    auto t_oct_1 = std::chrono::steady_clock::now();
    const double oct_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_oct_1 - t_oct_0).count();
    Log::handle().post_message("Mesh octree build done: %s (%zu shapes, %.0f ms)", f.c_str(), obj.shapes.size(), oct_ms);

    return data;
}

bool csg_supports_leaf_type(const std::string &type_lc)
{
    return !type_lc.compare(XTPROTO_LTRL_SPHERE)
        || !type_lc.compare(XTPROTO_LTRL_POINT)
        || !type_lc.compare(XTPROTO_LTRL_PLANE)
        || !type_lc.compare(XTPROTO_LTRL_MENGER_SPONGE)
        || !type_lc.compare(XTPROTO_LTRL_SIERPINSKI_TETRAHEDRON)
        || !type_lc.compare(XTPROTO_LTRL_MANDELBULB)
        || !type_lc.compare(XTPROTO_LTRL_JULIA);
}

xtcore::surface::CSG::op_t csg_parse_op(const std::string &token, bool &ok)
{
    std::string op = token;
    std::transform(op.begin(), op.end(), op.begin(), [](unsigned char c) {
        return (char)std::tolower(c);
    });
    op.erase(std::remove_if(op.begin(), op.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }), op.end());

    if (!op.compare(XTPROTO_LTRL_UNION)) {
        ok = true;
        return xtcore::surface::CSG::OP_UNION;
    }
    if (!op.compare(XTPROTO_LTRL_SOFT_UNION) || !op.compare(XTPROTO_LTRL_SMOOTH_UNION)) {
        ok = true;
        return xtcore::surface::CSG::OP_SOFT_UNION;
    }
    if (!op.compare(XTPROTO_LTRL_INTERSECTION)) {
        ok = true;
        return xtcore::surface::CSG::OP_INTERSECTION;
    }
    if (!op.compare(XTPROTO_LTRL_DIFFERENCE)) {
        ok = true;
        return xtcore::surface::CSG::OP_DIFFERENCE;
    }
    ok = false;
    return xtcore::surface::CSG::OP_UNION;
}

xtcore::asset::ISurface *deserialize_geometry_csg_node(const char *source, const ncf::NCF *p, size_t depth)
{
    if (!p) return 0;
    if (depth > 32) {
        Log::handle().post_warning("CSG recursion depth exceeded at %s", p->get_name());
        return 0;
    }

    const bool has_left = p->query_group(XTPROTO_PROP_LEFT);
    const bool has_right = p->query_group(XTPROTO_PROP_RIGHT);
    const bool has_op = p->query_property(XTPROTO_PROP_OP);

    if (has_left || has_right || has_op) {
        if (!has_left || !has_right || !has_op) {
            Log::handle().post_warning("CSG node %s requires op + left + right", p->get_name());
            return 0;
        }

        bool op_ok = false;
        xtcore::surface::CSG::op_t op = csg_parse_op(
            deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_OP)),
            op_ok
        );
        if (!op_ok) {
            Log::handle().post_warning("Invalid CSG op for node %s", p->get_name());
            return 0;
        }

        xtcore::asset::ISurface *lhs = deserialize_geometry_csg_node(source, p->get_group_by_name(XTPROTO_PROP_LEFT), depth + 1);
        if (!lhs) return 0;

        xtcore::asset::ISurface *rhs = deserialize_geometry_csg_node(source, p->get_group_by_name(XTPROTO_PROP_RIGHT), depth + 1);
        if (!rhs) {
            delete lhs;
            return 0;
        }

        xtcore::surface::CSG *node = new (std::nothrow) xtcore::surface::CSG();
        if (!node) {
            delete lhs;
            delete rhs;
            return 0;
        }

        node->op = op;
        if (op == xtcore::surface::CSG::OP_SOFT_UNION) {
            nmath::scalar_t smoothness = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_SMOOTHNESS));
            if (!std::isfinite((double)smoothness) || smoothness <= (nmath::scalar_t)EPSILON) {
                smoothness = (nmath::scalar_t)0.15;
            }
            node->smoothness = smoothness;
        }
        node->left = lhs;
        node->right = rhs;
        node->calc_aabb();
        return node;
    }

    std::string type = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_TYPE));
    std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) {
        return (char)std::tolower(c);
    });

    if (type.empty()) {
        Log::handle().post_warning("CSG leaf at %s is missing type", p->get_name());
        return 0;
    }
    if (!csg_supports_leaf_type(type)) {
        Log::handle().post_warning("Unsupported CSG leaf type %s at %s", type.c_str(), p->get_name());
        return 0;
    }

    return deserialize_geometry(source, p);
}

xtcore::asset::ISurface *deserialize_geometry_csg(const char *source, const ncf::NCF *p)
{
    return deserialize_geometry_csg_node(source, p, 0);
}

xtcore::asset::ISurface *deserialize_geometry(const char *source, const ncf::NCF *p)
{
	if (!p) return 0;

    xtcore::asset::ISurface *data = 0;

	std::string type = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_TYPE));

	if (!type.compare(XTPROTO_LTRL_PLANE)) {
		data = new (std::nothrow) xtcore::surface::Plane;
		((xtcore::surface::Plane *)data)->normal = deserialize_vec3(p, XTPROTO_PROP_NORMAL);
		((xtcore::surface::Plane *)data)->offset = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_DISTANCE));
	}
	else if (!type.compare(XTPROTO_LTRL_SPHERE)) {
		data = new (std::nothrow) xtcore::surface::Sphere;
		((xtcore::surface::Sphere *)data)->origin = deserialize_vec3(p, XTPROTO_PROP_POSITION);
		((xtcore::surface::Sphere *)data)->radius = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_RADIUS));
	}
    else if (!type.compare(XTPROTO_LTRL_POINT)) {
        data = new (std::nothrow) xtcore::surface::Sphere;
		((xtcore::surface::Sphere *)data)->origin = deserialize_vec3(p, XTPROTO_PROP_POSITION);
		((xtcore::surface::Sphere *)data)->radius = (nmath::scalar_t)EPSILON;
    }
	else if (!type.compare(XTPROTO_LTRL_TRIANGLE)) {
		data = new (std::nothrow) xtcore::surface::Triangle;

        xtcore::surface::Triangle *tri = (xtcore::surface::Triangle *)data;

		for (size_t i = 0; i < 3; ++i) {
			ncf::NCF *vnode = p->get_group_by_name(XTPROTO_PROP_VRTXDATA);

			tri->v[0]   = deserialize_vec3(vnode, XTPROTO_PROP_VRT_0);
			tri->v[1]   = deserialize_vec3(vnode, XTPROTO_PROP_VRT_1);
			tri->v[2]   = deserialize_vec3(vnode, XTPROTO_PROP_VRT_2);
			tri->tc[0]  = deserialize_tex2(vnode, XTPROTO_PROP_VRT_0);
			tri->tc[1]  = deserialize_tex2(vnode, XTPROTO_PROP_VRT_1);
			tri->tc[2]  = deserialize_tex2(vnode, XTPROTO_PROP_VRT_2);

            tri->calc_aabb();
		}
	}
    else if (!type.compare(XTPROTO_LTRL_MENGER_SPONGE)) {
        data = new (std::nothrow) xtcore::surface::MengerSponge;
        xtcore::surface::MengerSponge *f = (xtcore::surface::MengerSponge *)data;
        f->origin = deserialize_vec3(p, XTPROTO_PROP_POSITION, nmath::Vector3f(0, 0, 0));
        f->orientation = deserialize_vec3(p, XTPROTO_PROP_ORIENTATION, nmath::Vector3f(0, 0, 0));
        f->radius = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_RADIUS), 1.0f);
        if (f->radius <= 0.0f) f->radius = 1.0f;
        int i = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_RESOLUTION), 2);
        if (i < 1) i = 1;
        if (i > 5) i = 5;
        f->iterations = (size_t)i;
    }
    else if (!type.compare(XTPROTO_LTRL_SIERPINSKI_TETRAHEDRON)) {
        data = new (std::nothrow) xtcore::surface::SierpinskiTetrahedron;
        xtcore::surface::SierpinskiTetrahedron *f = (xtcore::surface::SierpinskiTetrahedron *)data;
        f->origin = deserialize_vec3(p, XTPROTO_PROP_POSITION, nmath::Vector3f(0, 0, 0));
        f->radius = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_RADIUS), 1.0f);
        if (f->radius <= 0.0f) f->radius = 1.0f;
        int i = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_RESOLUTION), 2);
        if (i < 1) i = 1;
        if (i > 8) i = 8;
        f->iterations = (size_t)i;
    }
    else if (!type.compare(XTPROTO_LTRL_MANDELBULB)) {
        data = new (std::nothrow) xtcore::surface::Mandelbulb;
        xtcore::surface::Mandelbulb *f = (xtcore::surface::Mandelbulb *)data;
        f->origin = deserialize_vec3(p, XTPROTO_PROP_POSITION, nmath::Vector3f(0, 0, 0));
        f->radius = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_RADIUS), 1.0f);
        if (f->radius <= 0.0f) f->radius = 1.0f;
        int i = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_RESOLUTION), 18);
        if (i < 1) i = 1;
        if (i > 64) i = 64;
        f->iterations = (size_t)i;
        f->power = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_POWER), 8.0f);
        if (f->power < 2.0f) f->power = 2.0f;
        if (f->power > 16.0f) f->power = 16.0f;
        f->bailout = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_BAILOUT), 4.0f);
        if (f->bailout < 2.0f) f->bailout = 2.0f;
        if (f->bailout > 64.0f) f->bailout = 64.0f;
    }
    else if (!type.compare(XTPROTO_LTRL_JULIA)) {
        data = new (std::nothrow) xtcore::surface::JuliaFractal;
        xtcore::surface::JuliaFractal *f = (xtcore::surface::JuliaFractal *)data;
        f->origin = deserialize_vec3(p, XTPROTO_PROP_POSITION, nmath::Vector3f(0, 0, 0));
        f->julia_c = deserialize_vec3(p, XTPROTO_PROP_JULIA_C, nmath::Vector3f(-0.24f, 0.74f, 0.12f));
        f->radius = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_RADIUS), 1.0f);
        if (f->radius <= 0.0f) f->radius = 1.0f;
        int i = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_RESOLUTION), 18);
        if (i < 1) i = 1;
        if (i > 64) i = 64;
        f->iterations = (size_t)i;
        f->power = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_POWER), 8.0f);
        if (f->power < 2.0f) f->power = 2.0f;
        if (f->power > 16.0f) f->power = 16.0f;
        f->bailout = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_BAILOUT), 4.0f);
        if (f->bailout < 2.0f) f->bailout = 2.0f;
        if (f->bailout > 64.0f) f->bailout = 64.0f;
    }
    else if (!type.compare(XTPROTO_LTRL_CSG)) {
        data = deserialize_geometry_csg(source, p);
    }
	// - Mesh
	else if (!type.compare(XTPROTO_LTRL_MESH)) data = deserialize_geometry_mesh(source, p);
	// unknown
	else {
		Log::handle().post_warning("Unsupported geometry type %s [%s]. Skipping..", p->get_name(), type.c_str());
		return 0;
	}

    if (data) {
    	data->uv_scale = nmath::Vector2f(
    		deserialize_numf(p->get_property_by_name(XTPROTO_PROP_USCALE), data->uv_scale.x),
	    	deserialize_numf(p->get_property_by_name(XTPROTO_PROP_VSCALE), data->uv_scale.y)
    	);

        data->calc_aabb();
    }

	return data;
}

xtcore::sampler::ISampler *deserialize_rgba(const ncf::NCF *p)
{
    xtcore::sampler::SolidColor *sampler = new (std::nothrow) xtcore::sampler::SolidColor();
    nimg::ColorRGBf col = deserialize_col3(p, XTPROTO_VALUE);
    ((xtcore::sampler::SolidColor *)sampler)->set(col);
    return sampler;
}

xtcore::sampler::ISampler *deserialize_graphpaper(const ncf::NCF *p)
{
    xtcore::sampler::GraphPaper *sampler = new (std::nothrow) xtcore::sampler::GraphPaper();
    if (!sampler || !p) return sampler;

    sampler->base_color = deserialize_col3(p, "base", sampler->base_color);
    sampler->minor_color = deserialize_col3(p, "minor", sampler->minor_color);
    sampler->major_color = deserialize_col3(p, "major", sampler->major_color);
    sampler->scale = deserialize_numf(p->get_property_by_name("scale"), sampler->scale);
    sampler->minor_width = deserialize_numf(p->get_property_by_name("minor_width"), sampler->minor_width);
    sampler->major_width = deserialize_numf(p->get_property_by_name("major_width"), sampler->major_width);
    sampler->major_every = deserialize_numi(p->get_property_by_name("major_every"), sampler->major_every);
    return sampler;
}

xtcore::sampler::ISampler *deserialize_checker(const ncf::NCF *p)
{
    xtcore::sampler::Checker *sampler = new (std::nothrow) xtcore::sampler::Checker();
    if (!sampler || !p) return sampler;

    sampler->color_a  = deserialize_col3(p, "a", sampler->color_a);
    sampler->color_b  = deserialize_col3(p, "b", sampler->color_b);
    sampler->scale_u  = deserialize_numf(p->get_property_by_name("scale_u"), sampler->scale_u);
    sampler->scale_v  = deserialize_numf(p->get_property_by_name("scale_v"), sampler->scale_v);
    sampler->offset_u = deserialize_numf(p->get_property_by_name("offset_u"), sampler->offset_u);
    sampler->offset_v = deserialize_numf(p->get_property_by_name("offset_v"), sampler->offset_v);
    return sampler;
}

xtcore::sampler::ISampler *deserialize_weave(const ncf::NCF *p)
{
    xtcore::sampler::Weave *sampler = new (std::nothrow) xtcore::sampler::Weave();
    if (!sampler || !p) return sampler;

    sampler->base_color = deserialize_col3(p, "base", sampler->base_color);
    sampler->warp_color = deserialize_col3(p, "warp", sampler->warp_color);
    sampler->weft_color = deserialize_col3(p, "weft", sampler->weft_color);
    sampler->scale = deserialize_numf(p->get_property_by_name("scale"), sampler->scale);
    sampler->band_width = deserialize_numf(p->get_property_by_name("band_width"), sampler->band_width);
    return sampler;
}

xtcore::sampler::ISampler *deserialize_fbm_marble(const ncf::NCF *p)
{
    xtcore::sampler::FBMMarble *sampler = new (std::nothrow) xtcore::sampler::FBMMarble();
    if (!sampler || !p) return sampler;

    sampler->color_a = deserialize_col3(p, "a", sampler->color_a);
    sampler->color_b = deserialize_col3(p, "b", sampler->color_b);
    sampler->vein_color = deserialize_col3(p, "vein", sampler->vein_color);
    sampler->scale = deserialize_numf(p->get_property_by_name("scale"), sampler->scale);
    sampler->vein_frequency = deserialize_numf(p->get_property_by_name("vein_frequency"), sampler->vein_frequency);
    sampler->turbulence = deserialize_numf(p->get_property_by_name("turbulence"), sampler->turbulence);
    sampler->octaves = deserialize_numi(p->get_property_by_name("octaves"), sampler->octaves);
    sampler->lacunarity = deserialize_numf(p->get_property_by_name("lacunarity"), sampler->lacunarity);
    sampler->gain = deserialize_numf(p->get_property_by_name("gain"), sampler->gain);
    sampler->vein_strength = deserialize_numf(p->get_property_by_name("vein_strength"), sampler->vein_strength);
    sampler->vein_sharpness = deserialize_numf(p->get_property_by_name("vein_sharpness"), sampler->vein_sharpness);
    return sampler;
}

xtcore::sampler::ISampler *deserialize_voronoi_normal(const ncf::NCF *p)
{
    xtcore::sampler::VoronoiNormal *sampler = new (std::nothrow) xtcore::sampler::VoronoiNormal();
    if (!sampler || !p) return sampler;

    sampler->cells = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_CELLS), sampler->cells);
    if (sampler->cells < 1) sampler->cells = 1;

    sampler->max_deviation = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_MAX_DEVIATION), sampler->max_deviation);
    if (sampler->max_deviation < (nmath::scalar_t)0.0) sampler->max_deviation = (nmath::scalar_t)0.0;
    if (sampler->max_deviation > (nmath::scalar_t)89.0) sampler->max_deviation = (nmath::scalar_t)89.0;

    sampler->seed = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_SEED), sampler->seed);
    return sampler;
}

xtcore::sampler::ISampler *deserialize_scenery_heightfield(const ncf::NCF *p)
{
    xtcore::sampler::SceneryHeightfield *sampler = new (std::nothrow) xtcore::sampler::SceneryHeightfield();
    if (!sampler || !p) return sampler;

    sampler->seed = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_SEED), sampler->seed);
    sampler->scale = deserialize_numf(p->get_property_by_name("scale"), sampler->scale);
    sampler->octaves = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_OCTAVES), sampler->octaves);
    sampler->lacunarity = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_LACUNARITY), sampler->lacunarity);
    sampler->gain = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_GAIN), sampler->gain);
    sampler->ridge_strength = deserialize_numf(p->get_property_by_name("ridge_strength"), sampler->ridge_strength);
    sampler->mountain_strength = deserialize_numf(p->get_property_by_name("mountain_strength"), sampler->mountain_strength);
    sampler->valley_strength = deserialize_numf(p->get_property_by_name("valley_strength"), sampler->valley_strength);
    return sampler;
}

xtcore::sampler::ISampler *deserialize_sampler_node(const char *source, const ncf::NCF *entry)
{
    if (!entry) return 0;
    std::string type = deserialize_cstr(entry->get_property_by_name(XTPROTO_PROP_TYPE));

         if (!type.compare(XTPROTO_TEXTURE )) return deserialize_texture(source, entry);
    else if (!type.compare(XTPROTO_CUBEMAP )) return deserialize_cubemap(source, entry);
    else if (!type.compare(XTPROTO_ERP     )) return deserialize_erp(source, entry);
    else if (!type.compare(XTPROTO_GRADIENT)) return deserialize_gradient(entry);
    else if (!type.compare(XTPROTO_RAYLEIGH_SKY)) return deserialize_rayleigh_sky(entry);
    else if (!type.compare(XTPROTO_GRAPHPAPER)) return deserialize_graphpaper(entry);
    else if (!type.compare(XTPROTO_CHECKER)) return deserialize_checker(entry);
    else if (!type.compare(XTPROTO_WEAVE)) return deserialize_weave(entry);
    else if (!type.compare(XTPROTO_FBM_MARBLE)) return deserialize_fbm_marble(entry);
    else if (!type.compare(XTPROTO_VORONOI_NORMAL)) return deserialize_voronoi_normal(entry);
    else if (!type.compare(XTPROTO_SCENERY_HEIGHTFIELD)) return deserialize_scenery_heightfield(entry);
    else if (!type.compare(XTPROTO_COLOR)) return deserialize_rgba(entry);
    return 0;
}

xtcore::asset::IMaterial *deserialize_material(const char *source, const ncf::NCF *p)
{
	if (!p) return 0;

    xtcore::asset::IMaterial *data = 0;

	std::string type = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_TYPE));

	     if (!type.compare(XTPROTO_LTRL_LAMBERT)   ) data = new (std::nothrow) xtcore::asset::material::Lambert();
	else if (!type.compare(XTPROTO_LTRL_PHONG)     ) data = new (std::nothrow) xtcore::asset::material::Phong();
	else if (!type.compare(XTPROTO_LTRL_BLINNPHONG)) data = new (std::nothrow) xtcore::asset::material::BlinnPhong();
	else if (!type.compare(XTPROTO_LTRL_EMISSIVE)  ) data = new (std::nothrow) xtcore::asset::material::Emissive();
	else if (!type.compare(XTPROTO_LTRL_DIELECTRIC)) data = new (std::nothrow) xtcore::asset::material::Dielectric();
    else if (!type.compare(XTPROTO_LTRL_PRINCIPLED)) data = new (std::nothrow) xtcore::asset::material::Principled();
    else if (!type.compare(XTPROTO_LTRL_ROUGH_DIELECTRIC)) data = new (std::nothrow) xtcore::asset::material::RoughDielectric();
    else if (!type.compare(XTPROTO_LTRL_THIN_DIELECTRIC)) data = new (std::nothrow) xtcore::asset::material::ThinDielectric();
    else if (!type.compare(XTPROTO_LTRL_SUBSURFACE)) data = new (std::nothrow) xtcore::asset::material::Subsurface();
    else if (!type.compare(XTPROTO_LTRL_SHEEN)) data = new (std::nothrow) xtcore::asset::material::Sheen();
    else if (!type.compare(XTPROTO_LTRL_THIN_TRANSLUCENT)) data = new (std::nothrow) xtcore::asset::material::ThinTranslucent();
    else if (!type.compare(XTPROTO_LTRL_BOUNDARY  )) data = new (std::nothrow) xtcore::asset::material::Boundary();
	else {
		Log::handle().post_warning("Unsupported material %s. Skipping..", p->get_name());
		delete data;
		return 0;
	}

    if (data) {
        ncf::NCF *gprops = p->get_group_by_name(XTPROTO_PROPERTIES);
        ncf::NCF *gsamplers = gprops ? gprops->get_group_by_name(XTPROTO_SAMPLERS) : 0;
        ncf::NCF *gscalars  = gprops ? gprops->get_group_by_name(XTPROTO_SCALARS) : 0;

        if (gsamplers) {
            for (size_t i = 0; i < gsamplers->count_groups(); ++i) {
                ncf::NCF *entry = gsamplers->get_group_by_index(i);

                xtcore::sampler::ISampler *sampler = deserialize_sampler_node(source, entry);
                data->add_sampler(entry->get_name(), sampler);
            }
        }

        if (gscalars) {
            for (size_t i = 0; i < gscalars->count_properties(); ++i) {
                std::string     name  = deserialize_cstr(gscalars->get_property_name_by_index(i));
                nmath::scalar_t value = deserialize_numf(gscalars->get_property_by_index(i));
                data->add_scalar(name.c_str(), value);
            }
        }
    }

	return data;
}

xtcore::sampler::Texture2D *deserialize_texture(const char *source, const ncf::NCF *p)
{
	if (!p)	return 0;

	xtcore::sampler::Texture2D *data = new (std::nothrow) xtcore::sampler::Texture2D;

	std::string script_base, script_filename, fsource = source;
	ncf::util::path_comp(fsource, script_base, script_filename);

    std::string fname  = asset_fetcher::resolve(
        deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_SOURCE)));
	std::string filter = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_FILTERING));
	std::string path = (fname.empty() || path_is_absolute(fname) || asset_fetcher::is_url(fname)) ? fname : script_base + fname;

	Log::handle().post_message("Loading texture: %s", path.c_str());
	if (data->load(path.c_str())) {
	    Log::handle().post_error("Failed to load texture: %s", path.c_str());
        delete data;
        return 0;
	}

  	if (      filter.empty()
      	  || !filter.compare(XTPROTO_LTRL_NEAREST )) { data->set_filtering(xtcore::sampler::FILTERING_NEAREST);  }
	else if (!filter.compare(XTPROTO_LTRL_BILINEAR)) { data->set_filtering(xtcore::sampler::FILTERING_BILINEAR); }
	else {
		Log::handle().post_warning("Invalid filtering method: %s", filter.c_str());
	}

	bool flip_x = deserialize_bool(p->get_property_by_name(XTPROTO_FLIP_X));
	bool flip_y = deserialize_bool(p->get_property_by_name(XTPROTO_FLIP_Y));
    if (flip_x) data->flip_horizontal();
    if (flip_y) data->flip_vertical();

    return data;
}

xtcore::asset::Object *deserialize_object(const ncf::NCF *p)
{
	if (!p)	return 0;

    xtcore::asset::Object *data = new (std::nothrow) xtcore::asset::Object;

    const std::string surface_name = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_OBJ_GEO));
    const std::string material_name = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_OBJ_MAT));

   	data->surface  = xtcore::pool::str::add(surface_name.c_str());
   	data->material = xtcore::pool::str::add(material_name.c_str());

   	return data;
}

int create_camera(Scene *scene, ncf::NCF *p)
{
    if (!scene) return -1;

    const char * name = p->get_name();
    HASH_UINT64 id = xtcore::pool::str::add(name);
    xtcore::asset::ICamera *data = deserialize_camera(p);
    if (!data) return 1;
    scene->destroy_camera(id);
    scene->m_cameras[id] = data;
    return 0;
}

int create_material(Scene *scene, ncf::NCF *p)
{
    if (!scene) return -1;

    const char * name = p->get_name();
    HASH_UINT64 id = xtcore::pool::str::add(name);
    xtcore::asset::IMaterial *data = deserialize_material(scene->m_source.c_str(), p);
    if (!data) return 1;
    scene->destroy_material(id);
    scene->m_materials[id] = data;
    return 0;
}

int create_geometry(Scene *scene, ncf::NCF *p)
{
    if (!scene) return -1;

    const char * name = p->get_name();
    HASH_UINT64 id = xtcore::pool::str::add(name);
    xtcore::asset::ISurface *data = deserialize_geometry(scene->m_source.c_str(), p);
    if (!data) return 1;
    scene->destroy_surface(id);
    scene->m_surface[id] = data;
    scene->mark_spatial_index_dirty();
    return 0;
}

int create_medium(std::map<HASH_UINT64, xtcore::asset::medium::IMedium*> &media, ncf::NCF *p)
{
    if (!p) return -1;

    const char *name = p->get_name();
    HASH_UINT64 id = xtcore::pool::str::add(name);

    const std::string mtype = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_TYPE));
    if (mtype != XTPROTO_LTRL_HOMOGENEOUS && mtype != XTPROTO_LTRL_HETEROGENEOUS_NOISE) {
        Log::handle().post_error("Unsupported medium type '%s' on medium %s", mtype.c_str(), name);
        return 1;
    }

    const nimg::ColorRGBf sigma_a = deserialize_col3(p, XTPROTO_PROP_SIGMA_A, nimg::ColorRGBf(0.0f, 0.0f, 0.0f));
    const nimg::ColorRGBf sigma_s = deserialize_col3(p, XTPROTO_PROP_SIGMA_S, nimg::ColorRGBf(0.0f, 0.0f, 0.0f));
    const nimg::ColorRGBf emission = deserialize_col3(p, XTPROTO_PROP_EMISSION, nimg::ColorRGBf(0.0f, 0.0f, 0.0f));
    const nmath::scalar_t g = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_G), 0.0f);

    const bool bad_sigma =
           sigma_a.r() < 0.0f || sigma_a.g() < 0.0f || sigma_a.b() < 0.0f
        || sigma_s.r() < 0.0f || sigma_s.g() < 0.0f || sigma_s.b() < 0.0f;
    const bool bad_emission = emission.r() < 0.0f || emission.g() < 0.0f || emission.b() < 0.0f;
    if (bad_sigma || bad_emission || g < -1.0f || g > 1.0f) {
        Log::handle().post_error("Invalid medium parameters on medium %s", name);
        return 1;
    }

    auto it = media.find(id);
    if (it != media.end()) {
        delete it->second;
        it->second = 0;
        media.erase(it);
    }

    xtcore::asset::medium::IMedium *medium = 0;
    if (mtype == XTPROTO_LTRL_HOMOGENEOUS) {
        medium = new (std::nothrow) xtcore::asset::medium::Homogeneous(sigma_a, sigma_s, emission, g);
    } else {
        const nmath::scalar_t density = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_DENSITY), 1.0f);
        const nmath::scalar_t noise_scale = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_NOISE_SCALE), 1.0f);
        const nmath::scalar_t noise_min = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_NOISE_MIN), 0.25f);
        const nmath::scalar_t noise_max = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_NOISE_MAX), 1.0f);
        const int octaves = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_OCTAVES), 4);
        const nmath::scalar_t lacunarity = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_LACUNARITY), 2.0f);
        const nmath::scalar_t gain = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_GAIN), 0.5f);
        const int seed = deserialize_numi(p->get_property_by_name(XTPROTO_PROP_SEED), 1337);

        const bool bad_noise =
               density < 0.0f
            || noise_scale <= 0.0f
            || noise_min < 0.0f
            || noise_max < 0.0f
            || octaves <= 0
            || lacunarity < 1.0f
            || gain <= 0.0f
            || gain >= 1.0f;
        if (bad_noise) {
            Log::handle().post_error("Invalid heterogeneous noise parameters on medium %s", name);
            return 1;
        }

        medium = new (std::nothrow) xtcore::asset::medium::HeterogeneousNoise(
              sigma_a
            , sigma_s
            , emission
            , g
            , density
            , noise_scale
            , noise_min
            , noise_max
            , octaves
            , lacunarity
            , gain
            , seed
        );
    }

    media[id] = medium;
    if (!media[id]) {
        Log::handle().post_error("Failed to allocate medium %s", name);
        return 1;
    }

    return 0;
}

xtcore::sampler::ISampler *create_sampler(const char *base, const char *texture, float value[3], bool white_fallback_on_missing)
{
     xtcore::sampler::ISampler *sampler = 0;
     {
        bool loaded_texture = false;
        if (texture && strlen(texture) > 0) {
            xtcore::sampler::Texture2D *tex = new (std::nothrow) xtcore::sampler::Texture2D();
            std::string normalized_path = base;
            normalized_path.append(texture);
            std::replace(normalized_path.begin(), normalized_path.end(), '\\', '/');
            if (tex && tex->load(normalized_path.c_str()) == 0) {
                tex->set_filtering(xtcore::sampler::FILTERING_BILINEAR);
                tex->flip_vertical();
                sampler = tex;
                loaded_texture = true;
            } else {
                Log::handle().post_warning("Failed to load texture %s, using solid color fallback", normalized_path.c_str());
                delete tex;
            }
        }
        if (!loaded_texture) {
            float r = value[0];
            float g = value[1];
            float b = value[2];
            if (white_fallback_on_missing && r <= 0.0f && g <= 0.0f && b <= 0.0f) {
                r = 1.0f;
                g = 1.0f;
                b = 1.0f;
            }
            sampler = new (std::nothrow) xtcore::sampler::SolidColor();
            nimg::ColorRGBf col(r, g, b);
            ((xtcore::sampler::SolidColor*)sampler)->set(col);
        }
     }
     return sampler;
}

int create_object(Scene *scene,
                  const char *filepath,
                  const char *prefix,
                  const xtcore::asset::medium::IMedium *medium_proto)
{
    if (!scene) return -1;

    Log::handle().post_message("Object: External loader [%s, %s]",filepath, prefix);
    auto t_import_0 = std::chrono::steady_clock::now();
    xtcore::imported_asset_t imported;
    std::string import_error;
    if (!xtcore::import_asset_file(filepath, imported, import_error)) {
   		Log::handle().post_warning("Failed to load asset from %s (%s)", filepath, import_error.c_str());
        return 1;
    }
    auto t_import_1 = std::chrono::steady_clock::now();
    const double import_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_import_1 - t_import_0).count();
    Log::handle().post_message("External asset import done: %s (%zu shapes, %.0f ms)",
                               filepath, imported.shapes.size(), import_ms);
    if (xtcore::create_objects_from_imported_asset(scene, imported, prefix, medium_proto, import_error) != 0) {
        Log::handle().post_error("Failed to build imported objects for %s (%s)", filepath, import_error.c_str());
        return 1;
    }
    Log::handle().post_message("External object build done: %s (%zu objects)", filepath, imported.shapes.size());
    return 0;
}

int create_object(Scene *scene, const char *filepath, const char *prefix)
{
    return create_object(scene, filepath, prefix, 0);
}

int create_object(Scene *scene,
                  ncf::NCF *p,
                  const std::map<HASH_UINT64, xtcore::asset::medium::IMedium*> &media_defs)
{
    if (!scene) return -1;

    const char * name = p->get_name();
    HASH_UINT64 id = xtcore::pool::str::add(name);
    xtcore::asset::Object *data = deserialize_object(p);
    if (!data) return 1;
    scene->destroy_object(id);
    scene->m_objects[id] = data;

    scene->clear_object_medium(id);
    if (p->query_group(XTPROTO_PROP_MEDIUM)) {
        Log::handle().post_error("Object %s uses inline medium block; only top-level medium assets are supported", name);
        return 1;
    }

    if (p->query_property(XTPROTO_PROP_MEDIUM)) {
        const std::string medium_name = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_MEDIUM));
        const HASH_UINT64 medium_id = xtcore::pool::str::add(medium_name.c_str());
        auto it = media_defs.find(medium_id);
        if (it == media_defs.end() || !it->second) {
            Log::handle().post_error("Object %s references unknown medium '%s'", name, medium_name.c_str());
            return 1;
        }
        scene->set_object_medium(id, it->second->clone());
        if (!scene->has_object_medium(id)) {
            Log::handle().post_error("Failed to allocate medium for object %s", name);
            return 1;
        }
    }

    scene->mark_spatial_index_dirty();
    return 0;
}

int create_object(Scene *scene, ncf::NCF *p)
{
    static const std::map<HASH_UINT64, xtcore::asset::medium::IMedium*> k_empty_media_defs;
    return create_object(scene, p, k_empty_media_defs);
}

int load(Scene *scene, const char *filename, const std::list<std::string> *modifiers, const char *variant)
{
	Log::handle().post_message("Loading script [%s]..", filename);
    auto t_load_0 = std::chrono::steady_clock::now();

    if(!filename) return 1;

    ncf::NCF root;
    root.set_source(filename);

    ncf::error_t error;

	if (root.parse(&error))  {
        root.purge();
		Log::handle().post_error("Failed to parse the scene. Line %i: %s", error.line, error.message);
		return 2;
	}

    if (!apply_variant_overlay(&root, variant)) {
        root.purge();
        return 2;
    }

    // Mods are of the form: group.group.property:value
    if (modifiers) {
    	std::list<std::string>::const_iterator mod_it = modifiers->begin();
    	std::list<std::string>::const_iterator mod_et = modifiers->end();

        for (; mod_it != mod_et; ++mod_it) {
            std::string mod = (*mod_it);
    		if (mod.find_last_of(':') == std::string::npos) {
    			Log::handle().post_warning("Invalid rule: %s", mod.c_str());
    			continue;
    		}

	    	ncf::NCF *node = &root;
    		std::string nleft, nright;
            while((mod.find_first_of('.') != std::string::npos)
               && (mod.find_first_of(':') > mod.find_first_of('.'))) {
    			ncf::util::split(mod, nleft, nright, '.');
	    		mod = nright;
		    	node = node->get_group_by_name(nleft.c_str());
    		}

		    ncf::util::split(mod, nleft, nright, ':');
    		node->set_property(nleft.c_str(), nright.c_str());
        }
	}

    Log::handle().post_message("Building the scene..");

    scene->m_source      = deserialize_cstr(filename);
    scene->m_name        = deserialize_cstr(root.get_property_by_name(XTPROTO_PROP_TITLE));
    scene->m_description = deserialize_cstr(root.get_property_by_name(XTPROTO_PROP_DESCR));
    scene->m_version     = deserialize_cstr(root.get_property_by_name(XTPROTO_PROP_VERSN));
    scene->m_default_camera = deserialize_cstr(root.get_property_by_name(XTPROTO_PROP_DEFAULT_CAMERA));
	scene->m_ambient     = deserialize_col3(&root, XTPROTO_PROP_IAMBN)
			             * deserialize_numf(root.get_property_by_name(XTPROTO_PROP_KAMBN), 1.);


    ncf::NCF *env_node = root.get_group_by_name(XTPROTO_NODE_ENVIRONMENT);
    ncf::NCF *env_data = env_node->get_group_by_name(XTPROTO_CONFIG);

    std::string environment = deserialize_cstr(env_node->get_property_by_name(XTPROTO_PROP_TYPE));
         if (!environment.compare(XTPROTO_CUBEMAP     )) scene->m_environment = deserialize_cubemap     (scene->m_source.c_str(), env_data);
    else if (!environment.compare(XTPROTO_ERP         )) scene->m_environment = deserialize_erp         (scene->m_source.c_str(), env_data);
    else if (!environment.compare(XTPROTO_GRADIENT    )) scene->m_environment = deserialize_gradient    (env_data);
    else if (!environment.compare(XTPROTO_COLOR       )) scene->m_environment = deserialize_rgba        (env_data);
    else if (!environment.compare(XTPROTO_RAYLEIGH_SKY)) scene->m_environment = deserialize_rayleigh_sky(env_data);

    std::map<HASH_UINT64, xtcore::asset::medium::IMedium*> medium_defs;
    ncf::NCF *medium_root = root.get_group_by_name(XTPROTO_NODE_MEDIUM);
    if (medium_root) {
        const size_t medium_count = medium_root->count_groups();
        for (size_t i = 0; i < medium_count; ++i) {
            ncf::NCF *mnode = medium_root->get_group_by_index(i);
            Log::handle().post_message("Creating medium / %s..", mnode->get_name());
            if (create_medium(medium_defs, mnode)) {
                Log::handle().post_error("Failed to load medium: %s", mnode->get_name());
                for (auto it = medium_defs.begin(); it != medium_defs.end(); ++it) delete it->second;
                medium_defs.clear();
                scene->release();
                return 1;
            }
        }
    }

	std::list<std::string> sections;
	sections.push_back(XTPROTO_NODE_CAMERA);
	sections.push_back(XTPROTO_NODE_GEOMETRY);
	sections.push_back(XTPROTO_NODE_MATERIAL);
	sections.push_back(XTPROTO_NODE_TEXTURE);
	sections.push_back(XTPROTO_NODE_OBJECT);

	std::list<std::string>::iterator it = sections.begin();
	std::list<std::string>::iterator et = sections.end();

	for (; it != et; ++it) {
    	size_t count = root.get_group_by_name((*it).c_str())->count_groups();

		if (count) {
			for (size_t i = 0; i < count; ++i) {
				ncf::NCF *lnode = root.get_group_by_name((*it).c_str())->get_group_by_index(i);
                Log::handle().post_message("Creating %s / %s..", (*it).c_str(), lnode->get_name());

                int res = 0;

				     if (!(*it).compare(XTPROTO_NODE_CAMERA  )) res = create_camera   (scene, lnode);
				else if (!(*it).compare(XTPROTO_NODE_MATERIAL)) res = create_material (scene, lnode);
				else if (!(*it).compare(XTPROTO_NODE_GEOMETRY)) res = create_geometry (scene, lnode);
			    else if (!(*it).compare(XTPROTO_NODE_OBJECT  )) {

                    // Check if the object is loaded from an external source
                    bool external = lnode->query_property(XTPROTO_PROP_SOURCE);

                    /* An object can be either:
                    **  1. Imported from an external file.
                    **     If multiple sub-objects are defined within the external source, the object
                    **     name will be used as a prefix and multiple individual objects will be
                    **     created. A separate prefix property will be used to create material and
                    **     geometry names.
                    **     Only OBJ format is supported for external data atm
                    **  2. Created from components defined in the scn structure. (backwards compatible)
                    ** If the source tag is defined then the parser will attempt to do (1).
                    ** Otherwise, a geometry and a material node will be used to create the object.
                    */
                    if (external) {
	                    std::string base, file, fsource = scene->m_source;
                		ncf::util::path_comp(fsource, base, file);

                        std::string flpath = deserialize_cstr(lnode->get_property_by_name(XTPROTO_PROP_SOURCE));
                        std::string prefix = deserialize_cstr(lnode->get_property_by_name(XTPROTO_PROP_PREFIX));
                        const xtcore::asset::medium::IMedium *medium_proto = 0;
                        if (lnode->query_group(XTPROTO_PROP_MEDIUM)) {
                            Log::handle().post_error("Object %s uses inline medium block; only top-level medium assets are supported", lnode->get_name());
                            for (auto it = medium_defs.begin(); it != medium_defs.end(); ++it) delete it->second;
                            medium_defs.clear();
                            scene->release();
                            return 1;
                        }
                        if (lnode->query_property(XTPROTO_PROP_MEDIUM)) {
                            const std::string medium_name = deserialize_cstr(lnode->get_property_by_name(XTPROTO_PROP_MEDIUM));
                            const HASH_UINT64 mid = xtcore::pool::str::add(medium_name.c_str());
                            auto mit = medium_defs.find(mid);
                            if (mit == medium_defs.end() || !mit->second) {
                                Log::handle().post_error("Object %s references unknown medium '%s'", lnode->get_name(), medium_name.c_str());
                                for (auto it = medium_defs.begin(); it != medium_defs.end(); ++it) delete it->second;
                                medium_defs.clear();
                                scene->release();
                                return 1;
                            }
                            medium_proto = mit->second;
                        }

                        flpath = asset_fetcher::resolve(flpath);
                        if (flpath.empty() || path_is_absolute(flpath) || asset_fetcher::is_url(flpath)) base = flpath;
                        else base.append(flpath);
                        res = create_object(scene, base.c_str(), prefix.c_str(), medium_proto);
                    }
                    else res = create_object(scene, lnode, medium_defs);
                }

                // Check for parsing errors
                if (res) {
                    Log::handle().post_error("Failed to load: %s", lnode->get_name());
                    for (auto mit = medium_defs.begin(); mit != medium_defs.end(); ++mit) delete mit->second;
                    medium_defs.clear();
                    scene->release();
                    return 1;
                }
			}
		}
	}

    ncf::NCF *random_node = root.get_group_by_name("random");
    const size_t random_count = random_node->count_groups();
    for (size_t i = 0; i < random_count; ++i) {
        ncf::NCF *entry = random_node->get_group_by_index(i);
        Log::handle().post_message("Creating random / %s..", entry->get_name());
        if (create_random(scene, entry)) {
            Log::handle().post_error("Failed to load random generator: %s", entry->get_name());
            scene->release();
            return 1;
        }
    }

    scene->rebuild_spatial_index();
    for (auto it = medium_defs.begin(); it != medium_defs.end(); ++it) delete it->second;
    medium_defs.clear();
	Log::handle().post_message("Scene loaded.");
    auto t_load_1 = std::chrono::steady_clock::now();
    const double load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_load_1 - t_load_0).count();
    Log::handle().post_message("Scene load completed in %.0f ms", load_ms);
	return 0;
}

unsigned long long load_async_start(const char *filename,
                                    const std::list<std::string> *modifiers,
                                    const char *variant)
{
    if (!filename || !*filename) return 0ULL;

    std::shared_ptr<async_load_job_t> job(new (std::nothrow) async_load_job_t());
    if (!job) return 0ULL;

    job->id = g_async_next_id.fetch_add(1ULL);
    job->filename = filename;
    job->has_modifiers = (modifiers != 0);
    if (modifiers) job->modifiers = *modifiers;
    job->has_variant = (variant && *variant);
    if (job->has_variant) job->variant = variant;

    {
        std::lock_guard<std::mutex> lock(g_async_jobs_mut);
        g_async_jobs[job->id] = job;
    }

    std::thread worker([job]() {
        {
            std::lock_guard<std::mutex> lock(g_async_jobs_mut);
            job->state = ASYNC_LOAD_RUNNING;
            job->error.clear();
        }

        std::shared_ptr<Scene> loaded_scene(new (std::nothrow) Scene());
        if (!loaded_scene) {
            std::lock_guard<std::mutex> lock(g_async_jobs_mut);
            job->state = ASYNC_LOAD_ERROR;
            job->error = "Failed to allocate scene";
            return;
        }

        int result = 1;
        {
            std::lock_guard<std::mutex> lock(g_async_load_exec_mut);
            const std::list<std::string> *mods = job->has_modifiers ? &job->modifiers : 0;
            const char *variant_name = job->has_variant ? job->variant.c_str() : 0;
            result = load(loaded_scene.get(), job->filename.c_str(), mods, variant_name);
        }

        std::lock_guard<std::mutex> lock(g_async_jobs_mut);
        if (result == 0) {
            job->scene = loaded_scene;
            job->state = ASYNC_LOAD_DONE;
            job->error.clear();
        } else {
            job->state = ASYNC_LOAD_ERROR;
            job->error = "Scene load failed";
        }
    });
    worker.detach();

    return job->id;
}

bool load_async_snapshot(unsigned long long id, async_load_snapshot_t *out)
{
    if (!id || !out) return false;

    std::lock_guard<std::mutex> lock(g_async_jobs_mut);
    std::map<unsigned long long, std::shared_ptr<async_load_job_t> >::const_iterator it = g_async_jobs.find(id);
    if (it == g_async_jobs.end() || !it->second) return false;

    out->id = it->second->id;
    out->state = it->second->state;
    out->state_name = async_load_state_name(it->second->state);
    out->filename = it->second->filename;
    out->error = it->second->error;
    return true;
}

std::shared_ptr<Scene> load_async_take_scene(unsigned long long id)
{
    if (!id) return std::shared_ptr<Scene>();

    std::lock_guard<std::mutex> lock(g_async_jobs_mut);
    std::map<unsigned long long, std::shared_ptr<async_load_job_t> >::iterator it = g_async_jobs.find(id);
    if (it == g_async_jobs.end() || !it->second) return std::shared_ptr<Scene>();
    if (it->second->state != ASYNC_LOAD_DONE) return std::shared_ptr<Scene>();

    std::shared_ptr<Scene> result = it->second->scene;
    it->second->scene.reset();
    return result;
}

void load_async_discard(unsigned long long id)
{
    if (!id) return;

    std::lock_guard<std::mutex> lock(g_async_jobs_mut);
    std::map<unsigned long long, std::shared_ptr<async_load_job_t> >::iterator it = g_async_jobs.find(id);
    if (it != g_async_jobs.end()) g_async_jobs.erase(it);
}

size_t load_async_gc_done(size_t keep_latest)
{
    std::lock_guard<std::mutex> lock(g_async_jobs_mut);

    std::vector<unsigned long long> completed_ids;
    completed_ids.reserve(g_async_jobs.size());

    for (std::map<unsigned long long, std::shared_ptr<async_load_job_t> >::const_iterator it = g_async_jobs.begin();
         it != g_async_jobs.end(); ++it) {
        if (!it->second) continue;
        if (it->second->state == ASYNC_LOAD_DONE || it->second->state == ASYNC_LOAD_ERROR) {
            completed_ids.push_back(it->first);
        }
    }

    if (completed_ids.size() <= keep_latest) return 0;

    const size_t remove_count = completed_ids.size() - keep_latest;
    for (size_t i = 0; i < remove_count; ++i) {
        std::map<unsigned long long, std::shared_ptr<async_load_job_t> >::iterator it = g_async_jobs.find(completed_ids[i]);
        if (it != g_async_jobs.end()) g_async_jobs.erase(it);
    }
    return remove_count;
}

        } /* namespace scn */
    } /* namespace io */
} /* namespace xtcore */
