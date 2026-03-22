#include <cmath>
#include <cstdio>
#include <unistd.h>

#include <nimg/conversion.h>
#include <nimg/img.h>
#include <nimg/pixmap.h>

namespace {

bool almost_equal(float a, float b, float eps)
{
    return std::fabs(a - b) <= eps;
}

int fail(const char *msg)
{
    std::fprintf(stderr, "colorspace_vectors_test: %s\n", msg);
    return 1;
}

} // namespace

int main()
{
    if (!almost_equal(srgb_to_linear(0.04045f), 0.0031308f, 1e-6f)) {
        return fail("sRGB decode knee mismatch");
    }
    if (!almost_equal(linear_to_srgb(0.0031308f), 0.04045f, 1e-6f)) {
        return fail("sRGB encode knee mismatch");
    }

    nimg::Pixmap in;
    if (in.init(2, 2) != 0) return fail("failed to init pixmap");

    in.pixel(0, 0) = nimg::ColorRGBAf(0.10f, 0.30f, 0.70f, 0.25f);
    in.pixel(1, 0) = nimg::ColorRGBAf(0.90f, 0.20f, 0.10f, 0.50f);
    in.pixel(0, 1) = nimg::ColorRGBAf(0.40f, 0.60f, 0.20f, 0.75f);
    in.pixel(1, 1) = nimg::ColorRGBAf(0.95f, 0.95f, 0.95f, 1.00f);

    char path[] = "/tmp/xtracer_colorspace_vectors_XXXXXX.png";
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

    if (out.width() != 2 || out.height() != 2) {
        return fail("round-trip dimensions mismatch");
    }

    const float rgb_eps = 1.0f / 255.0f + 2e-3f;
    const float a_eps = 1.0f / 255.0f + 1e-6f;

    for (size_t y = 0; y < 2; ++y) {
        for (size_t x = 0; x < 2; ++x) {
            const nimg::ColorRGBAf src = in.pixel(x, y);
            const nimg::ColorRGBAf dst = out.pixel(x, y);

            if (!almost_equal(src.r(), dst.r(), rgb_eps) ||
                !almost_equal(src.g(), dst.g(), rgb_eps) ||
                !almost_equal(src.b(), dst.b(), rgb_eps)) {
                return fail("RGB round-trip mismatch");
            }
            if (!almost_equal(src.a(), dst.a(), a_eps)) {
                return fail("alpha round-trip mismatch");
            }
        }
    }

    std::printf("colorspace_vectors_test: ok\n");
    return 0;
}
