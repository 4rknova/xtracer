#include <cmath>
#include <cstdio>

#include <nmath/sample.h>
#include <xtcore/math/sampling_util.h>

namespace {

bool closef(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

int fail(const char *msg)
{
    std::fprintf(stderr, "sampling_test: %s\n", msg);
    return 1;
}

} // namespace

int main()
{
    const nmath::Vector3f up(0.0f, 1.0f, 0.0f);

    {
        // Sphere samples should be normalized and roughly centered.
        nmath::Vector3f mean(0.0f, 0.0f, 0.0f);
        for (int i = 0; i < 20000; ++i) {
            const nmath::Vector3f s = nmath::sample::sphere();
            const float len = s.length();
            if (!closef(len, 1.0f, 1e-3f)) return fail("sphere sample not normalized");
            mean += s;
        }
        mean /= 20000.0f;
        if (std::fabs(mean.x) > 0.03f || std::fabs(mean.y) > 0.03f || std::fabs(mean.z) > 0.03f) {
            return fail("sphere sample mean drift too large");
        }
    }

    {
        // Hemisphere and diffuse helpers should stay above the requested normal.
        for (int i = 0; i < 20000; ++i) {
            const nmath::Vector3f h = nmath::sample::hemisphere(up, up);
            const nmath::Vector3f d = nmath::sample::diffuse(up);
            if (nmath::dot(h, up) < -1e-5f) return fail("hemisphere sample below normal");
            if (nmath::dot(d, up) < -1e-5f) return fail("diffuse sample below normal");
            if (!closef(h.length(), 1.0f, 1e-3f)) return fail("hemisphere sample not normalized");
            if (!closef(d.length(), 1.0f, 1e-3f)) return fail("diffuse sample not normalized");
        }
    }

    {
        // Tangent should be near unit and orthogonal.
        const nmath::Vector3f t = xtcore::math::sampling::build_tangent(up);
        if (!closef(t.length(), 1.0f, 1e-3f)) return fail("build_tangent not normalized");
        if (std::fabs(nmath::dot(t, up)) > 1e-3f) return fail("build_tangent not orthogonal");
    }

    {
        // Cosine hemisphere sampling: pdf must match cos(theta)/pi.
        for (int i = 0; i < 20000; ++i) {
            nmath::scalar_t pdf = 0.0;
            const nmath::Vector3f w = xtcore::math::sampling::sample_cosine_hemisphere(up, pdf);
            const nmath::scalar_t c = std::max((nmath::scalar_t)0.0, nmath::dot(up, w));
            const nmath::scalar_t expected = c / nmath::PI;
            if (nmath::dot(up, w) < -1e-5f) return fail("cosine hemisphere sample below normal");
            if (!closef((float)w.length(), 1.0f, 1e-3f)) return fail("cosine hemisphere sample not normalized");
            if (!closef((float)pdf, (float)expected, 1e-4f)) return fail("cosine hemisphere pdf mismatch");
        }
    }

    {
        // Power-cosine lobe sampling: pdf must match definition.
        const nmath::scalar_t exp = 16.0;
        for (int i = 0; i < 20000; ++i) {
            nmath::scalar_t pdf = 0.0;
            const nmath::Vector3f w = xtcore::math::sampling::sample_power_cosine_lobe(up, exp, pdf);
            const nmath::scalar_t c = std::max((nmath::scalar_t)0.0, nmath::dot(up, w));
            const nmath::scalar_t expected = ((exp + (nmath::scalar_t)1.0) / ((nmath::scalar_t)2.0 * nmath::PI))
                                           * nmath_pow(c, exp);
            if (nmath::dot(up, w) < -1e-5f) return fail("power cosine lobe sample below axis");
            if (!closef((float)w.length(), 1.0f, 1e-3f)) return fail("power cosine lobe sample not normalized");
            if (!closef((float)pdf, (float)expected, 1e-4f)) return fail("power cosine lobe pdf mismatch");
        }
    }

    std::printf("sampling_test: ok\n");
    return 0;
}
