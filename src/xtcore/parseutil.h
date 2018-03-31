#ifndef XTCORE_PARSEUTIL_H_INCLUDED
#define XTCORE_PARSEUTIL_H_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.h>
#include <ncf/ncf.h>
#include "math/surface.h"
#include "camera.h"
#include "material.h"
#include "sampler_tex.h"
#include "sampler_col.h"
#include "object.h"
#include "cubemap.h"
#include "scene.h"

namespace xtcore {
    namespace io {
        namespace scn {

/* Default Values */
#define DEFVAL_BOOL (false)
#define DEFVAL_NUMF (0.f)
#define DEFVAL_CSTR ("")
#define DEFVAL_TEX2 (NMath::Vector2f(0.f,0.f))
#define DEFVAL_VEC3 (NMath::Vector3f(0.f,0.f,0.f))
#define DEFVAL_COL3 (nimg::ColorRGBf(0.f,0.f,0.f))

bool            deserialize_bool(const char *val, const bool            def = DEFVAL_BOOL);
NMath::scalar_t deserialize_numf(const char *val, const NMath::scalar_t def = DEFVAL_NUMF);
std::string     deserialize_cstr(const char *val, const char*           def = DEFVAL_CSTR);
NMath::Vector2f deserialize_tex2(const ncf::NCF *node, const char *name, const NMath::Vector2f def = DEFVAL_TEX2);
NMath::Vector3f deserialize_vec3(const ncf::NCF *node, const char *name, const NMath::Vector3f def = DEFVAL_VEC3);
nimg::ColorRGBf deserialize_col3(const ncf::NCF *node, const char *name, const nimg::ColorRGBf def = DEFVAL_COL3);

xtcore::asset::ICamera      *deserialize_camera   (const char *source, const ncf::NCF *p);
xtcore::asset::IMaterial    *deserialize_material (const char *source, const ncf::NCF *p);
xtcore::asset::ISurface     *deserialize_geometry (const char *source, const ncf::NCF *p);
xtcore::asset::Object       *deserialize_object   (const char *source, const ncf::NCF *p);
xtcore::sampler::Texture2D  *deserialize_texture  (const char *source, const ncf::NCF *p);
xtcore::sampler::Cubemap    *deserialize_cubemap  (const char *source, const ncf::NCF *p);
xtcore::sampler::ISampler   *deserialize_rgba     (const char *source, const ncf::NCF *p);

int create_cubemap  (Scene *scene, ncf::NCF *p);
int create_camera   (Scene *scene, ncf::NCF *p);
int create_material (Scene *scene, ncf::NCF *p);
int create_geometry (Scene *scene, ncf::NCF *p);
int create_object   (Scene *scene, ncf::NCF *p);

int load(Scene *scene, const char *filename, const std::list<std::string> *modifiers = 0);

        } /* namespace scn */
    } /* namespace io */
} /* namespace xtcore */

#endif /* XTCORE_PARSEUTIL_H_INCLUDED */
