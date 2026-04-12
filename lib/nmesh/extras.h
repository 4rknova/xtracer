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
void mobius_strip(object_t *obj, size_t resolution = 64, float radius = 1.0f, float width = 0.64f);
void klein_bottle(object_t *obj, size_t resolution = 64);
void hairball(object_t *obj, size_t resolution = 32, int seed = 1337, float radius = 1.0f, size_t fibers = 0);
void shell_spiral(object_t *obj, size_t resolution = 64, float turns = 4.0f, float growth = 0.22f, float tube_radius = 0.14f);
void rock(object_t *obj, size_t resolution = 48, int seed = 1337, float radius = 1.0f, float roughness = 0.35f, size_t octaves = 4);
void terrain(object_t *obj, size_t resolution = 128, int seed = 1337, const nmath::Vector3f &dimensions = nmath::Vector3f(8.0f, 1.5f, 8.0f), float noise_scale = 2.0f, size_t octaves = 5, float lacunarity = 2.0f, float gain = 0.5f);
void draped_cloth_strip(object_t *obj, size_t resolution = 96, const nmath::Vector3f &dimensions = nmath::Vector3f(2.0f, 0.9f, 3.2f), float folds = 3.0f, float edge_lift = 0.18f, float curl = 0.28f, float taper = 0.12f, float sway = 0.20f, float asymmetry = 0.0f, float pinned = 0.55f);
void chain_link(object_t *obj, size_t resolution = 64, size_t count = 6, float major_radius = 0.55f, float minor_radius = 0.16f, float spacing = 1.05f, const std::vector<nmath::Vector3f> &spline = std::vector<nmath::Vector3f>());
void lathe(object_t *obj, const std::vector<nmath::Vector2f> &profile, size_t resolution = 64, bool cap_ends = true);
void gear(object_t *obj, size_t resolution = 32, size_t tooth_count = 12, float tooth_depth = 0.1f, float inner_radius = 0.2f, float outer_radius = 0.5f, float height = 0.2f);
void spring(object_t *obj, size_t resolution = 32, float coils = 6.0f, float wire_radius = 0.05f, float spring_radius = 0.3f, float height = 1.2f);
void hemisphere(object_t *obj, size_t resolution = 32);
void disc(object_t *obj, size_t resolution = 32, float inner_radius = 0.0f, float outer_radius = 1.0f);
void star(object_t *obj, size_t resolution = 32, size_t points = 5, float inner_radius = 0.4f, float outer_radius = 1.0f, float height = 0.2f);
void superellipsoid(object_t *obj, size_t resolution = 32, float e1 = 1.0f, float e2 = 1.0f);
void crystal(object_t *obj, size_t resolution = 32, size_t count = 5, float radius = 0.8f, float height = 1.5f, float tip_height = 0.6f, int seed = 1337);
void tree(object_t *obj, size_t resolution = 32, int depth = 4, int branch_count = 3, float branch_angle = 0.6f, float trunk_height = 1.2f, float trunk_radius = 0.08f, int seed = 1337);
void coral(object_t *obj, size_t resolution = 32, int depth = 4, int branch_count = 4, float branch_angle = 0.7f, float height = 1.0f, float branch_radius = 0.05f, int seed = 1337);

    } /* namespace generator */
} /* namespace nmesh */

#endif /* NMESH_EXTRAS_H_INCLUDED */
