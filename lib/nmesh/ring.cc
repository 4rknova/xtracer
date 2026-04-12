#include <algorithm>
#include <cmath>
#include <vector>

#include <nmath/precision.h>
#include <nmath/vector.h>

#include "ring.h"

namespace nmesh {
    namespace generator {

namespace {

struct band_sample_t
{
    // Cross-section sample used by the revolve helpers.
    // `radial` and `y` describe a point in the 2D profile plane, while
    // `normal_radial` and `normal_y` describe the corresponding 2D normal
    // before it is rotated around the Y axis into 3D.
    float radial;
    float y;
    float normal_radial;
    float normal_y;
};

static void append_vertex(object_t *obj, const nmath::Vector3f &p, const nmath::Vector3f &n, float u, float v)
{
    obj->attributes.v.push_back(p.x);
    obj->attributes.v.push_back(p.y);
    obj->attributes.v.push_back(p.z);

    obj->attributes.n.push_back(n.x);
    obj->attributes.n.push_back(n.y);
    obj->attributes.n.push_back(n.z);

    obj->attributes.uv.push_back(u);
    obj->attributes.uv.push_back(v);
}

static void append_triangle(shape_t &shape, int a, int b, int c)
{
    index_t ia;
    ia.v = a; ia.n = a; ia.uv = a;
    index_t ib;
    ib.v = b; ib.n = b; ib.uv = b;
    index_t ic;
    ic.v = c; ic.n = c; ic.uv = c;

    shape.mesh.indices.push_back(ia);
    shape.mesh.indices.push_back(ib);
    shape.mesh.indices.push_back(ic);
}

template <typename SampleFn>
static void append_revolved_band(object_t *obj,
                                 shape_t &shape,
                                 size_t seg,
                                 size_t rows,
                                 float v0,
                                 float v1,
                                 const SampleFn &sample_fn)
{
    // Builds a strip by sampling a parametric cross-section band in the
    // radial/Y plane and revolving each sampled row around the Y axis.
    //
    // `rows` controls resolution along the band profile.
    // `seg` controls angular resolution around the ring.
    // `v0..v1` reserve a UV span for this band so multiple bands can be
    // stacked in one mesh without sharing the same V range.
    if (!obj || rows < 1 || seg < 3) return;

    const size_t stride = seg + 1;
    const size_t count = (rows + 1) * stride;
    std::vector<nmath::Vector3f> positions(count);
    std::vector<nmath::Vector3f> normals(count);

    for (size_t j = 0; j <= rows; ++j) {
        const float t = (rows > 0) ? ((float)j / (float)rows) : 0.0f;
        const band_sample_t s = sample_fn(t);
        for (size_t i = 0; i <= seg; ++i) {
            // Duplicate the seam column at i == seg so the UVs wrap cleanly
            // from 1 back to 0 without sharing the same vertex.
            const float u = (float)i / (float)seg;
            const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
            const float ca = nmath_cos(a);
            const float sa = nmath_sin(a);

            const size_t idx = j * stride + i;
            positions[idx] = nmath::Vector3f(s.radial * ca, s.y, s.radial * sa);
            normals[idx] = nmath::Vector3f(s.normal_radial * ca, s.normal_y, s.normal_radial * sa).normalized();
        }
    }

    const int base = (int)(obj->attributes.v.size() / 3);
    for (size_t j = 0; j <= rows; ++j) {
        const float t = (rows > 0) ? ((float)j / (float)rows) : 0.0f;
        const float v = v0 + (v1 - v0) * t;
        for (size_t i = 0; i <= seg; ++i) {
            const float u = (float)i / (float)seg;
            const size_t idx = j * stride + i;
            append_vertex(obj, positions[idx], normals[idx], u, v);
        }
    }

    for (size_t j = 0; j < rows; ++j) {
        for (size_t i = 0; i < seg; ++i) {
            const int a = base + (int)(j * stride + i);
            const int b = a + 1;
            const int c = a + (int)stride;
            const int d = c + 1;

            // The same strip helper is used for outward-facing side walls,
            // inward-facing inner walls, and top/bottom caps. Determine the
            // triangle winding from the sampled normals so indices come out
            // consistently for all of those cases.
            const nmath::Vector3f avg_n = (normals[j * stride + i]
                + normals[j * stride + i + 1]
                + normals[(j + 1) * stride + i]
                + normals[(j + 1) * stride + i + 1]).normalized();
            const nmath::Vector3f tri_n = nmath::cross(positions[j * stride + i + 1] - positions[j * stride + i],
                                                       positions[(j + 1) * stride + i + 1] - positions[j * stride + i]);
            const bool reverse = nmath::dot(tri_n, avg_n) < 0.0f;

            if (!reverse) {
                append_triangle(shape, a, b, d);
                append_triangle(shape, a, d, c);
            } else {
                append_triangle(shape, a, d, b);
                append_triangle(shape, a, c, d);
            }
        }
    }
}

static void append_revolved_profile(object_t *obj,
                                    shape_t &shape,
                                    size_t seg,
                                    const std::vector<band_sample_t> &profile)
{
    // Revolves a full closed outline instead of an isolated band.
    //
    // Unlike append_revolved_band(), the input here is an ordered contour in
    // the radial/Y plane. The helper stitches each contour segment to the
    // next one, and finally closes the loop by stitching the last sample back
    // to the first. This is used for the rounded ring so the band is built as
    // one continuous surface of revolution instead of separate top/bottom/side
    // patches.
    if (!obj || seg < 3 || profile.size() < 3) return;

    const size_t stride = seg + 1;
    const size_t rows = profile.size();
    std::vector<nmath::Vector3f> positions(rows * stride);
    std::vector<nmath::Vector3f> normals(rows * stride);
    std::vector<float> vcoords(rows, 0.0f);

    float length_accum = 0.0f;
    for (size_t j = 1; j < rows; ++j) {
        const float dr = profile[j].radial - profile[j - 1].radial;
        const float dy = profile[j].y - profile[j - 1].y;
        length_accum += std::sqrt(dr * dr + dy * dy);
        vcoords[j] = length_accum;
    }
    {
        // Include the final closing segment when normalizing V so the entire
        // closed contour maps over the full 0..1 range.
        const float dr = profile[0].radial - profile[rows - 1].radial;
        const float dy = profile[0].y - profile[rows - 1].y;
        length_accum += std::sqrt(dr * dr + dy * dy);
    }
    if (length_accum > 1e-6f) {
        for (size_t j = 0; j < rows; ++j) {
            vcoords[j] /= length_accum;
        }
    }

    for (size_t j = 0; j < rows; ++j) {
        const band_sample_t &s = profile[j];
        for (size_t i = 0; i <= seg; ++i) {
            const float u = (float)i / (float)seg;
            const float a = (float)(nmath::PI_DOUBLE * 2.0) * u;
            const float ca = nmath_cos(a);
            const float sa = nmath_sin(a);
            const size_t idx = j * stride + i;
            positions[idx] = nmath::Vector3f(s.radial * ca, s.y, s.radial * sa);
            normals[idx] = nmath::Vector3f(s.normal_radial * ca, s.normal_y, s.normal_radial * sa).normalized();
        }
    }

    const int base = (int)(obj->attributes.v.size() / 3);
    for (size_t j = 0; j < rows; ++j) {
        const float v = vcoords[j];
        for (size_t i = 0; i <= seg; ++i) {
            const float u = (float)i / (float)seg;
            const size_t idx = j * stride + i;
            append_vertex(obj, positions[idx], normals[idx], u, v);
        }
    }

    for (size_t j = 0; j < rows; ++j) {
        const size_t next_j = (j + 1) % rows;
        for (size_t i = 0; i < seg; ++i) {
            const int a = base + (int)(j * stride + i);
            const int b = a + 1;
            const int c = base + (int)(next_j * stride + i);
            const int d = c + 1;

            const nmath::Vector3f avg_n = (normals[j * stride + i]
                + normals[j * stride + i + 1]
                + normals[next_j * stride + i]
                + normals[next_j * stride + i + 1]).normalized();
            const nmath::Vector3f tri_n = nmath::cross(positions[j * stride + i + 1] - positions[j * stride + i],
                                                       positions[next_j * stride + i + 1] - positions[j * stride + i]);
            const bool reverse = nmath::dot(tri_n, avg_n) < 0.0f;

            if (!reverse) {
                append_triangle(shape, a, b, d);
                append_triangle(shape, a, d, c);
            } else {
                append_triangle(shape, a, d, b);
                append_triangle(shape, a, c, d);
            }
        }
    }
}

inline float clamp_float(float v, float lo, float hi)
{
    return std::max(lo, std::min(hi, v));
}

static void compute_ring_dimensions(float radius,
                                    float height,
                                    float thickness,
                                    float &outer_r,
                                    float &inner_r,
                                    float &half_h)
{
    // Shared dimension rules for both ring generators.
    //
    // `radius` is interpreted as the outer radius.
    // `thickness` controls wall width in the radial direction.
    // `height` is full height, stored here as half-height for convenience.
    //
    // Clamp thickness just below the outer radius so the inner radius stays
    // positive and mesh generation does not collapse.
    outer_r = (radius > 0.0f ? radius : 1.0f);
    const float default_thickness = outer_r * 0.16f;
    const float ring_thickness = (thickness > 0.0f ? thickness : default_thickness);
    const float clamped_thickness = (ring_thickness >= outer_r ? (outer_r - 0.001f) : ring_thickness);
    inner_r = (clamped_thickness > 0.0f ? (outer_r - clamped_thickness) : outer_r * 0.999f);
    const float h = (height > 0.0f ? height : 0.64f);
    half_h = h * 0.5f;
}

} /* namespace */

void ring(object_t *obj,
          size_t resolution,
          float radius,
          float height,
          float thickness,
          size_t height_resolution)
{
    if (!obj) return;

    // Flat-profile ring:
    // - one cylindrical outer wall
    // - one cylindrical inner wall
    // - flat top cap spanning inner->outer
    // - flat bottom cap spanning inner->outer
    const size_t seg = (resolution < 24 ? 24 : resolution);
    const size_t hseg = (height_resolution < 1 ? 1 : height_resolution);

    float outer_r = 1.0f;
    float inner_r = 0.84f;
    float half_h = 0.32f;
    compute_ring_dimensions(radius, height, thickness, outer_r, inner_r, half_h);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    append_revolved_band(obj, out, seg, hseg, 0.0f, 1.0f,
        [outer_r, half_h](float t) -> band_sample_t {
            band_sample_t s;
            s.radial = outer_r;
            s.y = half_h + (-2.0f * half_h) * t;
            s.normal_radial = 1.0f;
            s.normal_y = 0.0f;
            return s;
        });

    append_revolved_band(obj, out, seg, hseg, 0.0f, 1.0f,
        [inner_r, half_h](float t) -> band_sample_t {
            band_sample_t s;
            s.radial = inner_r;
            s.y = half_h + (-2.0f * half_h) * t;
            s.normal_radial = -1.0f;
            s.normal_y = 0.0f;
            return s;
        });

    append_revolved_band(obj, out, seg, 1, 0.0f, 1.0f,
        [inner_r, outer_r, half_h](float t) -> band_sample_t {
            band_sample_t s;
            s.radial = inner_r + (outer_r - inner_r) * t;
            s.y = half_h;
            s.normal_radial = 0.0f;
            s.normal_y = 1.0f;
            return s;
        });

    append_revolved_band(obj, out, seg, 1, 0.0f, 1.0f,
        [inner_r, outer_r, half_h](float t) -> band_sample_t {
            band_sample_t s;
            s.radial = inner_r + (outer_r - inner_r) * t;
            s.y = -half_h;
            s.normal_radial = 0.0f;
            s.normal_y = -1.0f;
            return s;
        });
}

void rounded_ring(object_t *obj,
                  size_t resolution,
                  float radius,
                  float height,
                  float thickness,
                  size_t profile_resolution)
{
    if (!obj) return;

    // Rounded-profile ring:
    // build a single smooth closed 2D contour in the radial/Y plane, then
    // revolve it around the Y axis. The contour is a superellipse centered
    // on the band mid-radius:
    //
    //   |x / a|^p + |y / b|^p = 1
    //
    // with x = radial - center_r, a = half wall width, and b = half height.
    //
    // Using p > 2 keeps the band fuller/flatter than a pure ellipse while
    // avoiding explicit cylindrical side segments or separate torus-like cap
    // sections in the profile.
    const size_t seg = (resolution < 32 ? 32 : resolution);
    const size_t rows = (profile_resolution < 8 ? 8 : profile_resolution);

    float outer_r = 1.0f;
    float inner_r = 0.84f;
    float half_h = 0.32f;
    compute_ring_dimensions(radius, height, thickness, outer_r, inner_r, half_h);

    const float wall_width = std::max(outer_r - inner_r, 0.001f);
    const float cap_r = std::max(0.0005f, wall_width * 0.5f);
    const float center_r = 0.5f * (outer_r + inner_r);
    const float profile_exp = 4.0f;
    const float inv_exp = 2.0f / profile_exp;

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    std::vector<band_sample_t> profile;
    profile.reserve(rows * 4);

    for (size_t i = 0; i < rows * 4; ++i) {
        const float t = (float)i / (float)(rows * 4);
        const float theta = (float)(nmath::PI_DOUBLE * 2.0) * t;
        const float ct = nmath_cos(theta);
        const float st = nmath_sin(theta);
        const float abs_ct = std::fabs(ct);
        const float abs_st = std::fabs(st);
        const float x = cap_r * std::copysign(std::pow(abs_ct, inv_exp), ct);
        const float y = half_h * std::copysign(std::pow(abs_st, inv_exp), st);

        band_sample_t s;
        s.radial = center_r + x;
        s.y = y;

        // Normal from the implicit superellipse gradient.
        const float nx = (abs_ct > 1e-6f) ? (x / (cap_r * cap_r)) * std::pow(abs_ct, 2.0f - inv_exp) : 0.0f;
        const float ny = (abs_st > 1e-6f) ? (y / (half_h * half_h)) * std::pow(abs_st, 2.0f - inv_exp) : 0.0f;
        const nmath::Vector3f n2(nx, ny, 0.0f);
        const nmath::Vector3f nn = n2.normalized();
        s.normal_radial = nn.x;
        s.normal_y = nn.y;
        profile.push_back(s);
    }

    append_revolved_profile(obj, out, seg, profile);
}

    } /* namespace generator */
} /* namespace nmesh */
