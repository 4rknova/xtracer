#include <cstdio>
#include <algorithm>

#include <ncf/util.h>

#include <nmath/plane.h>
#include <nmath/sphere.h>
#include <nmath/triangle.h>
#include <nmesh/mesh.h>
#include <nmesh/transform.h>
#include <nmesh/obj.h>
#include <nmesh/invnormals.h>
#include <nmesh/icosahedron.h>
#include <nimg/checkerboard.h>

#include "proto.h"

#include "log.h"
#include "cam_perspective.h"
#include "cam_ods.h"
#include "cam_erp.h"
#include "cam_cubemap.h"
#include "mat_lambert.h"
#include "mat_phong.h"
#include "mat_blinnphong.h"

#include "extrude.h"

#include "parseutil.h"

namespace xtcore {
    namespace io {

bool deserialize_bool(const char *val, const bool def)
{
	if (!val) return false;
	std::string s = val;
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return ( (s == "yes") || (s == "true") ) ? true : false;
}

NMath::scalar_t deserialize_numf(const char *val, const NMath::scalar_t def)
{
	return val ? (NMath::scalar_t)ncf::util::to_double(val) : def;
}

std::string deserialize_cstr(const char *val, const char* def)
{
	return val ? val : def;
}

NMath::Vector2f deserialize_tex2(const ncf::NCF *node, const char *name, const NMath::Vector2f def)
{
	NMath::Vector2f res = def;

	if (node) {
        bool has_property = node->query_property(name);
        bool has_group    = node->query_group(name);

        if (has_group) {
            ncf::NCF *group = node->get_group_by_name(name);
            const char *u = group->get_property_by_name(XTPROTO_PROP_CRD_U);
            const char *v = group->get_property_by_name(XTPROTO_PROP_CRD_V);

    		res = NMath::Vector2f(
	    		u ? (NMath::scalar_t)ncf::util::to_double(u) : def.x,
		    	v ? (NMath::scalar_t)ncf::util::to_double(v) : def.y
    		);
        }
        else if (has_property) {
            float u, v;
            std::string val = node->get_property_by_name(name);
            if (2 == sscanf(val.c_str(), XTPROTO_FORMAT_TEX2, &u, &v)) res = NMath::Vector2f(u, v);
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
			    r ? (NMath::scalar_t)ncf::util::to_double(r) : def.r(),
    			g ? (NMath::scalar_t)ncf::util::to_double(g) : def.g(),
	    		b ? (NMath::scalar_t)ncf::util::to_double(b) : def.b()
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

NMath::Vector3f deserialize_vec3(const ncf::NCF *node, const char *name, const NMath::Vector3f def)
{
	NMath::Vector3f res = def;

	if (node) {
        bool has_property = node->query_property(name);
        bool has_group    = node->query_group(name);

        if (has_group) {
            ncf::NCF *group = node->get_group_by_name(name);
    		const char *x = group->get_property_by_name(XTPROTO_PROP_CRD_X);
	    	const char *y = group->get_property_by_name(XTPROTO_PROP_CRD_Y);
		    const char *z = group->get_property_by_name(XTPROTO_PROP_CRD_Z);

            res = NMath::Vector3f(
                x ? (NMath::scalar_t)ncf::util::to_double(x) : def.x,
                y ? (NMath::scalar_t)ncf::util::to_double(y) : def.y,
                z ? (NMath::scalar_t)ncf::util::to_double(z) : def.z
            );
        }
        else if (has_property) {
            float x, y, z;
            std::string val = node->get_property_by_name(name);
            if (3 == sscanf(val.c_str(), XTPROTO_FORMAT_VEC3, &x, &y, &z)) res = NMath::Vector3f(x, y, z);
        }
	}

	return res;
}

xtcore::assets::Cubemap *deserialize_cubemap(const char *source, const ncf::NCF *p)
{
    if (!p) return 0;

    xtcore::assets::Cubemap *data = new xtcore::assets::Cubemap;

    ncf::NCF *n = p->get_group_by_name(XTPROTO_NODE_CUBEMAP);

    if (data) {
        std::string posx = deserialize_cstr(n->get_property_by_name(XTPROTO_LTRL_POSX));
        std::string posy = deserialize_cstr(n->get_property_by_name(XTPROTO_LTRL_POSY));
        std::string posz = deserialize_cstr(n->get_property_by_name(XTPROTO_LTRL_POSZ));
        std::string negx = deserialize_cstr(n->get_property_by_name(XTPROTO_LTRL_NEGX));
        std::string negy = deserialize_cstr(n->get_property_by_name(XTPROTO_LTRL_NEGY));
        std::string negz = deserialize_cstr(n->get_property_by_name(XTPROTO_LTRL_NEGZ));

        std::string base, file, fsource = source;
    	ncf::util::path_comp(fsource, base, file);

        data->load((base + posx).c_str(), xtcore::assets::CUBEMAP_FACE_RIGHT);
        data->load((base + posy).c_str(), xtcore::assets::CUBEMAP_FACE_TOP);
        data->load((base + posz).c_str(), xtcore::assets::CUBEMAP_FACE_FRONT);
        data->load((base + negx).c_str(), xtcore::assets::CUBEMAP_FACE_LEFT);
        data->load((base + negy).c_str(), xtcore::assets::CUBEMAP_FACE_BOTTOM);
        data->load((base + negz).c_str(), xtcore::assets::CUBEMAP_FACE_BACK);
    }
    return data;
}


xtcore::assets::ICamera *deserialize_camera_tlp(const char *source, const ncf::NCF *p)
{
    if (!p) return 0;

    assets::ICamera *data = new (std::nothrow) CamPerspective();

    CamPerspective *cam = (CamPerspective *)data;
    cam->position = deserialize_vec3(p, XTPROTO_PROP_POSITION);
    cam->target   = deserialize_vec3(p, XTPROTO_PROP_TARGET);
    cam->up       = deserialize_vec3(p, XTPROTO_PROP_UP);
    cam->fov      = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_FOV));
    cam->flength  = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_FLENGTH));
    cam->aperture = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_APERTURE));

    return data;
}

xtcore::assets::ICamera *deserialize_camera_cbm(const char *source, const ncf::NCF *p)
{
    if (!p) return 0;

    assets::ICamera *data = new (std::nothrow) CamCubemap();

    CamCubemap *cam = (CamCubemap *)data;
    cam->position = deserialize_vec3(p, XTPROTO_PROP_POSITION);

    return data;
}

xtcore::assets::ICamera *deserialize_camera_ods(const char *source, const ncf::NCF *p)
{
    if (!p) return 0;

    assets::ICamera *data = new (std::nothrow) CamODS();

    CamODS *cam = (CamODS *)data;
    cam->position    = deserialize_vec3(p, XTPROTO_PROP_POSITION);
    cam->orientation = deserialize_vec3(p, XTPROTO_PROP_ORIENTATION);
    cam->ipd         = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_IPD));

    return data;
}

xtcore::assets::ICamera *deserialize_camera_erp(const char *source, const ncf::NCF *p)
{
    if (!p) return 0;

    assets::ICamera *data = new (std::nothrow) CamERP();

    CamERP *cam = (CamERP *)data;
    cam->position    = deserialize_vec3(p, XTPROTO_PROP_POSITION);
    cam->orientation = deserialize_vec3(p, XTPROTO_PROP_ORIENTATION);

    return data;
}

xtcore::assets::ICamera *deserialize_camera(const char *source, const ncf::NCF *p)
{
    if (!p) return 0;

    xtcore::assets::ICamera *data = 0;

    std::string type = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_TYPE));

         if (!type.compare(XTPROTO_LTRL_CAM_THINLENS)) data = deserialize_camera_tlp(source, p);
    else if (!type.compare(XTPROTO_LTRL_CAM_ODS)     ) data = deserialize_camera_ods(source, p);
    else if (!type.compare(XTPROTO_LTRL_CAM_ERP)     ) data = deserialize_camera_erp(source, p);
    else if (!type.compare(XTPROTO_LTRL_CAM_CUBEMAP) ) data = deserialize_camera_cbm(source, p);
    else Log::handle().post_warning("Unsupported camera type %s [%s]. Skipping..", p->get_name(), type.c_str());

	return data;
}

xtcore::assets::Geometry *deserialize_geometry_mesh(const char *source, const ncf::NCF *p)
{
	if (!p) return 0;

    xtcore::assets::Geometry *data = new (std::nothrow) nmesh::Mesh;

    nmesh::object_t obj;

    std::string f = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_SOURCE));

    std::string token;
    void* buffer = malloc(200 * sizeof(char));
    memset(buffer, 0, 200 * sizeof(char));
    int res = sscanf(f.c_str(), XTPROTO_FORMAT_GENERATE, (char*)buffer);
    if (buffer && *((char*)buffer)) token = (char*)buffer;
    free(buffer);

    // Procedural meshes
    if (res > 0) {
        if (!token.compare(XTPROTO_LTRL_ICOSAHEDRON)) nmesh::generator::icosahedron(&obj);
        else Log::handle().post_message("Invalid mesh generator: %s", token.c_str());
    }
    // External sources
    else {
        // Open source file from relative path
	    std::string base, file, fsource = source;
		ncf::util::path_comp(fsource, base, file);
    	base.append(f);

	    Log::handle().post_message("Loading data from %s", base.c_str());

	    if (nmesh::io::import::obj(base.c_str(), obj))
    	{
    		Log::handle().post_warning("Failed to load mesh from %s", f.c_str());
	    	delete data;
            return 0;
		}
    }

    if (p->query_group(XTPROTO_MODIFIERS)) {
        ncf::NCF *mods = p->get_group_by_name(XTPROTO_MODIFIERS);

    	NMath::Vector3f xform_rot = deserialize_vec3(mods, XTPROTO_PROP_ROTATION    , NMath::Vector3f(0,0,0));
    	NMath::Vector3f xform_scl = deserialize_vec3(mods, XTPROTO_PROP_SCALE       , NMath::Vector3f(1,1,1));
    	NMath::Vector3f xform_tsl = deserialize_vec3(mods, XTPROTO_PROP_TRANSLATION , NMath::Vector3f(0,0,0));
    	nmesh::mutator::rotate   (obj, xform_rot.x, xform_rot.y, xform_rot.z);
    	nmesh::mutator::scale    (obj, xform_scl.x, xform_scl.y, xform_scl.z);
    	nmesh::mutator::translate(obj, xform_tsl.x, xform_tsl.y, xform_tsl.z);

        if (mods->query_property(XTPROTO_FLIP_NORMALS)) {
            bool flag = deserialize_bool(mods->get_property_by_name(XTPROTO_FLIP_NORMALS));
            Log::handle().post_message("Applying modifier: flip normals..");
            if (flag) nmesh::mutator::invert_normals(&obj);
        }

        if (mods->query_group(XTPROTO_EXTRUDE)) {
            xtcore::assets::Cubemap *cb = deserialize_cubemap(source, mods->get_group_by_name(XTPROTO_EXTRUDE));
            Log::handle().post_message("Applying modifier: extrude..");
            xtcore::auxiliary::extrude(&obj, cb);
        }
    }

    Log::handle().post_message("Building octree..");
	((nmesh::Mesh *)data)->build_octree(obj);

    return data;
}

xtcore::assets::Geometry *deserialize_geometry(const char *source, const ncf::NCF *p)
{
	if (!p) return 0;

    xtcore::assets::Geometry *data = 0;

	std::string type = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_TYPE));

	if (!type.compare(XTPROTO_LTRL_PLANE)) {
		data = new (std::nothrow) NMath::Plane;
		((NMath::Plane *)data)->normal   = deserialize_vec3(p, XTPROTO_PROP_NORMAL);
		((NMath::Plane *)data)->distance = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_DISTANCE));
	}
	else if (!type.compare(XTPROTO_LTRL_SPHERE)) {
		data = new (std::nothrow) NMath::Sphere;
		((NMath::Sphere *)data)->origin = deserialize_vec3(p, XTPROTO_PROP_POSITION);
		((NMath::Sphere *)data)->radius = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_RADIUS));
	}
    else if (!type.compare(XTPROTO_LTRL_POINT)) {
        data = new (std::nothrow) NMath::Sphere;
		((NMath::Sphere *)data)->origin = deserialize_vec3(p, XTPROTO_PROP_POSITION);
		((NMath::Sphere *)data)->radius = 0;
    }
	else if (!type.compare(XTPROTO_LTRL_TRIANGLE)) {
		data = new (std::nothrow) NMath::Triangle;

        NMath::Triangle *tri = (NMath::Triangle *)data;

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
    	data->uv_scale = NMath::Vector2f(
    		deserialize_numf(p->get_property_by_name(XTPROTO_PROP_USCALE)),
	    	deserialize_numf(p->get_property_by_name(XTPROTO_PROP_VSCALE))
    	);

        data->calc_aabb();
    }

	return data;
}

xtcore::assets::IMaterial *deserialize_material_lambert(const char *source, const ncf::NCF *p)
{
    return new (std::nothrow) xtcore::assets::MaterialLambert();
}

xtcore::assets::IMaterial *deserialize_material_phong(const char *source, const ncf::NCF *p)
{
    return new (std::nothrow) xtcore::assets::MaterialPhong();
}

xtcore::assets::IMaterial *deserialize_material_blinnphong(const char *source, const ncf::NCF *p)
{
    return new (std::nothrow) xtcore::assets::MaterialBlinnPhong();
}

xtcore::assets::ISampler *deserialize_rgba(const char *source, const ncf::NCF *p)
{
    xtcore::assets::SolidColor *sampler = new (std::nothrow) xtcore::assets::SolidColor();
    nimg::ColorRGBf col = deserialize_col3(p, XTPROTO_VALUE);
    ((xtcore::assets::SolidColor *)sampler)->set(col);
    return sampler;
}

xtcore::assets::IMaterial *deserialize_material(const char *source, const ncf::NCF *p)
{
	if (!p) return 0;

    xtcore::assets::IMaterial *data = 0;

	std::string type = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_TYPE));

	     if (!type.compare(XTPROTO_LTRL_LAMBERT)   ) data = deserialize_material_lambert(source, p);
	else if (!type.compare(XTPROTO_LTRL_PHONG)     ) data = deserialize_material_phong(source, p);
	else if (!type.compare(XTPROTO_LTRL_BLINNPHONG)) data = deserialize_material_blinnphong(source, p);
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

            xtcore::assets::ISampler *sampler = 0;
            std::string type = deserialize_cstr(entry->get_property_by_name(XTPROTO_PROP_TYPE));

                 if (!type.compare(XTPROTO_TEXTURE)) sampler = deserialize_texture(source, entry);
            else if (!type.compare(XTPROTO_CUBEMAP)) sampler = deserialize_cubemap(source, entry);
            else if (!type.compare(XTPROTO_COLOR  )) sampler = deserialize_rgba   (source, entry);

            data->add_sampler(entry->get_name(), sampler);
        }

        for (size_t i = 0; i < gscalars->count_properties(); ++i) {
            std::string     name  = deserialize_cstr(gscalars->get_property_name_by_index(i));
            NMath::scalar_t value = deserialize_numf(gscalars->get_property_by_index(i));
            data->add_scalar(name.c_str(), value);
        }
    }

	return data;
}

xtcore::assets::Texture2D *deserialize_texture(const char *source, const ncf::NCF *p)
{
	if (!p)	return 0;

	xtcore::assets::Texture2D *data = new (std::nothrow) xtcore::assets::Texture2D;

	std::string script_base, script_filename, fsource = source;
	ncf::util::path_comp(fsource, script_base, script_filename);

    std::string fname  = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_SOURCE));
	std::string filter = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_FILTERING));
	std::string path = script_base + fname;

	Log::handle().post_message("Loading texture: %s", path.c_str());
	if (data->load(path.c_str())) {
	    Log::handle().post_error("Failed to texture: %s", path.c_str());
        delete data;
        return 0;
	}

  	if      ( filter.empty()
   	      || !filter.compare(XTPROTO_LTRL_NEAREST )) { data->set_filtering(xtcore::assets::FILTERING_NEAREST);  }
	else if (!filter.compare(XTPROTO_LTRL_BILINEAR)) { data->set_filtering(xtcore::assets::FILTERING_BILINEAR); }
	else {
		Log::handle().post_warning("Invalid filtering method: %s", filter.c_str());
	}

    return data;
}

xtcore::assets::Object *deserialize_object(const char *source, const ncf::NCF *p)
{
	if (!p)	return 0;

    xtcore::assets::Object *data = new (std::nothrow) xtcore::assets::Object;

	const char *g = p->get_property_by_name(XTPROTO_PROP_OBJ_GEO);
	const char *m = p->get_property_by_name(XTPROTO_PROP_OBJ_MAT);

	data->geometry = deserialize_cstr(g);
	data->material = deserialize_cstr(m);

	return data;
}

    } /* namespace io */
} /* namespace xtcore */
