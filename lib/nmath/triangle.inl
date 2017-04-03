#ifndef XTCORE_TRIANGLE_INL_INCLUDED
#define XTCORE_TRIANGLE_INL_INCLUDED

#ifndef XTCORE_TRIANGLE_H_INCLUDED
    #error "triangle.h must be included before triangle.inl"
#endif /* XTCORE_TRIANGLE_H_INCLUDED */

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

static inline triangle_t triangle_pack(vec3_t v0, vec3_t v1, vec3_t v2)
{
	triangle_t t;
	t.v[0] = v0;
	t.v[1] = v1;
	t.v[2] = v2;
	return t;
}

#ifdef __cplusplus
}
#endif	/* __cplusplus */

} /* namespace NMath */

#endif /* XTCORE_TRIANGLE_INL_INCLUDED */
