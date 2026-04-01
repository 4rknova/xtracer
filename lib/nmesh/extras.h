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
void terrain(object_t *obj, size_t resolution = 128, int seed = 1337, const nmath::Vector3f &dimensions = nmath::Vector3f(8.0f, 1.5f, 8.0f), float noise_scale = 2.0f, size_t octaves = 5, float lacunarity = 2.0f, float gain = 0.5f);
void draped_cloth_strip(object_t *obj, size_t resolution = 96, const nmath::Vector3f &dimensions = nmath::Vector3f(2.0f, 0.9f, 3.2f), float folds = 3.0f, float edge_lift = 0.18f, float curl = 0.28f, float taper = 0.12f, float sway = 0.20f, float asymmetry = 0.0f, float pinned = 0.55f);
void chain_link(object_t *obj, size_t resolution = 64, size_t count = 6, float major_radius = 0.55f, float minor_radius = 0.16f, float spacing = 1.05f, const std::vector<nmath::Vector3f> &spline = std::vector<nmath::Vector3f>());
void lathe(object_t *obj, const std::vector<nmath::Vector2f> &profile, size_t resolution = 64, bool cap_ends = true);

    } /* namespace generator */
} /* namespace nmesh */

#endif /* NMESH_EXTRAS_H_INCLUDED */
