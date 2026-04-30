#include <cmath>
#include <algorithm>

#include "sampler_scalegrid.h"

namespace xtcore {
    namespace sampler {

namespace {

// Seven-segment digit table.
// Bit encoding: a=1 b=2 c=4 d=8 e=16 f=32 g=64
// Segment layout in normalized [0,1]x[0,1] glyph space (v=0 bottom, v=1 top):
//
//   aaa     ← top horizontal
//  f   b
//  f   b    ← left/right verticals, top half
//   ggg     ← middle horizontal
//  e   c
//  e   c    ← left/right verticals, bottom half
//   ddd     ← bottom horizontal
//
// Index 10 = minus sign (segment g only).
static const int SEG_TABLE[11] = {
     63, // 0  abcdef
      6, // 1  bc
     91, // 2  abdeg
     79, // 3  abcdg
    102, // 4  bcfg
    109, // 5  acdfg
    125, // 6  acdefg
      7, // 7  abc
    127, // 8  abcdefg
    111, // 9  abcdfg
     64, // -  g
};

// Returns smooth coverage [0,1] for the given digit at glyph-space (u,v).
// Uses SDF-style rect coverage so edges are anti-aliased rather than hard-clipped.
static float seg7_coverage(int digit, float u, float v)
{
    if (digit < 0 || digit > 10) return 0.f;
    const int   segs = SEG_TABLE[digit];
    const float s    = 0.20f;             // arm thickness (wider = more legible)
    const float ms   = 0.50f - s * 0.5f; // middle-seg lower boundary
    const float Mt   = 0.50f + s * 0.5f; // middle-seg upper boundary
    const float aa   = 0.05f;            // anti-aliasing half-width in glyph space

    // Signed distance from interior of rect [u0,u1]x[v0,v1]; positive = inside.
    // Coverage ramps 0→1 over aa on each edge.
    auto rc = [&](float u0, float u1, float v0, float v1) -> float {
        const float cu = std::min(u - u0, u1 - u);
        const float cv = std::min(v - v0, v1 - v);
        return std::max(0.f, std::min(1.f, 0.5f + std::min(cu, cv) / aa));
    };

    float c = 0.f;
    if (segs &  1) c = std::max(c, rc(s,   1.f-s, 1.f-s, 1.f  )); // a top
    if (segs &  2) c = std::max(c, rc(1.f-s, 1.f, Mt,   1.f-s )); // b top-right
    if (segs &  4) c = std::max(c, rc(1.f-s, 1.f, s,    ms    )); // c bot-right
    if (segs &  8) c = std::max(c, rc(s,   1.f-s, 0.f,  s     )); // d bottom
    if (segs & 16) c = std::max(c, rc(0.f,  s,    s,    ms    )); // e bot-left
    if (segs & 32) c = std::max(c, rc(0.f,  s,    Mt,   1.f-s )); // f top-left
    if (segs & 64) c = std::max(c, rc(s,   1.f-s, ms,   Mt    )); // g middle
    return c;
}

// Returns maximum segment coverage across all glyphs in an integer label.
// (fu, fv): sample position within cell [0,1]
// (ox, oy): bottom-left origin of the label box in cell space
// ts: glyph height as fraction of cell
// val: absolute (non-negative) integer value
// negative: prepend a minus glyph
static float render_int_label(float fu, float fv,
                              float ox, float oy, float ts,
                              int val, bool negative)
{
    const float aspect    = 0.60f;        // glyph width/height ratio (wider = more legible)
    const float char_w    = ts * aspect;
    const float char_gap  = char_w * 0.20f;
    const float char_step = char_w + char_gap;

    int ndigits = 0;
    int tmp = val;
    do { ndigits++; tmp /= 10; } while (tmp > 0);
    const int nchars = ndigits + (negative ? 1 : 0);

    const float lu = fu - ox;
    const float lv = fv - oy;

    if (lu < 0.f || lv < 0.f || lv > ts) return 0.f;

    const int cidx = (int)(lu / char_step);
    if (cidx >= nchars) return 0.f;

    const float cu = (lu - cidx * char_step) / char_w;
    const float cv = lv / ts;
    if (cu < 0.f || cu > 1.f) return 0.f;

    int digit;
    if (negative && cidx == 0) {
        digit = 10; // minus
    } else {
        const int d = cidx - (negative ? 1 : 0);
        int power = 1;
        for (int i = 0; i < ndigits - 1 - d; i++) power *= 10;
        digit = (val / power) % 10;
    }

    return seg7_coverage(digit, cu, cv);
}

} // namespace

ScaleGrid::ScaleGrid()
    : base_color(nimg::ColorRGBf(0.93f, 0.93f, 0.93f))
    , line_color(nimg::ColorRGBf(0.50f, 0.60f, 0.70f))
    , text_color(nimg::ColorRGBf(0.18f, 0.18f, 0.18f))
    , scale((nmath::scalar_t)8.0)
    , line_width((nmath::scalar_t)0.040)
    , text_size((nmath::scalar_t)0.28)
{}

ScaleGrid::~ScaleGrid()
{}

nimg::ColorRGBf ScaleGrid::sample(const nmath::Vector3f &uvw) const
{
    const float s  = ((float)scale > 1e-6f) ? (float)scale : 8.0f;
    const float u  = (float)uvw.x * s;
    const float v  = (float)uvw.y * s;
    const int   iu = (int)std::floor((double)u);
    const int   iv = (int)std::floor((double)v);
    const float fu = u - (float)iu;
    const float fv = v - (float)iv;

    // Grid lines
    const float hw = std::max(0.005f, std::min(0.45f, (float)line_width)) * 0.5f;
    if (fu < hw || fu > 1.f - hw || fv < hw || fv > 1.f - hw)
        return line_color;

    const float ts     = std::max(0.05f, std::min(0.40f, (float)text_size));
    const float margin = hw * 2.f + 0.025f;

    float cov = 0.f;
    cov = std::max(cov, render_int_label(fu, fv, margin, margin,           ts, (iu < 0) ? -iu : iu, iu < 0));
    cov = std::max(cov, render_int_label(fu, fv, margin, 1.f - margin - ts, ts, (iv < 0) ? -iv : iv, iv < 0));

    if (cov <= 0.f) return base_color;

    return base_color * (1.f - cov) + text_color * cov;
}

    } /* namespace sampler */
} /* namespace xtcore */
