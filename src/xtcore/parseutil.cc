#include <cstdio>
#include <algorithm>
#include <cctype>
#include <vector>
#include <chrono>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <fstream>

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
#include "mesh.h"
#include "proto.h"
#include "obj.h"

#include "strpool.h"
#include "log.h"
#include "camera.h"
#include "material.h"
#include "sampler.h"
#include "sampler/sampler_erp.h"
#include "sampler/sampler_graphpaper.h"
#include "sampler/sampler_checker.h"
#include "sampler/sampler_weave.h"
#include "sampler/sampler_fbm_marble.h"
#include "sampler/sampler_voronoi_normal.h"
#include "macro.h"

#include "extrude.h"

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

namespace {

struct async_load_job_t {
    unsigned long long id;
    async_load_state_t state;
    std::string filename;
    std::string error;
    std::shared_ptr<Scene> scene;
    std::list<std::string> modifiers;
    bool has_modifiers;

    async_load_job_t()
        : id(0)
        , state(ASYNC_LOAD_QUEUED)
        , filename()
        , error()
        , scene()
        , modifiers()
        , has_modifiers(false)
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

static bool path_exists(const std::string &path)
{
    std::ifstream in(path.c_str(), std::ios::binary);
    return in.good();
}

static std::string resolve_obj_case_path(const std::string &path)
{
    if (path.empty()) return path;
    if (path_exists(path)) return path;

    const size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return path;

    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    if (ext != "obj") return path;

    static const char *variants[] = { "obj", "OBJ", "Obj" };
    for (size_t i = 0; i < (sizeof(variants) / sizeof(variants[0])); ++i) {
        std::string candidate = path.substr(0, dot + 1);
        candidate.append(variants[i]);
        if (path_exists(candidate)) return candidate;
    }
    return path;
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

xtcore::sampler::Cubemap *deserialize_cubemap(const char *source, const ncf::NCF *p)
{
    if (!p) return 0;

    xtcore::sampler::Cubemap *data = new xtcore::sampler::Cubemap;

    if (data) {
        std::string posx = deserialize_cstr(p->get_property_by_name(XTPROTO_LTRL_POSX));
        std::string posy = deserialize_cstr(p->get_property_by_name(XTPROTO_LTRL_POSY));
        std::string posz = deserialize_cstr(p->get_property_by_name(XTPROTO_LTRL_POSZ));
        std::string negx = deserialize_cstr(p->get_property_by_name(XTPROTO_LTRL_NEGX));
        std::string negy = deserialize_cstr(p->get_property_by_name(XTPROTO_LTRL_NEGY));
        std::string negz = deserialize_cstr(p->get_property_by_name(XTPROTO_LTRL_NEGZ));

        std::string base, file, fsource = source;
    	ncf::util::path_comp(fsource, base, file);

        data->load((base + posx).c_str(), xtcore::sampler::CUBEMAP_FACE_RIGHT);
        data->load((base + posy).c_str(), xtcore::sampler::CUBEMAP_FACE_TOP);
        data->load((base + posz).c_str(), xtcore::sampler::CUBEMAP_FACE_FRONT);
        data->load((base + negx).c_str(), xtcore::sampler::CUBEMAP_FACE_LEFT);
        data->load((base + negy).c_str(), xtcore::sampler::CUBEMAP_FACE_BOTTOM);
        data->load((base + negz).c_str(), xtcore::sampler::CUBEMAP_FACE_BACK);
    }
    return data;
}

xtcore::sampler::ERP *deserialize_erp(const char *source, const ncf::NCF *p)
{
    if (!p) return 0;

    xtcore::sampler::ERP *data = new xtcore::sampler::ERP;

    if (data) {
        std::string src = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_SOURCE));

        std::string base, file, fsource = source;
        ncf::util::path_comp(fsource, base, file);

        data->load((base + src).c_str());
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

            nmesh::generator::ring(&obj, (size_t)i, radius, height, thickness, (size_t)hres);
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
        else if (!token.compare(XTPROTO_LTRL_SIERPINSKI_TETRAHEDRON)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 2);
            if (i < 1) i = 1;
            nmesh::generator::sierpinski_tetrahedron(&obj, (size_t)i);
        }
        else if (!token.compare(XTPROTO_LTRL_MOBIUS_STRIP)) {
            int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 64);
            if (i < 24) i = 24;
            nmesh::generator::mobius_strip(&obj, (size_t)i);
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

        else Log::handle().post_message("Invalid mesh generator: %s (%s)", token.c_str(), f.c_str());
    }
    // External sources
    else {
        // Open source file from relative path
	    std::string base, file, fsource = source;
		ncf::util::path_comp(fsource, base, file);
    	base.append(f);

        std::string import_path = resolve_obj_case_path(base);
        if (import_path != base) {
            Log::handle().post_warning("OBJ path case fallback: %s -> %s", base.c_str(), import_path.c_str());
        }
	    Log::handle().post_message("Loading data from %s", import_path.c_str());
        auto t_import_0 = std::chrono::steady_clock::now();

	    if (nmesh::io::import::obj(import_path.c_str(), obj))
    	{
    		Log::handle().post_warning("Failed to load mesh from %s", f.c_str());
	    	delete data;
            return 0;
		}
        auto t_import_1 = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_import_1 - t_import_0).count();
        Log::handle().post_message("OBJ import done: %s (%.0f ms)", base.c_str(), ms);
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
	else {
		Log::handle().post_warning("Unsupported material %s. Skipping..", p->get_name());
		delete data;
		return 0;
	}

    if (data) {
        ncf::NCF *gsamplers = p->get_group_by_name(XTPROTO_PROPERTIES)->get_group_by_name(XTPROTO_SAMPLERS);
        ncf::NCF *gscalars  = p->get_group_by_name(XTPROTO_PROPERTIES)->get_group_by_name(XTPROTO_SCALARS);

        for (size_t i = 0; i < gsamplers->count_groups(); ++i) {
            ncf::NCF *entry = gsamplers->get_group_by_index(i);

            xtcore::sampler::ISampler *sampler = 0;
            std::string type = deserialize_cstr(entry->get_property_by_name(XTPROTO_PROP_TYPE));

                 if (!type.compare(XTPROTO_TEXTURE )) sampler = deserialize_texture (source, entry);
            else if (!type.compare(XTPROTO_CUBEMAP )) sampler = deserialize_cubemap (source, entry);
            else if (!type.compare(XTPROTO_ERP     )) sampler = deserialize_erp     (source, entry);
            else if (!type.compare(XTPROTO_GRADIENT)) sampler = deserialize_gradient(entry);
            else if (!type.compare(XTPROTO_GRAPHPAPER)) sampler = deserialize_graphpaper(entry);
            else if (!type.compare(XTPROTO_CHECKER)) sampler = deserialize_checker(entry);
            else if (!type.compare(XTPROTO_WEAVE)) sampler = deserialize_weave(entry);
            else if (!type.compare(XTPROTO_FBM_MARBLE)) sampler = deserialize_fbm_marble(entry);
            else if (!type.compare(XTPROTO_VORONOI_NORMAL)) sampler = deserialize_voronoi_normal(entry);
            else if (!type.compare(XTPROTO_COLOR   )) sampler = deserialize_rgba    (entry);

            data->add_sampler(entry->get_name(), sampler);
        }

        for (size_t i = 0; i < gscalars->count_properties(); ++i) {
            std::string     name  = deserialize_cstr(gscalars->get_property_name_by_index(i));
            nmath::scalar_t value = deserialize_numf(gscalars->get_property_by_index(i));
            data->add_scalar(name.c_str(), value);
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

    std::string fname  = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_SOURCE));
	std::string filter = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_FILTERING));
	std::string path = script_base + fname;

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

xtcore::sampler::ISampler *create_sampler(const char *base, const char *texture, float value[3], bool white_fallback_on_missing = false)
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

int create_object(Scene *scene, const char *filepath, const char *prefix)
{
    if (!scene) return -1;

    Log::handle().post_message("Object: External loader [%s, %s]",filepath, prefix);

    nmesh::object_t obj;

    std::string base, filename, fsource = filepath;

	ncf::util::path_comp(fsource, base, filename);
    std::string import_path = resolve_obj_case_path(fsource);
    if (import_path != fsource) {
        Log::handle().post_warning("OBJ path case fallback: %s -> %s", fsource.c_str(), import_path.c_str());
    }
    auto t_import_0 = std::chrono::steady_clock::now();
    if (nmesh::io::import::obj(import_path.c_str(), obj, base.c_str()))	{
   		Log::handle().post_warning("Failed to load mesh from %s", fsource.c_str());
        return 1;
    }
    auto t_import_1 = std::chrono::steady_clock::now();
    const double import_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_import_1 - t_import_0).count();
    Log::handle().post_message("External OBJ import done: %s (%zu shapes, %.0f ms)", fsource.c_str(), obj.shapes.size(), import_ms);

    Log::handle().post_debug("Materials: %i", obj.materials.size());
    Log::handle().post_debug("   Shapes: %i", obj.shapes.size());

    std::vector<HASH_UINT64> matids;
    for (size_t m = 0; m < obj.materials.size(); ++m) {
        std::string name = prefix;
        name.append(obj.materials[m].name);
        HASH_UINT64 id = xtcore::pool::str::add(name.c_str());

        // Check if the id is already in there
        bool loaded = false;
        for (size_t i = 0; i < matids.size(); ++i) {
            if (matids[i] == id) {
                Log::handle().post_debug("Material [%s] already exists", name.c_str());
                // Fix indices
                for (nmesh::shape_t shape: obj.shapes) {
                    if (shape.mesh.materials[0] == (int)m) shape.mesh.materials[0] = i;
                }
                loaded = true;
                break;
            }
        }

        if (!loaded) {
            Log::handle().post_debug("creating material %s", name.c_str());
            matids.push_back(id);

            // Determine material type
            xtcore::asset::IMaterial *mat = 0;

            bool has_emissive_col = ((obj.materials[m].emission[0] > 0.f) || (obj.materials[m].emission[1] > 0.f) || (obj.materials[m].emission[2] > 0.f));
            if (obj.materials[m].texture_emissive.length() != 0 || has_emissive_col)
            {
                mat = new (std::nothrow) xtcore::asset::material::Emissive();
                xtcore::sampler::ISampler *s = create_sampler(base.c_str(), obj.materials[m].texture_emissive.c_str(), obj.materials[m].emission);
                mat->add_sampler(MAT_SAMPLER_EMISSIVE, s);
            }
            else
            {
                mat = new (std::nothrow) xtcore::asset::material::Lambert();
                xtcore::sampler::ISampler *kd = create_sampler(base.c_str(), obj.materials[m].texture_diffuse.c_str() , obj.materials[m].diffuse, true);
                mat->add_sampler(MAT_SAMPLER_DIFFUSE , kd);

                std::string normal_tex = obj.materials[m].texture_normal;
                if (normal_tex.empty()) normal_tex = obj.materials[m].texture_bump;
                if (!normal_tex.empty()) {
                    float normal_default[3] = { 0.5f, 0.5f, 1.0f };
                    xtcore::sampler::ISampler *kn = create_sampler(base.c_str(), normal_tex.c_str(), normal_default);
                    mat->add_sampler(MAT_SAMPLER_NORMAL, kn);
                }
            }

            scene->m_materials[id] = mat;
        }
    }

    HASH_UINT64 fallback_mat_id = HASH_ID_INVALID;
    int shape_count = 0;
    double octree_total_ms = 0.0;
    for (nmesh::shape_t shape : obj.shapes) {
        ++shape_count;
        std::string name = std::to_string(shape_count);
        name.append(prefix);
        name.append(shape.name);

        HASH_UINT64 id = xtcore::pool::str::add(name.c_str());

        Log::handle().post_debug("creating geometry %s", name.c_str());

        // Create Geometry
        xtcore::asset::ISurface *surf = new (std::nothrow) xtcore::surface::Mesh();
        auto t_oct_0 = std::chrono::steady_clock::now();
        ((xtcore::surface::Mesh*)surf)->build_octree(shape, obj.attributes);
        auto t_oct_1 = std::chrono::steady_clock::now();
        const double oct_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_oct_1 - t_oct_0).count();
        octree_total_ms += oct_ms;
        scene->m_surface[id] = surf;

        // Create Object
        xtcore::asset::Object *obj = new (std::nothrow) xtcore::asset::Object();
        obj->surface  = id;
        int material_index = -1;
        if (!shape.mesh.materials.empty()) material_index = shape.mesh.materials[0];

        if (material_index < 0 || (size_t)material_index >= matids.size()) {
            if (fallback_mat_id == HASH_ID_INVALID) {
                std::string fallback_name = std::string(prefix ? prefix : "") + "__fallback_material";
                fallback_mat_id = xtcore::pool::str::add(fallback_name.c_str());
                if (scene->m_materials.find(fallback_mat_id) == scene->m_materials.end()) {
                    xtcore::asset::IMaterial *mat = new (std::nothrow) xtcore::asset::material::Lambert();
                    float kd_val[3] = { 1.0f, 1.0f, 1.0f };
                    mat->add_sampler(MAT_SAMPLER_DIFFUSE, create_sampler(base.c_str(), "", kd_val));
                    scene->m_materials[fallback_mat_id] = mat;
                }
            }
            Log::handle().post_warning("Shape %s has invalid material index (%d), using fallback material",
                                       shape.name.c_str(), material_index);
            obj->material = fallback_mat_id;
        } else {
            obj->material = matids[(size_t)material_index];
        }
        scene->m_objects[id] = obj;
        Log::handle().post_message("creating object %s : %s"
                                 , name.c_str()
                                 , xtcore::pool::str::get(obj->material));
    }
    Log::handle().post_message("External object build done: %s (%d objects, %.0f ms octree total)", filepath, shape_count, octree_total_ms);
    scene->mark_spatial_index_dirty();

    return 0;
}

int create_object(Scene *scene, ncf::NCF *p)
{
    if (!scene) return -1;

    const char * name = p->get_name();
    HASH_UINT64 id = xtcore::pool::str::add(name);
    xtcore::asset::Object *data = deserialize_object(p);
    if (!data) return 1;
    scene->destroy_object(id);
    scene->m_objects[id] = data;
    scene->mark_spatial_index_dirty();
    return 0;
}

int load(Scene *scene, const char *filename, const std::list<std::string> *modifiers)
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
         if (!environment.compare(XTPROTO_CUBEMAP )) scene->m_environment = deserialize_cubemap (scene->m_source.c_str(), env_data);
         if (!environment.compare(XTPROTO_ERP     )) scene->m_environment = deserialize_erp     (scene->m_source.c_str(), env_data);
    else if (!environment.compare(XTPROTO_GRADIENT)) scene->m_environment = deserialize_gradient(env_data);
    else if (!environment.compare(XTPROTO_COLOR   )) scene->m_environment = deserialize_rgba    (env_data);

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

                        base.append(flpath);
                        res = create_object(scene, base.c_str(), prefix.c_str());
                    }
                    else res = create_object(scene, lnode);
                }

                // Check for parsing errors
                if (res) {
                    Log::handle().post_error("Failed to load: %s", lnode->get_name());
                    scene->release();
                    return 1;
                }
			}
		}
	}
    scene->rebuild_spatial_index();
	Log::handle().post_message("Scene loaded.");
    auto t_load_1 = std::chrono::steady_clock::now();
    const double load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_load_1 - t_load_0).count();
    Log::handle().post_message("Scene load completed in %.0f ms", load_ms);
	return 0;
}

unsigned long long load_async_start(const char *filename, const std::list<std::string> *modifiers)
{
    if (!filename || !*filename) return 0ULL;

    std::shared_ptr<async_load_job_t> job(new (std::nothrow) async_load_job_t());
    if (!job) return 0ULL;

    job->id = g_async_next_id.fetch_add(1ULL);
    job->filename = filename;
    job->has_modifiers = (modifiers != 0);
    if (modifiers) job->modifiers = *modifiers;

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
            result = load(loaded_scene.get(), job->filename.c_str(), mods);
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
