#ifndef NMESH_EXTRAS_H_INCLUDED
#define NMESH_EXTRAS_H_INCLUDED

#include <vector>
#include <nmath/vector.h>

#include "structs.h"

namespace nmesh {
    namespace generator {

void capsule(object_t *obj, size_t resolution = 32);
void cylinder(object_t *obj, size_t resolution = 32);
void capped_cylinder(object_t *obj, size_t resolution = 32);
void cone(object_t *obj, size_t resolution = 32);
void truncated_cone(object_t *obj, size_t resolution = 32);
void torus_knot(object_t *obj, size_t resolution = 48);
void icosphere(object_t *obj, size_t resolution = 32);
void geodesic_dome(object_t *obj, size_t resolution = 32);
void icosa_cage(object_t *obj, size_t resolution = 32);
void menger_sponge(object_t *obj, size_t resolution = 2);
void menger_sponge_implicit(object_t *obj, size_t resolution = 2);
void sierpinski_tetrahedron(object_t *obj, size_t resolution = 2);
void sierpinski_tetrahedron_implicit(object_t *obj, size_t resolution = 2);
void mobius_strip(object_t *obj, size_t resolution = 64);
void klein_bottle(object_t *obj, size_t resolution = 64);
void hairball(object_t *obj, size_t resolution = 32, int seed = 1337, float radius = 1.0f, size_t fibers = 0);
void shell_spiral(object_t *obj, size_t resolution = 64, float turns = 4.0f, float growth = 0.22f, float tube_radius = 0.14f);
void rock(object_t *obj, size_t resolution = 48, int seed = 1337, float radius = 1.0f, float roughness = 0.35f, size_t octaves = 4);
void chain_link(object_t *obj, size_t resolution = 64, size_t count = 6, float major_radius = 0.55f, float minor_radius = 0.16f, float spacing = 1.05f, const std::vector<nmath::Vector3f> &spline = std::vector<nmath::Vector3f>());
void lathe(object_t *obj, const std::vector<nmath::Vector2f> &profile, size_t resolution = 64, bool cap_ends = true);

    } /* namespace generator */
} /* namespace nmesh */

#endif /* NMESH_EXTRAS_H_INCLUDED */
