#include <cmath>
#include <cstdio>

#include <xtcore/math/csg.h>
#include <xtcore/math/sphere.h>

namespace {

int fail(const char *msg)
{
    std::fprintf(stderr, "csg_test: %s\n", msg);
    return 1;
}

bool almost_equal(float a, float b, float eps = 1e-3f)
{
    return std::fabs(a - b) <= eps;
}

xtcore::surface::CSG *make_union_two_spheres()
{
    xtcore::surface::CSG *u = new xtcore::surface::CSG();
    u->op = xtcore::surface::CSG::OP_UNION;

    xtcore::surface::Sphere *a = new xtcore::surface::Sphere();
    a->origin = nmath::Vector3f(-0.8f, 0.0f, 0.0f);
    a->radius = 1.0f;
    a->calc_aabb();

    xtcore::surface::Sphere *b = new xtcore::surface::Sphere();
    b->origin = nmath::Vector3f(0.8f, 0.0f, 0.0f);
    b->radius = 1.0f;
    b->calc_aabb();

    u->left = a;
    u->right = b;
    u->calc_aabb();
    return u;
}

} // namespace

int main()
{
    {
        xtcore::surface::CSG *u = make_union_two_spheres();
        const float d_center = (float)u->distance(nmath::Vector3f(0.0f, 0.0f, 0.0f));
        if (!(d_center < 0.0f)) {
            delete u;
            return fail("union distance should be negative inside");
        }
        if (!almost_equal((float)u->aabb.min.x, -1.8f) || !almost_equal((float)u->aabb.max.x, 1.8f)) {
            delete u;
            return fail("union aabb mismatch");
        }

        xtcore::hit_record_t hr;
        xtcore::Ray ray(nmath::Vector3f(0.0f, 0.0f, -3.0f), nmath::Vector3f(0.0f, 0.0f, 1.0f));
        if (!u->intersection(ray, &hr)) {
            delete u;
            return fail("union expected intersection");
        }
        if (!(hr.t > 0.0f)) {
            delete u;
            return fail("union hit t must be positive");
        }
        delete u;
    }

    {
        xtcore::surface::CSG inter;
        inter.op = xtcore::surface::CSG::OP_INTERSECTION;

        xtcore::surface::Sphere *a = new xtcore::surface::Sphere();
        a->origin = nmath::Vector3f(-1.2f, 0.0f, 0.0f);
        a->radius = 1.0f;
        a->calc_aabb();

        xtcore::surface::Sphere *b = new xtcore::surface::Sphere();
        b->origin = nmath::Vector3f(1.2f, 0.0f, 0.0f);
        b->radius = 1.0f;
        b->calc_aabb();

        inter.left = a;
        inter.right = b;
        inter.calc_aabb();

        xtcore::hit_record_t hr;
        xtcore::Ray ray(nmath::Vector3f(0.0f, 0.0f, -3.0f), nmath::Vector3f(0.0f, 0.0f, 1.0f));
        if (inter.intersection(ray, &hr)) return fail("disjoint intersection should miss");
    }

    {
        xtcore::surface::CSG diff;
        diff.op = xtcore::surface::CSG::OP_DIFFERENCE;

        xtcore::surface::Sphere *a = new xtcore::surface::Sphere();
        a->origin = nmath::Vector3f(0.0f, 0.0f, 0.0f);
        a->radius = 1.0f;
        a->calc_aabb();

        xtcore::surface::Sphere *b = new xtcore::surface::Sphere();
        b->origin = nmath::Vector3f(0.55f, 0.0f, 0.0f);
        b->radius = 0.7f;
        b->calc_aabb();

        diff.left = a;
        diff.right = b;
        diff.calc_aabb();

        const float d_inside_a_outside_b = (float)diff.distance(nmath::Vector3f(-0.6f, 0.0f, 0.0f));
        if (!(d_inside_a_outside_b < 0.0f)) return fail("difference expected interior in retained region");

        const float d_subtracted = (float)diff.distance(nmath::Vector3f(0.55f, 0.0f, 0.0f));
        if (!(d_subtracted > 0.0f)) return fail("difference expected exterior in subtracted region");
    }

    std::printf("csg_test: ok\n");
    return 0;
}
