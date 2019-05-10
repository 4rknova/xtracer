#ifndef XTCORE_BBOX_H_INCLUDED
#define XTCORE_BBOX_H_INCLUDED

#include <nmath/defs.h>
#include <nmath/types.h>
#include <nmath/vector.h>
#include "ray.h"

using NMath::Vector2f;
using NMath::Vector3f;

namespace xtcore {

class AABB2
{
public:
    AABB2();
    AABB2(const Vector2f& a, const Vector2f& b);

    inline bool contains(const Vector2f& p) const; // returns true if the given point is within the bounds of the box, else false
    inline Vector2f center() const;                // returns the center coordinates of the box

    inline void augment(const Vector2f& v);        // augments the bounding box to include the given vector
    inline void augment(const AABB2& b);    // augments the bounding box to include the given bounding box

    Vector2f min, max;
};

class AABB3
{
public:
    AABB3();
    AABB3(const Vector3f& a, const Vector3f& b);

    inline bool contains(const Vector3f& p) const;        // returns true if the given point is within the bounds of the box, else false
	inline bool contains(const AABB3 &aabb) const;
    inline Vector3f center() const;                       // returns the center coordinates of the box

    inline void augment(const Vector3f& v);               // augments the bounding box to include the given vector
    inline void augment(const AABB3& b);           // augments the bounding box to include the given bounding box

	bool intersection(const Ray &ray) const;

    Vector3f min, max;
};

} /* namespace xtcore */

#include "aabb.inl"

#endif /* XTCORE_BBOX_H_INCLUDED */
