#include <cmath>
#include <cstdio>

#include <xtcore/material/boundary.h>

namespace {

int fail(const char *msg)
{
    std::fprintf(stderr, "boundary_material_test: %s\n", msg);
    return 1;
}

bool almost_equal(float a, float b, float eps = 1e-6f)
{
    return std::fabs(a - b) <= eps;
}

} // namespace

int main()
{
    xtcore::asset::material::Boundary boundary;

    xtcore::hit_record_t hit;
    hit.point = nmath::Vector3f(1.0f, 2.0f, 3.0f);
    hit.normal = nmath::Vector3f(0.0f, 0.0f, -1.0f);
    hit.incident_direction = nmath::Vector3f(0.0f, 0.0f, 1.0f);
    hit.ior = 1.33f;

    xtcore::hit_result_t next;
    if (!boundary.sample_path(next, hit)) return fail("sample_path returned false");

    if (!almost_equal(next.ray.direction.x, 0.0f) ||
        !almost_equal(next.ray.direction.y, 0.0f) ||
        !almost_equal(next.ray.direction.z, 1.0f)) {
        return fail("direction was not preserved");
    }

    if (!almost_equal(next.ior, 1.33f)) return fail("ior was not preserved");

    if (!almost_equal(next.intensity.r(), 1.0f) ||
        !almost_equal(next.intensity.g(), 1.0f) ||
        !almost_equal(next.intensity.b(), 1.0f)) {
        return fail("intensity is not unity");
    }

    if (!(next.ray.origin.z > hit.point.z)) return fail("origin was not nudged forward");

    nimg::ColorRGBf shaded(1.0f, 1.0f, 1.0f);
    if (!boundary.shade(shaded, nullptr, nullptr, hit)) return fail("shade returned false");

    if (!almost_equal(shaded.r(), 0.0f) ||
        !almost_equal(shaded.g(), 0.0f) ||
        !almost_equal(shaded.b(), 0.0f)) {
        return fail("shade did not clear intensity");
    }

    if (!boundary.bsdf_is_delta()) return fail("boundary should be delta");

    std::printf("boundary_material_test: ok\n");
    return 0;
}
