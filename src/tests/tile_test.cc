#include <algorithm>
#include <cmath>
#include <cstdio>

#include <xtcore/tile.h>

namespace {

int fail(const char *msg)
{
    std::fprintf(stderr, "tile_test: %s\n", msg);
    return 1;
}

bool almost_equal(float a, float b, float eps = 1e-6f)
{
    return std::fabs(a - b) <= eps;
}

int radial_score(const xtcore::render::tile_t &tile, int w, int h)
{
    const int cx = static_cast<int>(tile.x0() + tile.width() / 2);
    const int cy = static_cast<int>(tile.y0() + tile.height() / 2);
    const int dx = cx - (w / 2);
    const int dy = cy - (h / 2);
    return dx * dx + dy * dy;
}

} // namespace

int main()
{
    xtcore::render::Tileset tiles;
    xtcore::render::segment_framebuffer(tiles, 100, 70, 32);

    if (tiles.size() != 12) return fail("unexpected tile count for 100x70@32");

    const xtcore::render::tile_t &first = tiles.front();
    if (first.x0() != 0 || first.y0() != 0 || first.x1() != 32 || first.y1() != 32) {
        return fail("first tile bounds mismatch");
    }

    const xtcore::render::tile_t &last = tiles.back();
    if (last.x0() != 96 || last.y0() != 64 || last.x1() != 100 || last.y1() != 70) {
        return fail("last tile bounds mismatch");
    }

    size_t total_area = 0;
    for (size_t i = 0; i < tiles.size(); ++i) {
        const xtcore::render::tile_t &t = tiles[i];
        if (t.x0() >= t.x1() || t.y0() >= t.y1()) return fail("degenerate tile");
        if (t.x1() > 100 || t.y1() > 70) return fail("tile exceeds framebuffer bounds");
        total_area += t.width() * t.height();
    }
    if (total_area != 100 * 70) return fail("tile area coverage mismatch");

    xtcore::render::order_scanline(tiles);
    for (size_t i = 1; i < tiles.size(); ++i) {
        const xtcore::render::tile_t &a = tiles[i - 1];
        const xtcore::render::tile_t &b = tiles[i];
        if (a.y0() > b.y0()) return fail("scanline order broke y monotonicity");
        if (a.y0() == b.y0() && a.x0() > b.x0()) return fail("scanline order broke x monotonicity");
    }

    xtcore::render::Tileset radial_tiles;
    const int w = 96;
    const int h = 96;
    xtcore::render::segment_framebuffer(radial_tiles, static_cast<size_t>(w), static_cast<size_t>(h), 32);
    xtcore::render::order_radial(radial_tiles, true);

    int prev = radial_score(radial_tiles[0], w, h);
    for (size_t i = 1; i < radial_tiles.size(); ++i) {
        const int now = radial_score(radial_tiles[i], w, h);
        if (now < prev) return fail("radial-out order is not nondecreasing by distance");
        prev = now;
    }

    if (!almost_equal(static_cast<float>(total_area), 7000.0f)) {
        return fail("sanity check failed");
    }

    std::printf("tile_test: ok\n");
    return 0;
}
