/*

    This file is part of the libnmath.

    aabb.h
    Bounding box

    Copyright (C) 2008, 2010, 2011
    Papadopoulos Nikolaos

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General
    Public License along with this library; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301 USA

*/

#ifndef NMATH_BBOX_INL_INCLUDED
#define NMATH_BBOX_INL_INCLUDED

#ifndef NMATH_BBOX_H_INCLUDED
    #error "aabb.h must be included before aabb.inl"
#endif /* NMATH_BBOX_H_INCLUDED */

#include "types.h"
#include "vector.h"

namespace NMath {

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* C 2D bounding box functions */
static inline aabb2_t aabb2_pack(vec2_t a, vec2_t b)
{
    aabb2_t box;
	box.min.x = (a.x<=b.x)? a.x : b.x;
	box.min.y = (a.y<=b.y)? a.y : b.y;

	box.max.x = (a.x>=b.x)? a.x : b.x;
	box.max.y = (a.y>=b.y)? a.y : b.y;

	return box;
}

static inline short aabb2_contains(aabb2_t b, vec2_t v)
{
    return ( (v.x>=b.min.x) && (v.y>=b.min.y) && (v.x<=b.max.x) && (v.y<=b.max.y) )? 0 : 1;
}

static inline vec2_t aabb2_center(aabb2_t b)
{
    return vec2_pack( (b.min.x + b.max.x) / 2, (b.min.y + b.max.y) / 2 );
}

static inline aabb2_t aabb2_augment_by_vec(aabb2_t s, vec2_t v)
{
    if(v.x > s.max.x) s.max.x = v.x;
    if(v.x < s.min.x) s.min.x = v.x;

    if(v.y > s.max.y) s.max.y = v.y;
    if(v.y < s.min.y) s.min.y = v.y;
    return s;
}

static inline aabb2_t aabb2_augment_by_aabb(aabb2_t s, aabb2_t b)
{
    if(b.max.x > s.max.x) s.max.x = b.max.x;
    if(b.min.x < s.min.x) s.min.x = b.min.x;

    if(b.max.y > s.max.y) s.max.y = b.max.y;
    if(b.min.y < s.min.y) s.min.y = b.min.y;
    return s;
}

/* C 3D bounding box functions */
static inline aabb3_t aabb3_pack(vec3_t a, vec3_t b)
{
    aabb3_t box;
	box.min.x = (a.x<=b.x)? a.x : b.x;
	box.min.y = (a.y<=b.y)? a.y : b.y;
	box.min.z = (a.z<=b.z)? a.z : b.z;

	box.max.x = (a.x>=b.x)? a.x : b.x;
	box.max.y = (a.y>=b.y)? a.y : b.y;
	box.min.z = (a.z>=b.z)? a.z : b.z;

	return box;
}

static inline short aabb3_contains(aabb3_t b, vec3_t v)
{
    return ( (v.x>=b.min.x) && (v.y>=b.min.y) && (v.z>=b.min.z) && (v.x<=b.max.x) && (v.y<=b.max.y) && (v.z<=b.min.z) )? 0 : 1;
}

static inline vec3_t aabb3_center(aabb3_t b)
{
    return vec3_pack( (b.min.x + b.max.x) / 2, (b.min.y + b.max.y) / 2, (b.min.z + b.max.z) / 2 );
}

static inline aabb3_t aabb3_augment_by_vec(aabb3_t s, vec3_t v)
{
    if(v.x > s.max.x) s.max.x = v.x;
    if(v.x < s.min.x) s.min.x = v.x;

    if(v.y > s.max.y) s.max.y = v.y;
    if(v.y < s.min.y) s.min.y = v.y;

    if(v.z > s.max.z) s.max.z = v.z;
    if(v.z < s.min.z) s.min.z = v.z;
    return s;
}

static inline aabb3_t aabb3_augment_by_aabb(aabb3_t s, aabb3_t b)
{
    if(b.max.x > s.max.x) s.max.x = b.max.x;
    if(b.min.x < s.min.x) s.min.x = b.min.x;

    if(b.max.y > s.max.y) s.max.y = b.max.y;
    if(b.min.y < s.min.y) s.min.y = b.min.y;

    if(b.max.z > s.max.z) s.max.z = b.max.z;
    if(b.min.z < s.min.z) s.min.z = b.min.z;
    return s;
}

#ifdef __cplusplus
}

/* BoundingBox2 class */
inline bool BoundingBox2::contains(const Vector2f& p) const
{
    return (p.x >= min.x) && (p.y>=min.y) && (p.x<=max.x) && (p.y<=max.y);
}

inline Vector2f BoundingBox2::center() const
{
    return (min + max) / 2;
}

inline void BoundingBox2::augment(const Vector2f& v)
{
    if(v.x > max.x)	max.x = v.x;
    if(v.x < min.x)	min.x = v.x;

    if(v.y > max.y)	max.y = v.y;
    if(v.y < min.y)	min.y = v.y;
}

inline void BoundingBox2::augment(const BoundingBox2& b)
{
    if(b.max.x > max.x)	max.x = b.max.x;
    if(b.min.x < min.x)	min.x = b.min.x;

    if(b.max.y > max.y)	max.y = b.max.y;
    if(b.min.y < min.y)	min.y = b.min.y;
}

/* BoundingBox3 class */
inline bool BoundingBox3::contains(const Vector3f& p) const
{
    return (p.x>= min.x) && (p.y>=min.y) && (p.z>=min.z) && (p.x<=max.x) && (p.y<=max.y) && (p.z<=max.z);
}

inline bool BoundingBox3::contains(const BoundingBox3 &aabb) const
{

	if(min.x > aabb.max.x || aabb.min.x > max.x) {
		return false;
	}
	
	if(min.y > aabb.max.y || aabb.min.y > max.y) {
		return false;
	}

	if(min.z > aabb.max.z || aabb.min.z > max.z) {
		return false;
	}
	
	return true;
}

inline Vector3f BoundingBox3::center() const
{
    return (min + max) / 2.f;
}

inline void BoundingBox3::augment(const Vector3f& v)
{
    if(v.x > max.x)	max.x = v.x;
    if(v.x < min.x)	min.x = v.x;

    if(v.y > max.y)	max.y = v.y;
    if(v.y < min.y)	min.y = v.y;

    if(v.z > max.z)	max.z = v.z;
    if(v.z < min.z)	min.z = v.z;
}

inline void BoundingBox3::augment(const BoundingBox3& b)
{
    if(b.max.x > max.x)	max.x = b.max.x;
    if(b.min.x < min.x)	min.x = b.min.x;

    if(b.max.y > max.y)	max.y = b.max.y;
    if(b.min.y < min.y)	min.y = b.min.y;

    if(b.max.z > max.z)	max.z = b.max.z;
    if(b.min.z < min.z)	min.z = b.min.z;
}

#endif /* __cplusplus */

} /* namespace NMath */

#endif /* NMATH_BBOX_INL_INCLUDED */
