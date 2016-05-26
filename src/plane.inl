#ifndef XTCORE_PLANE_INL_INCLUDED
#define XTCORE_PLANE_INL_INCLUDED

#ifndef XTCORE_PLANE_H_INCLUDED
    #error "plane.h must be included before plane.inl"
#endif /* XTCORE_PLANE_H_INCLUDED */

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

static inline plane_t plane_pack(vec3_t normal, scalar_t distance)
{
	plane_t s;
	s.normal = normal;
	s.distance = distance;
	return s;
}

#ifdef __cplusplus
}
#endif	/* __cplusplus */

} /* namespace NMath */

#endif /* XTCORE_PLANE_INL_INCLUDED */
