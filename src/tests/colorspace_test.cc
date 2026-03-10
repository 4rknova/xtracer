#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <nimg/pixmap.h>
#include <nimg/img.h>
#include <nimg/conversion.h>

namespace {

bool almost_equal(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

int fail(const char *msg)
{
    std::fprintf(stderr, "colorspace_test: %s\n", msg);
    return 1;
}

} // namespace

int main()
{
    if (!almost_equal(srgb_to_linear(0.5f), 0.21404114f, 1e-5f)) {
        return fail("srgb_to_linear(0.5) mismatch");
    }

    if (!almost_equal(linear_to_srgb(0.21404114f), 0.5f, 1e-5f)) {
        return fail("linear_to_srgb inverse mismatch");
    }

    nimg::Pixmap in;
    if (in.init(4, 1) != 0) return fail("failed to init input pixmap");

    const float ramp[4] = {0.0f, 0.18f, 0.5f, 1.0f};
    for (size_t x = 0; x < 4; ++x) {
        in.pixel(x, 0) = nimg::ColorRGBAf(ramp[x], ramp[x], ramp[x], 1.0f);
    }

    char path[] = "/tmp/xtracer_colorspace_test_XXXXXX.png";
    int fd = mkstemps(path, 4);
    if (fd < 0) return fail("failed to create temporary png path");
    close(fd);

    if (nimg::io::save::png(path, in) != 0) {
        unlink(path);
        return fail("failed to save png");
    }

    nimg::Pixmap out;
    if (nimg::io::load::image(path, out) != 0) {
        unlink(path);
        return fail("failed to load png");
    }
    unlink(path);

    if (out.width() != 4 || out.height() != 1) {
        return fail("round-trip dimensions mismatch");
    }

    const float eps = 1.0f / 255.0f + 1e-3f;
    for (size_t x = 0; x < 4; ++x) {
        const nimg::ColorRGBAf p = out.pixel(x, 0);
        if (!almost_equal(p.r(), ramp[x], eps) ||
            !almost_equal(p.g(), ramp[x], eps) ||
            !almost_equal(p.b(), ramp[x], eps)) {
            return fail("linear round-trip mismatch");
        }
        if (!almost_equal(p.a(), 1.0f, 1e-6f)) {
            return fail("alpha round-trip mismatch");
        }
    }

    std::printf("colorspace_test: ok\n");
    return 0;
}
