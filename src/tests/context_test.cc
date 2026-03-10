#include <cmath>
#include <cstdio>

#include <xtcore/context.h>

namespace {

int fail(const char *msg)
{
    std::fprintf(stderr, "context_test: %s\n", msg);
    return 1;
}

bool almost_equal(float a, float b, float eps = 1e-6f)
{
    return std::fabs(a - b) <= eps;
}

} // namespace

int main()
{
    xtcore::render::context_t ctx;
    ctx.params.width = 5;
    ctx.params.height = 4;
    ctx.params.tile_size = 3;
    ctx.init();

    if (ctx.tiles.size() != 4) return fail("unexpected tile count for context init");

    for (size_t i = 0; i < ctx.tiles.size(); ++i) {
        xtcore::render::tile_t &tile = ctx.tiles[i];
        tile.init();
        for (size_t y = tile.y0(); y < tile.y1(); ++y) {
            for (size_t x = tile.x0(); x < tile.x1(); ++x) {
                tile.write(x, y, nimg::ColorRGBAf(static_cast<float>(x), static_cast<float>(y), 0.25f, 1.0f));
            }
        }
    }

    nimg::Pixmap out;
    xtcore::render::assemble(out, ctx);

    if (out.width() != 5 || out.height() != 4) return fail("assembled framebuffer dimensions mismatch");

    for (size_t y = 0; y < 4; ++y) {
        for (size_t x = 0; x < 5; ++x) {
            const nimg::ColorRGBAf p = out.pixel(x, y);
            if (!almost_equal(p.r(), static_cast<float>(x)) ||
                !almost_equal(p.g(), static_cast<float>(y)) ||
                !almost_equal(p.b(), 0.25f) ||
                !almost_equal(p.a(), 1.0f)) {
                return fail("assembled pixel mismatch");
            }
        }
    }

    xtcore::raygraph::raygraph_t rg;
    xtcore::render::assemble(rg, ctx);
    if (rg.bundles.size() != ctx.tiles.size()) return fail("raygraph bundle count mismatch");
    for (size_t i = 0; i < rg.bundles.size(); ++i) {
        if (rg.bundles[i] != &(ctx.tiles[i].raygraph_bundle)) return fail("raygraph bundle pointer mismatch");
    }

    std::printf("context_test: ok\n");
    return 0;
}
