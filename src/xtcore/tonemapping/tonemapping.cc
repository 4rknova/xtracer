// -----------------------------------------------------------------------------
// Tone mapping operators
//
// All operators work in linear light (scene-referred). The output is also
// linear and still in floating-point; sRGB gamma encoding is applied later
// by the image-save layer (pack_rgba8_srgb in nimg/img.cc).
//
// Exposure scaling (settings.exposure) is applied before any operator.
// -----------------------------------------------------------------------------

#include <algorithm>
#include <cmath>
#include <vector>
#include <nimg/luminance.h>

#include "tonemapping.h"

namespace xtcore {
namespace tonemapping {

namespace {

inline float clamp01(float v)
{
    return std::max(0.0f, std::min(1.0f, v));
}

// =============================================================================
// Reinhard extended (global, per-channel)
//
// Extended Reinhard operator from:
//   E. Reinhard, M. Stark, P. Shirley, J. Ferwerda
//   "Photographic Tone Reproduction for Digital Images"
//   SIGGRAPH 2002, eq. (4)
//   https://doi.org/10.1145/566570.566575
//
// T(x) = x * (1 + x/w²) / (1 + x)
//
// where w is the white_point (any input >= w maps to 1 exactly).
// When white_point <= 0 the simpler T(x) = x/(1+x) is used (w → ∞).
// Applied per-channel; see apply_reinhard_luminance for the hue-preserving
// luminance variant.
// =============================================================================
inline float reinhard_extended(float x, float white_point)
{
    if (white_point <= 0.0f) return x / (1.0f + x);
    const float w2 = white_point * white_point;
    return (x * (1.0f + x / w2)) / (1.0f + x);
}

inline nimg::ColorRGBf apply_reinhard(const nimg::ColorRGBf &c, float white_point)
{
    return nimg::ColorRGBf(
          reinhard_extended(c.r(), white_point)
        , reinhard_extended(c.g(), white_point)
        , reinhard_extended(c.b(), white_point)
    );
}

// =============================================================================
// Reinhard luminance (global, hue-preserving)
//
// Same reference as above. Maps luminance through the Reinhard curve and
// scales all channels by the same ratio, preserving the original hue.
// Avoids the hue shift of the per-channel variant at the cost of potentially
// leaving individual channels above 1 before the final clamp.
// =============================================================================
inline nimg::ColorRGBf apply_reinhard_luminance(const nimg::ColorRGBf &c, float white_point)
{
    const float l = nimg::eval::luminance(c);
    if (l <= 0.0f) return nimg::ColorRGBf(0, 0, 0);
    const float mapped_l = reinhard_extended(l, white_point);
    return c * (mapped_l / l);
}

// =============================================================================
// Hable / Uncharted 2 filmic
//
// John Hable, "Filmic Tonemapping Operators", blog 2010
//   http://filmicworlds.com/blog/filmic-tonemapping-operators/
// Originally presented in:
//   J. Hable, "Uncharted 2: HDR Lighting", GDC 2010
//
// The curve is a piecewise rational polynomial with toe, linear, and shoulder
// segments controlled by six parameters (A-F). The normalised result is
// hable_f(x * exposure) / hable_f(W), where W is the scene linear white
// point. The canonical Uncharted 2 value is W = 11.2; if settings.white_point
// is at its default of 1.0 (or any value <= 1.0) that value is used for all
// other operators, but Hable substitutes 11.2 automatically since 1.0 yields
// a near-identity mapping for that curve shape.
//
// Parameter roles:
//   A = shoulder strength   C = linear angle   E = toe numerator
//   B = linear strength     D = toe strength   F = toe denominator
// =============================================================================
inline float hable_f(float x)
{
    const float A = 0.15f;
    const float B = 0.50f;
    const float C = 0.10f;
    const float D = 0.20f;
    const float E = 0.02f;
    const float F = 0.30f;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

inline nimg::ColorRGBf apply_hable(const nimg::ColorRGBf &c, float white_point)
{
    // Use 11.2 as the default white point; only use the user value when it
    // is explicitly set above 1.0 (the settings_t default).
    const float W     = (white_point > 1.0f) ? white_point : 11.2f;
    const float inv_W = 1.0f / hable_f(W);
    return nimg::ColorRGBf(
          clamp01(hable_f(std::max(0.0f, c.r())) * inv_W)
        , clamp01(hable_f(std::max(0.0f, c.g())) * inv_W)
        , clamp01(hable_f(std::max(0.0f, c.b())) * inv_W)
    );
}

// =============================================================================
// ACES fitted curve (Narkowicz approximation)
//
// A rational polynomial fit to the ACES RRT+ODT filmic curve by Krzysztof
// Narkowicz:
//   "ACES Filmic Tone Mapping Curve"
//   https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
//
// f(x) = (x * (2.51x + 0.03)) / (x * (2.43x + 0.59) + 0.14)
//
// The denominator minimum is 0.14 (at x = 0), so there is no division by
// zero. Applied per-channel; white_point is not used — the ACES curve has
// its own implicit roll-off baked into the coefficients.
// =============================================================================
inline nimg::ColorRGBf apply_aces_fitted(const nimg::ColorRGBf &c)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c1 = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;

    const float r   = (c.r() * (a * c.r() + b)) / (c.r() * (c1 * c.r() + d) + e);
    const float g   = (c.g() * (a * c.g() + b)) / (c.g() * (c1 * c.g() + d) + e);
    const float bch = (c.b() * (a * c.b() + b)) / (c.b() * (c1 * c.b() + d) + e);

    return nimg::ColorRGBf(clamp01(r), clamp01(g), clamp01(bch));
}

// =============================================================================
// Exponential
//
// T(x) = 1 - exp(-x)
//
// The simplest smooth operator: monotonically increases from 0 to 1,
// asymptotically approaching 1 as x → ∞. Never clips; very bright values
// are compressed into a narrow range near 1. No parameters beyond exposure.
// Good for fast previews where artistic accuracy is not required.
// =============================================================================
inline nimg::ColorRGBf apply_exponential(const nimg::ColorRGBf &c)
{
    return nimg::ColorRGBf(
          1.0f - std::exp(-std::max(0.0f, c.r()))
        , 1.0f - std::exp(-std::max(0.0f, c.g()))
        , 1.0f - std::exp(-std::max(0.0f, c.b()))
    );
}

// =============================================================================
// Lottes
//
// Timothy Lottes, "Advanced Techniques and Optimization of HDR Color
// Pipelines", GDC 2016.
// Reference implementation:
//   https://www.shadertoy.com/view/WdjSW3
//
// T(x) = x^a / (x^a * b + c)
//
// Parameters b and c are solved from two constraints to fix two perceptual
// anchor points, ensuring T(midIn) = midOut and T(hdrMax) = 1:
//
//   b = (-midIn^a + hdrMax^a * midOut) / ((hdrMax^a - midIn^a) * midOut)
//   c = (hdrMax^a * midIn^a - midIn^a * hdrMax^a * midOut)
//       / ((hdrMax^a - midIn^a) * midOut)
//
// Defaults: a=1.6 (contrast), hdrMax=8 (scene range), midIn=0.18,
// midOut=0.267 (18% grey maps to 26.7% display).
// =============================================================================
inline float lottes_f(float x)
{
    static const float a      = 1.6f;
    static const float hdrMax = 8.0f;
    static const float midIn  = 0.18f;
    static const float midOut = 0.267f;
    static const float pa  = std::pow(midIn,  a);   // midIn^a  ≈ 0.06476
    static const float ha  = std::pow(hdrMax, a);   // hdrMax^a ≈ 27.857
    static const float b   = (-pa + ha * midOut) / ((ha - pa) * midOut);
    static const float c   = (ha * pa - pa * ha * midOut) / ((ha - pa) * midOut);
    const float xa = std::pow(std::max(0.0f, x), a);
    return xa / (xa * b + c);
}

inline nimg::ColorRGBf apply_lottes(const nimg::ColorRGBf &c)
{
    return nimg::ColorRGBf(
          clamp01(lottes_f(c.r()))
        , clamp01(lottes_f(c.g()))
        , clamp01(lottes_f(c.b()))
    );
}

// =============================================================================
// Cineon / Hejl-Burgess-Dawson filmic ALU
//
// Jim Hejl and Richard Burgess-Dawson, submitted to John Hable's GDC 2010
// talk "Uncharted 2: HDR Lighting" (slide 142). Inspired by Haarm-Pieter
// Duiker's Kodak Cineon film-response curve work:
//   H.-P. Duiker, "Film Emulation Tonemapping in Game Realtime Rendering",
//   GDC 2010.
// Hable's blog reference:
//   http://filmicworlds.com/blog/filmic-tonemapping-operators/
//
// T(x) = (X * (6.2X + 0.5)) / (X * (6.2X + 1.7) + 0.06),  X = max(0, x - 0.004)
//
// The 0.004 offset lifts the black point slightly, mimicking film base fog.
// Note: the original formula included a pow(result, 2.2) gamma step because
// it targeted a non-linearised display pipeline; we omit it since our
// pipeline applies sRGB encoding at the save stage.
// =============================================================================
inline float cineon_f(float x)
{
    x = std::max(0.0f, x - 0.004f);
    return (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);
}

inline nimg::ColorRGBf apply_cineon(const nimg::ColorRGBf &c)
{
    return nimg::ColorRGBf(
          clamp01(cineon_f(c.r()))
        , clamp01(cineon_f(c.g()))
        , clamp01(cineon_f(c.b()))
    );
}

// =============================================================================
// Uchimura / Gran Turismo
//
// Hajime Uchimura, "HDR Theory and Practice", CEDEC 2017 / SIGGRAPH 2017
// Advances in Real-Time Rendering course.
// Reference shadertoy by Uchimura:
//   https://www.shadertoy.com/view/WdjSW3
//
// The curve is divided into three regions by smooth blending weights:
//
//   Toe (x < m):     T(x) = m * (x/m)^c + b
//   Linear (m..S0):  L(x) = m + a * (x - m)
//   Shoulder (x>S0): S(x) = P - (P - S1) * exp(CP * (x - S0))
//
// where:
//   S0 = m + l0,  S1 = m + a*l0,  l0 = (P-m)*l/a
//   C2 = a*P / (P - S1),          CP = -C2/P
//
// Weights use a smoothstep for the toe boundary and a step for the shoulder.
// The result is C1-continuous everywhere.
//
// Default parameters (Gran Turismo 7 presets):
//   P  = 1.0   peak display luminance
//   a  = 1.0   contrast (linear section slope)
//   m  = 0.22  linear section start
//   l  = 0.4   linear section length (fraction of P-m)
//   cv = 1.33  toe curve tightness (black)
//   bv = 0.0   pedestal (shadow lift)
// =============================================================================
inline float smoothstep_f(float edge0, float edge1, float x)
{
    const float t = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
    return t * t * (3.0f - 2.0f * t);
}

inline float uchimura_f(float x)
{
    const float P  = 1.0f;    // peak display luminance
    const float a  = 1.0f;    // contrast
    const float m  = 0.22f;   // linear section start
    const float l  = 0.4f;    // linear section length
    const float cv = 1.33f;   // black tightness (renamed to avoid clash with 'c')
    const float bv = 0.0f;    // pedestal

    x = std::max(0.0f, x);

    const float l0 = ((P - m) * l) / a;
    const float S0 = m + l0;
    const float S1 = m + a * l0;
    const float C2 = (a * P) / std::max(1e-6f, P - S1);
    const float CP = -C2 / P;

    // Region weights: toe, linear, shoulder
    const float w0 = 1.0f - smoothstep_f(0.0f, m, x);
    const float w2 = (x >= S0) ? 1.0f : 0.0f;
    const float w1 = 1.0f - w0 - w2;

    const float T = m * std::pow(x / std::max(1e-10f, m), cv) + bv;  // toe
    const float L = m + a * (x - m);                                  // linear
    const float S = P - (P - S1) * std::exp(CP * (x - S0));           // shoulder

    return clamp01(T * w0 + L * w1 + S * w2);
}

inline nimg::ColorRGBf apply_uchimura(const nimg::ColorRGBf &c)
{
    return nimg::ColorRGBf(
          uchimura_f(c.r())
        , uchimura_f(c.g())
        , uchimura_f(c.b())
    );
}

// =============================================================================
// AgX
//
// Troy Sobotka, AgX — designed to solve the out-of-gamut hue-shift problem
// that affects ACES and most per-channel operators. Highly saturated or
// emissive inputs that fall outside the display gamut are compressed smoothly
// rather than clipping, avoiding the "ACES orange" artefact.
//   https://github.com/sobotka/AgX
//
// Minimal C++ implementation derived from:
//   M. Devert, "A Minimal AgX Implementation"
//   https://iolite-engine.com/blog_posts/minimal_agx_implementation
// Also used in Blender (default tone mapper since 3.6):
//   https://projects.blender.org/blender/blender/src/branch/main/release/datafiles/colormanagement/
//
// Pipeline:
//   1. Inset matrix   — compress linear sRGB into the AgX working gamut.
//   2. Log2 encoding  — map the scene range [min_ev, max_ev] to [0,1].
//   3. S-curve        — 6th-degree polynomial fit to the AgX contrast curve.
//   4. Outset matrix  — expand back to linear sRGB with the AgX look applied.
//
// Matrix convention: the GLSL mat3(col0, col1, col2) constructors are
// column-major, so M*v in GLSL is transposed relative to the row-major
// indexing used here. The matrices below are already expressed as C++ row
// operations (result.r = row0 · input, etc.).
//
// Note: some reference implementations apply pow(val, 2.2) after the outset
// matrix. We omit this because our pipeline applies sRGB encoding at save
// time (pack_rgba8_srgb in nimg/img.cc), which already performs the
// linear → perceptual transform.
// =============================================================================
inline float agx_scurve(float x)
{
    // 6th-degree polynomial fit to the AgX S-curve contrast function.
    // Input and output are both in [0, 1] (log-encoded, normalised space).
    const float x2 = x * x;
    const float x4 = x2 * x2;
    return   15.5f   * x4 * x2
           - 40.14f  * x4 * x
           + 31.96f  * x4
           -  6.868f * x2 * x
           +  0.4298f * x2
           +  0.1191f * x
           -  0.00232f;
}

inline nimg::ColorRGBf apply_agx(const nimg::ColorRGBf &c)
{
    // Step 1: linear sRGB → AgX space (inset matrix, GLSL column-major decoded)
    //   col0 = (0.842479, 0.042328, 0.042376)
    //   col1 = (0.078434, 0.878469, 0.078434)
    //   col2 = (0.079224, 0.079166, 0.879143)
    const float ar = 0.842479062253094f  * c.r() + 0.0784335999999992f * c.g() + 0.0792237451477643f * c.b();
    const float ag = 0.0423282422610123f * c.r() + 0.878468636469772f  * c.g() + 0.0791661274605434f * c.b();
    const float ab = 0.0423756549057051f * c.r() + 0.0784336f          * c.g() + 0.879142973793104f  * c.b();

    // Step 2: log2 encoding, normalised to [0,1] over the scene EV range.
    //   min_ev = -12.47393 EV,  max_ev = 4.026069 EV  (≈ 16.5 stops total)
    const float min_ev = -12.47393f;
    const float max_ev =   4.026069f;
    const float rng    = max_ev - min_ev;
    const float lr = clamp01((std::log2(std::max(ar, 1e-10f)) - min_ev) / rng);
    const float lg = clamp01((std::log2(std::max(ag, 1e-10f)) - min_ev) / rng);
    const float lb = clamp01((std::log2(std::max(ab, 1e-10f)) - min_ev) / rng);

    // Step 3: apply S-curve per channel in log space
    const float sr = agx_scurve(lr);
    const float sg = agx_scurve(lg);
    const float sb = agx_scurve(lb);

    // Step 4: AgX space → linear sRGB (outset matrix, column-major decoded)
    //   col0 = ( 1.19688, -0.098021, -0.099030)
    //   col1 = (-0.052897,  1.15190, -0.098961)
    //   col2 = (-0.052972, -0.098043,  1.15107)
    const float r =  1.19687900512017f   * sr - 0.0528968517574562f  * sg - 0.0529716355144438f  * sb;
    const float g = -0.0980208811401368f * sr + 1.15190312990417f    * sg - 0.0980434501171241f  * sb;
    const float b = -0.0990297440797205f * sr - 0.0989611768448433f  * sg + 1.15107367264116f    * sb;

    return nimg::ColorRGBf(clamp01(r), clamp01(g), clamp01(b));
}

// =============================================================================
// Khronos PBR Neutral
//
// Khronos Group, 2023. Designed for PBR content where minimal artistic bias
// is desired: bring HDR values into display range while disturbing the
// physical color relationships as little as possible. Useful for evaluating
// material accuracy rather than achieving a cinematic look.
//   https://github.com/KhronosGroup/ToneMapping/tree/main/PBR_Neutral
//   Reference GLSL: pbrNeutral.glsl (MIT licence)
//
// The operator has two stages:
//
//   Toe — removes a small black offset to improve shadow detail:
//     offset = x < 0.08 ? x - 6.25*x² : 0.04
//     (This is the approximate inverse of sRGB's linear segment at low values.)
//
//   Shoulder — compresses highlights and desaturates near peak white:
//     newPeak = 1 - d² / (peak + d - startCompression),  d = 1 - startCompression
//     Channels are scaled to newPeak, then blended toward white by factor g
//     which grows with (peak - newPeak), controlled by desaturation = 0.15.
//
// startCompression = 0.76 (highlights above this value are compressed).
// =============================================================================
inline nimg::ColorRGBf apply_khronos_pbr_neutral(const nimg::ColorRGBf &c)
{
    const float startCompression = 0.76f;   // 0.8 - 0.04
    const float desaturation     = 0.15f;

    float r = c.r();
    float g = c.g();
    float b = c.b();

    // Toe: remove black offset (approximates inverse of sRGB linear section)
    const float x      = std::min(r, std::min(g, b));
    const float offset = (x < 0.08f) ? x - 6.25f * x * x : 0.04f;
    r -= offset;
    g -= offset;
    b -= offset;

    // Shoulder: compress and desaturate highlights above startCompression
    const float peak = std::max(r, std::max(g, b));
    if (peak < startCompression)
        return nimg::ColorRGBf(r, g, b);

    const float d       = 1.0f - startCompression;
    const float newPeak = 1.0f - d * d / (peak + d - startCompression);
    const float scale   = newPeak / peak;
    r *= scale;
    g *= scale;
    b *= scale;

    // Blend toward white as the highlight is compressed
    const float gr = 1.0f - 1.0f / (desaturation * (peak - newPeak) + 1.0f);
    r += gr * (newPeak - r);
    g += gr * (newPeak - g);
    b += gr * (newPeak - b);

    return nimg::ColorRGBf(clamp01(r), clamp01(g), clamp01(b));
}

// =============================================================================
// Gaussian blur helper (log-luminance domain, used by local operator below)
//
// Standard two-pass (horizontal then vertical) separable Gaussian convolution
// with clamped boundary (nearest-neighbour padding). sigma controls spatial
// extent; radius = ceil(3*sigma) covers 99.7% of kernel mass.
// =============================================================================
inline size_t idx(size_t x, size_t y, size_t w)
{
    return y * w + x;
}

void gaussian_blur_separable(const std::vector<float> &src,
                             std::vector<float> &dst,
                             size_t w, size_t h,
                             float sigma)
{
    if (w == 0 || h == 0) return;

    sigma = std::max(0.15f, sigma);
    int radius = std::max(1, (int)std::ceil(3.0f * sigma));

    std::vector<float> kernel((size_t)(2 * radius + 1), 0.0f);
    float sum = 0.0f;
    for (int i = -radius; i <= radius; ++i) {
        const float x = (float)i;
        const float v = std::exp(-(x * x) / (2.0f * sigma * sigma));
        kernel[(size_t)(i + radius)] = v;
        sum += v;
    }
    if (sum <= 0.0f) sum = 1.0f;
    for (size_t i = 0; i < kernel.size(); ++i) kernel[i] /= sum;

    std::vector<float> tmp(w * h, 0.0f);

    // Horizontal pass
    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            float acc = 0.0f;
            for (int k = -radius; k <= radius; ++k) {
                int sx = (int)x + k;
                if (sx < 0) sx = 0;
                if (sx >= (int)w) sx = (int)w - 1;
                acc += src[idx((size_t)sx, y, w)] * kernel[(size_t)(k + radius)];
            }
            tmp[idx(x, y, w)] = acc;
        }
    }

    // Vertical pass
    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            float acc = 0.0f;
            for (int k = -radius; k <= radius; ++k) {
                int sy = (int)y + k;
                if (sy < 0) sy = 0;
                if (sy >= (int)h) sy = (int)h - 1;
                acc += tmp[idx(x, (size_t)sy, w)] * kernel[(size_t)(k + radius)];
            }
            dst[idx(x, y, w)] = acc;
        }
    }
}

// =============================================================================
// Local tone mapper (log-domain base/detail decomposition)
//
// This is a log-domain local operator inspired by the base/detail decomposition
// approach described in:
//   R. Fattal, D. Lischinski, M. Werman
//   "Gradient Domain High Dynamic Range Compression"
//   SIGGRAPH 2002
//   https://doi.org/10.1145/566570.566573
//
// and the multi-scale bilateral framework of:
//   F. Durand, J. Dorsey
//   "Fast Bilateral Filtering for the Display of High-Dynamic-Range Images"
//   SIGGRAPH 2002
//   https://doi.org/10.1145/566570.566574
//
// Note: this operator shares conceptual ideas with the Mantiuk 2006 paper
// (gradient-domain compression, local adaptation) but does NOT implement the
// full Mantiuk model (which requires a spatially-varying contrast sensitivity
// function and iterative gradient-domain solve). The implementation here is a
// simpler single-scale Gaussian base/detail split, which is faster and
// sufficient for interactive use.
//
// Algorithm:
//   1. Compute per-pixel log-luminance L = log(lum).
//   2. Extract the base layer B = GaussianBlur(L, sigma), where sigma is
//      derived from the `detail` parameter (large sigma → more global).
//   3. The detail layer is D = L - B.
//   4. Reconstruct a compressed log-luminance:
//        L' = (B - mean(B)) * base_scale + D * detail_gain
//      where base_scale compresses the global contrast and detail_gain
//      amplifies or attenuates fine structure.
//   5. Exponentiate and apply extended Reinhard to clip to [0,1].
//   6. Reconstruct chroma using a saturation-weighted ratio of original
//      channel values to luminance:
//        channel' = mapped_L * (1 + saturation * (channel/L - 1))
//
// Parameters:
//   contrast   [0,1]   — global contrast compression. 0 = maximally flat,
//                        1 = full dynamic range preserved before Reinhard clip.
//   saturation [0,2]   — chroma scale. 1 = unchanged, 0 = desaturated,
//                        values > 1 boost saturation.
//   detail     [1,99]  — local sharpness. Low values use a large blur radius
//                        (more aggressive local adaptation); high values use
//                        a small blur radius (structure is preserved).
// =============================================================================
void apply_mantiuk_2006_pixmap(nimg::Pixmap &pixmap, const settings_t &settings)
{
    const size_t w = pixmap.width();
    const size_t h = pixmap.height();
    if (w == 0 || h == 0) return;

    const float contrast   = std::max(0.0f, std::min(1.0f,  settings.mantiuk_contrast));
    const float saturation = std::max(0.0f, std::min(2.0f,  settings.mantiuk_saturation));
    const float detail     = std::max(1.0f, std::min(99.0f, settings.mantiuk_detail));
    const float white_point = (settings.white_point > 0.0f) ? settings.white_point : 1.0f;
    const float exposure    = (settings.exposure    > 0.0f) ? settings.exposure    : 1.0f;
    const float eps = 1e-6f;

    std::vector<float> lum(w * h, 0.0f);
    std::vector<float> log_lum(w * h, 0.0f);
    std::vector<float> base(w * h, 0.0f);
    std::vector<float> log_mapped(w * h, 0.0f);

    // Step 1: compute luminance and log-luminance; accumulate log mean for
    // centering (equivalent to computing the log-average / key value).
    float mean_log = 0.0f;
    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            nimg::ColorRGBAf px = pixmap.pixel(x, y);
            nimg::ColorRGBf c = nimg::ColorRGBf(px) * exposure;
            const float l = std::max(eps, nimg::eval::luminance(c));
            const size_t i = idx(x, y, w);
            lum[i]     = l;
            log_lum[i] = std::log(l);
            mean_log  += log_lum[i];
        }
    }
    mean_log /= (float)(w * h);

    // Step 2: extract base layer via Gaussian blur.
    // sigma maps exponentially from ~12 px (detail=1, most global) to ~0.6 px
    // (detail=99, most local), giving perceptually even spacing across the UI range.
    const float t     = (detail - 1.0f) / 98.0f;
    const float sigma = 12.0f * std::pow(0.05f, t); // range: ~12.0 -> ~0.6
    gaussian_blur_separable(log_lum, base, w, h, sigma);

    // Steps 3-4: compress base, preserve/boost detail.
    // base_scale < 1 compresses global contrast; detail_gain > 1 sharpens local structure.
    const float base_scale  = 0.18f + 0.82f * contrast;
    const float dnorm       = (detail - 50.0f) / 49.0f;    // normalised to [-1, 1]
    const float detail_gain = std::pow(2.5f, dnorm);        // range: ~0.4 -> ~2.5
    for (size_t i = 0; i < log_mapped.size(); ++i) {
        const float detail_layer  = log_lum[i] - base[i];
        const float centered_base = base[i] - mean_log;
        log_mapped[i] = centered_base * base_scale + detail_layer * detail_gain;
    }

    // Steps 5-6: reconstruct per-pixel colour.
    for (size_t y = 0; y < h; ++y) {
        for (size_t x = 0; x < w; ++x) {
            const size_t i = idx(x, y, w);

            // Exponentiate and apply Reinhard clip to land in [0,1].
            float mapped_l = std::exp(log_mapped[i]);
            mapped_l = reinhard_extended(mapped_l, white_point);
            mapped_l = std::max(eps, mapped_l);

            const float src_l = std::max(eps, lum[i]);

            nimg::ColorRGBAf px = pixmap.pixel(x, y);
            nimg::ColorRGBf c = nimg::ColorRGBf(px) * exposure;

            // Chroma-preserving reconstruction with explicit saturation control.
            // rr/gg/bb are the per-channel ratios relative to luminance.
            // saturation=1 → sr=rr (original chroma); saturation=0 → sr=1 (grey).
            const float rr = c.r() / src_l;
            const float gg = c.g() / src_l;
            const float bb = c.b() / src_l;
            const float sr = 1.0f + saturation * (rr - 1.0f);
            const float sg = 1.0f + saturation * (gg - 1.0f);
            const float sb = 1.0f + saturation * (bb - 1.0f);

            float r = mapped_l * std::max(0.0f, sr);
            float g = mapped_l * std::max(0.0f, sg);
            float b = mapped_l * std::max(0.0f, sb);

            px.r(clamp01(r));
            px.g(clamp01(g));
            px.b(clamp01(b));
            pixmap.pixel(x, y) = px;
        }
    }
}

} // namespace

nimg::ColorRGBf apply(const nimg::ColorRGBf &color, const settings_t &settings)
{
    nimg::ColorRGBf c = color;

    const float exposure    = (settings.exposure    > 0.0f) ? settings.exposure    : 1.0f;
    const float white_point = (settings.white_point > 0.0f) ? settings.white_point : 1.0f;
    c *= exposure;

    switch (settings.op) {
        case OP_NONE:
            return c;
        case OP_REINHARD:
            return apply_reinhard(c, white_point);
        case OP_REINHARD_LUMINANCE:
            return apply_reinhard_luminance(c, white_point);
        case OP_MANTIUK_2006:
            // The full local-operator path requires image context (neighbouring
            // pixels) and is handled in apply(Pixmap). When called per-pixel,
            // fall back to luminance Reinhard as a reasonable approximation.
            return apply_reinhard_luminance(c, white_point);
        case OP_ACES_FITTED:
            return apply_aces_fitted(c);
        case OP_HABLE:
            return apply_hable(c, white_point);
        case OP_EXPONENTIAL:
            return apply_exponential(c);
        case OP_LOTTES:
            return apply_lottes(c);
        case OP_CINEON:
            return apply_cineon(c);
        case OP_UCHIMURA:
            return apply_uchimura(c);
        case OP_AGX:
            return apply_agx(c);
        case OP_KHRONOS_PBR_NEUTRAL:
            return apply_khronos_pbr_neutral(c);
    }

    return c;
}

void apply(nimg::Pixmap &pixmap, const settings_t &settings)
{
    if (settings.op == OP_MANTIUK_2006) {
        apply_mantiuk_2006_pixmap(pixmap, settings);
        return;
    }

    for (size_t y = 0; y < pixmap.height(); ++y) {
        for (size_t x = 0; x < pixmap.width(); ++x) {
            nimg::ColorRGBAf px = pixmap.pixel(x, y);
            nimg::ColorRGBf tm = apply(nimg::ColorRGBf(px), settings);
            px.r(tm.r());
            px.g(tm.g());
            px.b(tm.b());
            pixmap.pixel(x, y) = px;
        }
    }
}

} /* namespace tonemapping */
} /* namespace xtcore */
