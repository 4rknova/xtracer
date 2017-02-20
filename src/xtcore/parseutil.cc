#include <algorithm>
#include <ncf/util.h>
#include "proto.h"
#include "parseutil.h"

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

    } /* namespace io */
} /* namesp:ace xtracer */
