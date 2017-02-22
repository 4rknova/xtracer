#include <algorithm>

#include <ncf/util.h>

#include <nmath/plane.h>
#include <nmath/sphere.h>
#include <nmath/triangle.h>
#include <nmesh/mesh.h>
#include <nmesh/transform.h>
#include <nmesh/obj.h>
#include <nimg/checkerboard.h>

#include "proto.h"
#include "parseutil.h"

#include "log.h"
#include "cam_perspective.h"
#include "cam_ods.h"
#include "cam_erp.h"

namespace xtracer {
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
	return val ? (NMath::scalar_t)Util::String::to_double(val) : def;
}

std::string deserialize_cstr(const char *val, const char* def)
{
	return val ? val : def;
}

NMath::Vector2f deserialize_tex2(const NCF *node, const NMath::Vector2f def)
{
	NMath::Vector2f res = def;

	if (node) {
		const char *u = node->get_property_by_name(XTPROTO_PROP_CRD_U);
		const char *v = node->get_property_by_name(XTPROTO_PROP_CRD_V);

		res = NMath::Vector2f(
			u ? (NMath::scalar_t)Util::String::to_double(u) : def.x,
			v ? (NMath::scalar_t)Util::String::to_double(v) : def.y
		);
	}

	return res;
}

nimg::ColorRGBf deserialize_col3(const NCF *node, const nimg::ColorRGBf def)
{
	nimg::ColorRGBf res = def;

	if (node) {
		const char *r = node->get_property_by_name(XTPROTO_PROP_COL_R);
		const char *g = node->get_property_by_name(XTPROTO_PROP_COL_G);
		const char *b = node->get_property_by_name(XTPROTO_PROP_COL_B);

		res = nimg::ColorRGBf(
			r ? (NMath::scalar_t)Util::String::to_double(r) : def.r(),
			g ? (NMath::scalar_t)Util::String::to_double(g) : def.g(),
			b ? (NMath::scalar_t)Util::String::to_double(b) : def.b()
		);
	}

	return res;
}

NMath::Vector3f deserialize_vec3(const NCF *node, const NMath::Vector3f def)
{
	NMath::Vector3f res = def;

	if (node) {
		const char *x = node->get_property_by_name(XTPROTO_PROP_CRD_X);
		const char *y = node->get_property_by_name(XTPROTO_PROP_CRD_Y);
		const char *z = node->get_property_by_name(XTPROTO_PROP_CRD_Z);

		res = NMath::Vector3f(
			x ? (NMath::scalar_t)Util::String::to_double(x) : def.x,
			y ? (NMath::scalar_t)Util::String::to_double(y) : def.y,
			z ? (NMath::scalar_t)Util::String::to_double(z) : def.z
		);
	}

	return res;
}

int deserialize_cubemap(const char *source, const NCF *p, xtracer::assets::Cubemap &data)
{
    if (!p) return 1;

    NCF *n = p->get_group_by_name(XTPROTO_NODE_CUBEMAP);

    std::string posx = xtracer::io::deserialize_cstr(n->get_property_by_name(XTPROTO_LTRL_POSX));
    std::string posy = xtracer::io::deserialize_cstr(n->get_property_by_name(XTPROTO_LTRL_POSY));
    std::string posz = xtracer::io::deserialize_cstr(n->get_property_by_name(XTPROTO_LTRL_POSZ));
    std::string negx = xtracer::io::deserialize_cstr(n->get_property_by_name(XTPROTO_LTRL_NEGX));
    std::string negy = xtracer::io::deserialize_cstr(n->get_property_by_name(XTPROTO_LTRL_NEGY));
    std::string negz = xtracer::io::deserialize_cstr(n->get_property_by_name(XTPROTO_LTRL_NEGZ));

    std::string base, file, fsource = source;
	Util::String::path_comp(fsource, base, file);

    data.load((base + posx).c_str(), xtracer::assets::CUBEMAP_FACE_RIGHT);
    data.load((base + posy).c_str(), xtracer::assets::CUBEMAP_FACE_TOP);
    data.load((base + posz).c_str(), xtracer::assets::CUBEMAP_FACE_FRONT);
    data.load((base + negx).c_str(), xtracer::assets::CUBEMAP_FACE_LEFT);
    data.load((base + negy).c_str(), xtracer::assets::CUBEMAP_FACE_BOTTOM);
    data.load((base + negz).c_str(), xtracer::assets::CUBEMAP_FACE_BACK);

    return 0;
}

xtracer::assets::ICamera *deserialize_camera(const char *source, const NCF *p)
{
    if (!p) return 0;

    xtracer::assets::ICamera *data = 0;

    std::string type = deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_TYPE));

    if (!type.compare(XTPROTO_LTRL_CAM_THINLENS)) {
    	NMath::Vector3f pos     = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_POSITION));
    	NMath::Vector3f target  = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_TARGET));
    	NMath::Vector3f up      = deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_UP));
       	NMath::scalar_t fov     = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_FOV));
    	NMath::scalar_t flength = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_FLENGTH));
    	NMath::scalar_t ap      = deserialize_numf(p->get_property_by_name(XTPROTO_PROP_APERTURE));

	    data = new (std::nothrow) CamPerspective();
        ((CamPerspective *)data)->position = pos;
        ((CamPerspective *)data)->target   = target;
        ((CamPerspective *)data)->up       = up;
        ((CamPerspective *)data)->fov      = fov;
        ((CamPerspective *)data)->flength  = flength;
        ((CamPerspective *)data)->aperture = ap;
    }
    else if (!type.compare(XTPROTO_LTRL_CAM_ODS)) {
	    NMath::Vector3f pos = xtracer::io::deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_POSITION));
	    NMath::Vector3f orn = xtracer::io::deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_ORIENTATION));
	    NMath::scalar_t ipd = xtracer::io::deserialize_numf(p->get_property_by_name(XTPROTO_PROP_IPD));

        data = new (std::nothrow) CamODS();
        ((CamODS *)data)->position    = pos;
        ((CamODS *)data)->orientation = orn;
        ((CamODS *)data)->ipd         = ipd;
    }
    else if (!type.compare(XTPROTO_LTRL_CAM_ERP)) {
	    NMath::Vector3f pos = xtracer::io::deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_POSITION));
	    NMath::Vector3f orn = xtracer::io::deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_ORIENTATION));

        data = new (std::nothrow) CamERP();
        ((CamERP *)data)->position    = pos;
        ((CamERP *)data)->orientation = orn;
    }
    else {
		Log::handle().post_warning("Unsupported camera type %s [%s]. Skipping..", p->get_name(), type.c_str());
    }

	return data;
}

xtracer::assets::Geometry *deserialize_geometry(const char *source, const NCF *p)
{
	if (!p) return 0;

    xtracer::assets::Geometry *data = 0;

	std::string type = xtracer::io::deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_TYPE));

	if (!type.compare(XTPROTO_LTRL_PLANE)) {
		data = new (std::nothrow) NMath::Plane;
		((NMath::Plane *)data)->normal   = xtracer::io::deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_NORMAL));
		((NMath::Plane *)data)->distance = xtracer::io::deserialize_numf(p->get_property_by_name(XTPROTO_PROP_DISTANCE));
	}
	else if (!type.compare(XTPROTO_LTRL_SPHERE)) {
		data = new (std::nothrow) NMath::Sphere;
		((NMath::Sphere *)data)->origin = xtracer::io::deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_POSITION));
		((NMath::Sphere *)data)->radius = xtracer::io::deserialize_numf(p->get_property_by_name(XTPROTO_PROP_RADIUS));
	}
    else if (!type.compare(XTPROTO_LTRL_POINT)) {
        data = new (std::nothrow) NMath::Sphere;
		((NMath::Sphere *)data)->origin = xtracer::io::deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_POSITION));
		((NMath::Sphere *)data)->radius = 0;
    }
	else if (!type.compare(XTPROTO_LTRL_TRIANGLE)) {
		data = new (std::nothrow) NMath::Triangle;
		for (size_t i = 0; i < 3; ++i) {
			NCF *vnode = p->get_group_by_name(XTPROTO_PROP_VRTXDATA)->get_group_by_index(i);
			((NMath::Triangle *)data)->v[i]  = xtracer::io::deserialize_vec3(vnode);
			((NMath::Triangle *)data)->tc[i] = xtracer::io::deserialize_tex2(vnode);
		}
	}
	// - Mesh
	else if (!type.compare(XTPROTO_LTRL_MESH)) {
		data = new (std::nothrow) NMesh::Mesh;

		// Data source
		std::string f = p->get_property_by_name(XTPROTO_PROP_SOURCE);

		// Open source file from relative path
		std::string base, file, fsource = source;
		Util::String::path_comp(fsource, base, file);
		base.append(f);

		Log::handle().post_message("Loading data from %s", base.c_str());

        NMesh::object_t obj;

		if (NMesh::IO::Import::obj(base.c_str(), obj))
		{
			Log::handle().post_warning("Failed to load mesh from %s", f.c_str());
			delete data;
            return 0;
		}

		if (p->query_group(XTPROTO_PROP_ROTATION)) {
			NMath::Vector3f v = xtracer::io::deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_ROTATION));
			NMesh::Mutator::rotate(obj, v.x, v.y, v.z);
		}

		if (p->query_group(XTPROTO_PROP_SCALE)) {
			NMath::Vector3f v = xtracer::io::deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_SCALE));
			NMesh::Mutator::scale(obj, v.x, v.y, v.z);
		}

		if (p->query_group(XTPROTO_PROP_TRANSLATION)) {
			NMath::Vector3f v = xtracer::io::deserialize_vec3(p->get_group_by_name(XTPROTO_PROP_TRANSLATION));
			NMesh::Mutator::translate(obj, v.x, v.y, v.z);
		}

		Log::handle().post_message("Building octree..");
		((NMesh::Mesh *)data)->build_octree(obj);
	}
	// unknown
	else {
		Log::handle().post_warning("Unsupported geometry type %s [%s]. Skipping..", p->get_name(), type.c_str());
		return 0;
	}

	data->uv_scale = NMath::Vector2f(
		xtracer::io::deserialize_numf(p->get_property_by_name(XTPROTO_PROP_USCALE)),
		xtracer::io::deserialize_numf(p->get_property_by_name(XTPROTO_PROP_VSCALE))
	);

	data->calc_aabb();

	return data;
}

xtracer::assets::Material *deserialize_material(const char *source, const NCF *p)
{
	if (!p) return 0;

    xtracer::assets::Material *data = new (std::nothrow) xtracer::assets::Material;

	std::string type = xtracer::io::deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_TYPE));

	     if (!type.compare(XTPROTO_LTRL_LAMBERT)   ) data->type = xtracer::assets::MATERIAL_LAMBERT;
	else if (!type.compare(XTPROTO_LTRL_PHONG)     ) data->type = xtracer::assets::MATERIAL_PHONG;
	else if (!type.compare(XTPROTO_LTRL_BLINNPHONG)) data->type = xtracer::assets::MATERIAL_BLINNPHONG;
	else if (!type.compare(XTPROTO_LTRL_EMISSIVE  )) data->type = xtracer::assets::MATERIAL_EMISSIVE;
	else {
		Log::handle().post_warning("Unsupported material %s. Skipping..", p->get_name());
		delete data;
		return 0;
	}

	data->ambient      = xtracer::io::deserialize_col3(p->get_group_by_name(XTPROTO_PROP_IAMBN));
	data->specular     = xtracer::io::deserialize_col3(p->get_group_by_name(XTPROTO_PROP_ISPEC));
	data->diffuse      = xtracer::io::deserialize_col3(p->get_group_by_name(XTPROTO_PROP_IDIFF));
	data->emissive     = xtracer::io::deserialize_col3(p->get_group_by_name(XTPROTO_PROP_EMISSIVE));
	data->kdiff        = xtracer::io::deserialize_numf(p->get_property_by_name(XTPROTO_PROP_KDIFF), 1.f);
	data->kspec        = xtracer::io::deserialize_numf(p->get_property_by_name(XTPROTO_PROP_KSPEC), 0.f);
	data->ksexp        = xtracer::io::deserialize_numf(p->get_property_by_name(XTPROTO_PROP_KEXPN), 0.f);
	data->roughness    = xtracer::io::deserialize_numf(p->get_property_by_name(XTPROTO_PROP_ROUGH), 0.f);
	data->transparency = xtracer::io::deserialize_numf(p->get_property_by_name(XTPROTO_PROP_TRSPC), 0.f);
	data->reflectance  = xtracer::io::deserialize_numf(p->get_property_by_name(XTPROTO_PROP_REFLC), 0.f);
	data->ior          = xtracer::io::deserialize_numf(p->get_property_by_name(XTPROTO_PROP_IOR  ), 1.f);

	return data;
}

xtracer::assets::Texture2D *deserialize_texture(const char *source, const NCF *p)
{
	if (!p)	return 0;

	xtracer::assets::Texture2D *data = new (std::nothrow) xtracer::assets::Texture2D;

	std::string script_base, script_filename, fsource = source;
	Util::String::path_comp(fsource, script_base, script_filename);

    std::string fname  = xtracer::io::deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_SOURCE));
	std::string filter = xtracer::io::deserialize_cstr(p->get_property_by_name(XTPROTO_PROP_FILTERING));
	std::string path = script_base + fname;

	Log::handle().post_message("Loading data from %s", path.c_str());
	if (data->load(path.c_str())) {
		Log::handle().post_warning("Failed to load texture [%s->%s]", p->get_name(), path.c_str());
		Log::handle().post_warning("Replacing with checkerboard..");

		Pixmap tex;
		ColorRGBAf a   = ColorRGBAf(0.5,0.5,0.5,1);
		ColorRGBAf b   = ColorRGBAf(1,1,1,1);
		nimg::generator::checkerboard(tex, 2, 2, a, b);

		data->load(tex);
	}

  	if      ( filter.empty()
   	      || !filter.compare(XTPROTO_LTRL_NEAREST )) { data->set_filtering(xtracer::assets::FILTERING_NEAREST);  }
	else if (!filter.compare(XTPROTO_LTRL_BILINEAR)) { data->set_filtering(xtracer::assets::FILTERING_BILINEAR); }
	else {
		Log::handle().post_warning("Invalid filtering method: %s", filter.c_str());
	}

    return data;
}

xtracer::assets::Object *deserialize_object(const char *source, const NCF *p)
{
	if (!p)	return 0;

    xtracer::assets::Object *data = new (std::nothrow) xtracer::assets::Object;

	const char *g = p->get_property_by_name(XTPROTO_PROP_OBJ_GEO);
	const char *m = p->get_property_by_name(XTPROTO_PROP_OBJ_MAT);
	const char *t = p->get_property_by_name(XTPROTO_PROP_OBJ_TEX);

	data->geometry = xtracer::io::deserialize_cstr(g);
	data->material = xtracer::io::deserialize_cstr(m);
	data->texture  = xtracer::io::deserialize_cstr(t);

	return data;
}

    } /* namespace io */
} /* namespace xtracer */
