#include <cmath>
#include <cstdio>

#include <xtcore/math/sphere.h>

namespace {

int fail(const char *msg)
{
    std::fprintf(stderr, "sphere_test: %s\n", msg);
    return 1;
}

bool almost_equal(float a, float b, float eps = 1e-5f)
{
    return std::fabs(a - b) <= eps;
}

} // namespace

int main()
{
    xtcore::surface::Sphere s(nmath::Vector3f(0.0f, 0.0f, 0.0f), 1.0f);

    if (!almost_equal(s.distance(nmath::Vector3f(0.0f, 0.0f, 0.0f)), -1.0f)) {
        return fail("distance at center mismatch");
    }
    if (!almost_equal(s.distance(nmath::Vector3f(2.0f, 0.0f, 0.0f)), 1.0f)) {
        return fail("distance outside mismatch");
    }

    xtcore::Ray hit_ray(nmath::Vector3f(0.0f, 0.0f, -3.0f), nmath::Vector3f(0.0f, 0.0f, 1.0f));
    xtcore::hit_record_t hr;
    if (!s.intersection(hit_ray, &hr)) return fail("expected intersection for front ray");

    if (!almost_equal(hr.t, 2.0f)) return fail("unexpected hit distance");
    if (!almost_equal(hr.point.x, 0.0f) || !almost_equal(hr.point.y, 0.0f) || !almost_equal(hr.point.z, -1.0f)) {
        return fail("unexpected hit position");
    }
    if (!almost_equal(hr.normal.x, 0.0f) || !almost_equal(hr.normal.y, 0.0f) || !almost_equal(hr.normal.z, -1.0f)) {
        return fail("unexpected hit normal");
    }

    xtcore::Ray miss_ray(nmath::Vector3f(0.0f, 0.0f, -3.0f), nmath::Vector3f(0.0f, 1.0f, 0.0f));
    if (s.intersection(miss_ray, &hr)) return fail("unexpected intersection for miss ray");

    s.calc_aabb();
    if (!almost_equal(s.aabb.min.x, -1.0f) || !almost_equal(s.aabb.min.y, -1.0f) || !almost_equal(s.aabb.min.z, -1.0f)) {
        return fail("aabb min mismatch");
    }
    if (!almost_equal(s.aabb.max.x, 1.0f) || !almost_equal(s.aabb.max.y, 1.0f) || !almost_equal(s.aabb.max.z, 1.0f)) {
        return fail("aabb max mismatch");
    }

    xtcore::surface::Sphere fallback(nmath::Vector3f(0.0f, 0.0f, 0.0f), -5.0f);
    if (!almost_equal(static_cast<float>(fallback.radius), static_cast<float>(XTCORE_SPHERE_DEFAULT_RADIUS))) {
        return fail("negative radius did not fall back to default");
    }

    std::printf("sphere_test: ok\n");
    return 0;
}
