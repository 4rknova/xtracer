#include <cmath>
#include <algorithm>

#include "sampler_hexgrid.h"

namespace xtcore {
    namespace sampler {

namespace {

inline nmath::scalar_t clamp01(nmath::scalar_t v)
{
    return std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, v));
}

// Returns the distance from point (px,py) in hex-cell-local coordinates to the
// nearest hex edge, normalised so the hex has inradius 1.
// A flat-top hex: the 6 edge half-plane normals are at 0°, 60°, 120°, 180°, 240°, 300°.
inline nmath::scalar_t hex_edge_dist(nmath::scalar_t px, nmath::scalar_t py)
{
    // Convert to cube coordinates, take distance to hexagon boundary.
    // For a flat-topped hex with inradius r=1, max signed distance from centre
    // to any edge is 1.0.  We compute the minimum distance to any of the 6 edges.
    const nmath::scalar_t sq3o2 = (nmath::scalar_t)0.8660254037844386; // sqrt(3)/2

    // Abs symmetry to fold into 30-degree sector
    nmath::scalar_t x = (nmath::scalar_t)std::fabs((double)px);
    nmath::scalar_t y = (nmath::scalar_t)std::fabs((double)py);

    // Ensure x >= y for the pointy-top symmetry
    if (y > x) { nmath::scalar_t tmp = x; x = y; y = tmp; }

    // Distance to the flat edge at x = sq3o2  (flat-top hex, inradius sq3o2 when outer radius=1)
    // and to the angled edge
    const nmath::scalar_t d_flat  = sq3o2 - x;
    const nmath::scalar_t d_angle = (nmath::scalar_t)1.0 - (x * sq3o2 + y * (nmath::scalar_t)0.5);
    return std::min(d_flat, d_angle);
}

// Convert UV to flat-top hex grid, returning the local cell coordinate and cell index.
inline void hex_cell(nmath::scalar_t u, nmath::scalar_t v,
                     nmath::scalar_t &lx, nmath::scalar_t &ly)
{
    const nmath::scalar_t sq3 = (nmath::scalar_t)1.7320508075688772;

    // Axial coordinate conversion for flat-top hexagons (outer radius = 1)
    const nmath::scalar_t q = u * (nmath::scalar_t)(2.0 / 3.0);
    const nmath::scalar_t r = (-u / (nmath::scalar_t)3.0) + v / sq3;

    // Round to nearest hex
    const nmath::scalar_t s = -q - r;
    int rq = (int)std::round((double)q);
    int rr = (int)std::round((double)r);
    int rs = (int)std::round((double)s);
    const nmath::scalar_t dq = (nmath::scalar_t)std::fabs((double)(rq - q));
    const nmath::scalar_t dr = (nmath::scalar_t)std::fabs((double)(rr - r));
    const nmath::scalar_t ds = (nmath::scalar_t)std::fabs((double)(rs - s));
    if (dq > dr && dq > ds)      rq = -rr - rs;
    else if (dr > ds)            rr = -rq - rs;

    // Cell centre in UV space
    const nmath::scalar_t cx = (nmath::scalar_t)(3.0 / 2.0) * (nmath::scalar_t)rq;
    const nmath::scalar_t cy = sq3 * ((nmath::scalar_t)rr + (nmath::scalar_t)rq * (nmath::scalar_t)0.5);

    lx = u - cx;
    ly = v - cy;
}

} // namespace

HexGrid::HexGrid()
    : color_tile(nimg::ColorRGBf(0.20f, 0.55f, 0.60f))
    , color_border(nimg::ColorRGBf(0.10f, 0.10f, 0.10f))
    , scale((nmath::scalar_t)4.0)
    , border_width((nmath::scalar_t)0.08)
    , border_softness((nmath::scalar_t)0.02)
{}

HexGrid::~HexGrid()
{}

nimg::ColorRGBf HexGrid::sample(const nmath::Vector3f &uvw) const
{
    const nmath::scalar_t s = (std::fabs((double)scale) > 1e-6) ? scale : (nmath::scalar_t)4.0;

    nmath::scalar_t lx, ly;
    hex_cell(uvw.x * s, uvw.y * s, lx, ly);

    const nmath::scalar_t d = hex_edge_dist(lx, ly);

    // d ~ 0 at border, ~ positive inside tile
    const nmath::scalar_t bw = clamp01(border_width);
    const nmath::scalar_t bs = clamp01(border_softness) + (nmath::scalar_t)1e-6;
    const nmath::scalar_t t  = clamp01((d - bw) / bs);

    return color_border * ((nmath::scalar_t)1.0 - t) + color_tile * t;
}

    } /* namespace sampler */
} /* namespace xtcore */
