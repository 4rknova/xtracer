#ifndef XTCORE_PARSEUTIL_H_INCLUDED
#define XTCORE_PARSEUTIL_H_INCLUDED

#include <nmath/vector.h>
#include <nimg/color.h>
#include <ncf/ncf.h>
#include <map>
#include <memory>
#include <list>
#include <string>
#include <vector>
#include "math/surface.h"
#include "camera.h"
#include "material.h"
#include "sampler/sampler_tex.h"
#include "sampler/sampler_col.h"
#include "sampler/sampler_cubemap.h"
#include "sampler/sampler_gradient.h"
#include "sampler/sampler_rayleigh_sky.h"
#include "object.h"
#include "cubemap.h"
#include "scene.h"

namespace xtcore {
    namespace io {
        namespace scn {

/* Default Values */
#define DEFVAL_BOOL (false)
#define DEFVAL_NUMI (0)
#define DEFVAL_NUMF (0.f)
#define DEFVAL_CSTR ("")
#define DEFVAL_TEX2 (nmath::Vector2f(0.f,0.f))
#define DEFVAL_VEC3 (nmath::Vector3f(0.f,0.f,0.f))
#define DEFVAL_COL3 (nimg::ColorRGBf(0.f,0.f,0.f))

bool            deserialize_bool(const char *val, const bool            def = DEFVAL_BOOL);
int             deserialize_numi(const char *val, const int             def = DEFVAL_NUMI);
nmath::scalar_t deserialize_numf(const char *val, const nmath::scalar_t def = DEFVAL_NUMF);
std::string     deserialize_cstr(const char *val, const char*           def = DEFVAL_CSTR);
nmath::Vector2f deserialize_tex2(const ncf::NCF *node, const char *name, const nmath::Vector2f def = DEFVAL_TEX2);
nmath::Vector3f deserialize_vec3     (const ncf::NCF *node, const char *name, const nmath::Vector3f def = DEFVAL_VEC3);
nmath::Vector3f deserialize_euler    (const ncf::NCF *node, const char *name, const nmath::Vector3f def = DEFVAL_VEC3);
nmath::Vector3f deserialize_radians  (const ncf::NCF *node, const char *name, const nmath::Vector3f def = DEFVAL_VEC3);
// Use this for all rotation properties — accepts vec3(rad), euler(deg), radians(rad).
// TODO: migrate remaining deserialize_vec3(... ROTATION ...) call sites to this.
nmath::Vector3f deserialize_rotation (const ncf::NCF *node, const char *name, const nmath::Vector3f def = DEFVAL_VEC3);
nimg::ColorRGBf deserialize_col3(const ncf::NCF *node, const char *name, const nimg::ColorRGBf def = DEFVAL_COL3);

xtcore::asset::ICamera      *deserialize_camera   (const char *source, const ncf::NCF *p);
xtcore::asset::IMaterial    *deserialize_material (const char *source, const ncf::NCF *p);
xtcore::asset::ISurface     *deserialize_geometry (const char *source, const ncf::NCF *p);
xtcore::asset::Object       *deserialize_object   (const char *source, const ncf::NCF *p);
xtcore::sampler::Texture2D  *deserialize_texture  (const char *source, const ncf::NCF *p);
xtcore::sampler::Cubemap    *deserialize_cubemap  (const char *source, const ncf::NCF *p);
xtcore::sampler::Gradient   *deserialize_gradient (const ncf::NCF *p);
xtcore::sampler::RayleighSky *deserialize_rayleigh_sky(const ncf::NCF *p);
xtcore::sampler::ISampler   *deserialize_rgba     (const ncf::NCF *p);

int create_cubemap  (Scene *scene, ncf::NCF *p);
int create_camera   (Scene *scene, ncf::NCF *p);
int create_material (Scene *scene, ncf::NCF *p);
int create_geometry (Scene *scene, ncf::NCF *p);
int create_object   (Scene *scene, ncf::NCF *p);

std::string generate_geometry_mesh_json(const std::string &gen_id, const std::map<std::string, std::string> &params);

int load(Scene *scene,
         const char *filename,
         const std::list<std::string> *modifiers = 0,
         const char *variant = 0,
         std::vector<std::string> *out_warnings = 0);

enum async_load_state_t
{
    ASYNC_LOAD_QUEUED = 0,
    ASYNC_LOAD_RUNNING,
    ASYNC_LOAD_DONE,
    ASYNC_LOAD_ERROR
};

struct async_load_snapshot_t
{
    unsigned long long id;
    async_load_state_t state;
    std::string state_name;
    std::string filename;
    std::string error;
    std::vector<std::string> warnings;
};

unsigned long long load_async_start(const char *filename,
                                    const std::list<std::string> *modifiers = 0,
                                    const char *variant = 0);
bool load_async_snapshot(unsigned long long id, async_load_snapshot_t *out);
std::shared_ptr<Scene> load_async_take_scene(unsigned long long id);
void load_async_discard(unsigned long long id);
size_t load_async_gc_done(size_t keep_latest = 128);

        } /* namespace scn */
    } /* namespace io */
} /* namespace xtcore */

#endif /* XTCORE_PARSEUTIL_H_INCLUDED */
