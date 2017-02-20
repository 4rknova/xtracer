#ifndef XTRACER_PARSEUTIL_H_INCLUDED
#define XTRACER_PARSEUTIL_H_INCLUDED

#include <ncf/ncf.h>
#include <nmath/vector.h>
#include <nimg/color.h>

namespace xtracer {
    namespace io {

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
NMath::Vector2f deserialize_tex2(const NCF *node, const NMath::Vector2f def = DEFVAL_TEX2);
NMath::Vector3f deserialize_vec3(const NCF *node, const NMath::Vector3f def = DEFVAL_VEC3);
nimg::ColorRGBf deserialize_col3(const NCF *node, const nimg::ColorRGBf def = DEFVAL_COL3);

    } /* namespace io */
} /* namespace xtracer */

#endif /* XTRACER_PARSEUTIL_H_INCLUDED */