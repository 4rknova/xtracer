#include <cmath>
#include <cstdio>

#include <xtcore/math/triangle.h>

namespace {

int fail(const char *msg)
{
    std::fprintf(stderr, "triangle_test: %s\n", msg);
    return 1;
}

bool almost_equal(float a, float b, float eps = 1e-5f)
{
    return std::fabs(a - b) <= eps;
}

} // namespace

int main()
{
    xtcore::surface::Triangle t;
    t.v[0] = nmath::Vector3f(0.0f, 0.0f, 0.0f);
    t.v[1] = nmath::Vector3f(1.0f, 0.0f, 0.0f);
    t.v[2] = nmath::Vector3f(0.0f, 1.0f, 0.0f);

    t.n[0] = nmath::Vector3f(0.0f, 0.0f, 1.0f);
    t.n[1] = nmath::Vector3f(0.0f, 0.0f, 1.0f);
    t.n[2] = nmath::Vector3f(0.0f, 0.0f, 1.0f);

    t.tc[0] = nmath::Vector2f(0.0f, 0.0f);
    t.tc[1] = nmath::Vector2f(1.0f, 0.0f);
    t.tc[2] = nmath::Vector2f(0.0f, 1.0f);

    const nmath::Vector3f bc = t.calc_barycentric(nmath::Vector3f(0.25f, 0.25f, 0.0f));
    if (!almost_equal(bc.x + bc.y + bc.z, 1.0f)) return fail("barycentric sum mismatch");
    if (bc.x <= 0.0f || bc.y <= 0.0f || bc.z <= 0.0f) return fail("inside point barycentric must be positive");

    xtcore::hit_record_t hr;
    xtcore::Ray hit_ray(nmath::Vector3f(0.25f, 0.25f, -1.0f), nmath::Vector3f(0.0f, 0.0f, 1.0f));
    if (!t.intersection(hit_ray, &hr)) return fail("expected hit for inside ray");
    if (!almost_equal(hr.t, 1.0f)) return fail("unexpected hit t");
    if (!almost_equal(hr.point.x, 0.25f) || !almost_equal(hr.point.y, 0.25f) || !almost_equal(hr.point.z, 0.0f)) {
        return fail("unexpected hit point");
    }
    if (!almost_equal(hr.normal.x, 0.0f) || !almost_equal(hr.normal.y, 0.0f) || !almost_equal(hr.normal.z, 1.0f)) {
        return fail("unexpected interpolated normal");
    }

    xtcore::Ray outside_ray(nmath::Vector3f(1.2f, 1.2f, -1.0f), nmath::Vector3f(0.0f, 0.0f, 1.0f));
    if (t.intersection(outside_ray, &hr)) return fail("outside ray should miss");

    xtcore::Ray parallel_ray(nmath::Vector3f(0.25f, 0.25f, 1.0f), nmath::Vector3f(1.0f, 0.0f, 0.0f));
    if (t.intersection(parallel_ray, &hr)) return fail("parallel ray should miss");

    t.calc_aabb();
    if (!almost_equal(t.aabb.min.x, 0.0f) || !almost_equal(t.aabb.min.y, 0.0f) || !almost_equal(t.aabb.min.z, 0.0f)) {
        return fail("aabb min mismatch");
    }
    if (!almost_equal(t.aabb.max.x, 1.0f) || !almost_equal(t.aabb.max.y, 1.0f) || !almost_equal(t.aabb.max.z, 0.0f)) {
        return fail("aabb max mismatch");
    }

    std::printf("triangle_test: ok\n");
    return 0;
}
