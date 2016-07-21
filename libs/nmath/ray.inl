#ifndef XTCORE_RAY_INL_INCLUDED
#define XTCORE_RAY_INL_INCLUDED

#ifndef XTCORE_RAY_H_INCLUDED
    #error "ray.h must be included before ray.inl"
#endif /* XTCORE_RAY_H_INCLUDED */

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

static inline ray_t ray_pack(vec3_t origin, vec3_t direction)
{
	ray_t r;
	r.origin = origin;
	r.direction = vec3_normalize(direction);
	return r;
}

#ifdef __cplusplus
}
#endif	/* __cplusplus */

} /* namespace NMath */

#endif /* XTCORE_RAY_INL_INCLUDED */
