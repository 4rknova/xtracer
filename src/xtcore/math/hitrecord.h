#ifndef XTCORE_HITRECORD_H_INCLUDED
#define XTCORE_HITRECORD_H_INCLUDED

#include <stdint.h>
#include <nmath/precision.h>
#include <nmath/vector.h>
#include <nimg/color.h>
#include "ray.h"

#define HASH_UINT32 uint32_t
#define HASH_UINT64 uint64_t
#define HASH_ID     HASH_UINT64

using nmath::scalar_t;
using nmath::Vector3f;
using nimg::ColorRGBf;

namespace xtcore {

struct hit_result_t
{
    scalar_t  ior;       // Index of refraction
    ColorRGBf intensity; // Light attenuation
    Ray       ray;
};

struct hit_record_t
{
    hit_record_t();

	Vector3f normal;
	Vector3f point;
	Vector3f texcoord;
    Vector3f incident_direction;
	scalar_t t;
    scalar_t ior;
    HASH_ID  id_object;
};

} /* namespace xtcore */

#endif /* XTCORE_HITRECORD_H_INCLUDED */
