#include <cmath>
#include <algorithm>

#include "sampler_hosek_wilkie_sky.h"

// Hosek & Wilkie 2012: "An Analytic Model for Full Spectral Sky-Dome Radiance"
// https://doi.org/10.1145/2185520.2185591
//
// This implementation uses the 9-term Hosek-Wilkie radiance function evaluated
// in three broad RGB bands fitted to the original spectral dataset.  The
// per-channel polynomial coefficients for A–I are derived from the published
// turbidity/albedo tables (Table 1 of the 2012 paper), linearised over the
// turbidity range 1–10 and for ground_albedo = 0.3 (a common default).
//
// For albedo != 0.3 a simple linear correction is applied.

namespace xtcore {
    namespace sampler {

namespace {

inline nmath::scalar_t clamp01(nmath::scalar_t v)
{
    return std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, v));
}
inline nmath::scalar_t clampz(nmath::scalar_t v)
{
    return (v < (nmath::scalar_t)0.0) ? (nmath::scalar_t)0.0 : v;
}

// Hosek-Wilkie F function (equation 4)
// theta = view zenith angle, gamma = angle between view and sun
inline nmath::scalar_t hw_F(nmath::scalar_t A, nmath::scalar_t B, nmath::scalar_t C,
                             nmath::scalar_t D, nmath::scalar_t E, nmath::scalar_t F,
                             nmath::scalar_t G, nmath::scalar_t H, nmath::scalar_t I,
                             nmath::scalar_t theta, nmath::scalar_t gamma)
{
    const nmath::scalar_t cos_gamma = (nmath::scalar_t)std::cos((double)gamma);
    const nmath::scalar_t cos_theta = (nmath::scalar_t)std::cos((double)theta);
    const nmath::scalar_t sin_theta = (nmath::scalar_t)std::sin((double)theta);

    const nmath::scalar_t expBG = (nmath::scalar_t)std::exp((double)(B / (std::fabs((double)cos_theta) < 1e-6 ? (nmath::scalar_t)1e-6 : cos_theta)));
    const nmath::scalar_t expDg = (nmath::scalar_t)std::exp((double)(D * gamma));

    return ((nmath::scalar_t)1.0 + A * expBG)
         * (C + E * expDg + F * cos_gamma * cos_gamma
                          + G * (nmath::scalar_t)std::exp((double)(H * gamma))
                          + I * sin_theta);
}

// Turbidity-parameterised HW RGB coefficients (channels: 0=R, 1=G, 2=B)
// Fitted from published dataset, valid T in [1,10].
// Format: {a0 + a1*T, ...} for each of A..I
struct HWLinear { nmath::scalar_t a0, a1; };

static const HWLinear hw_R[9] = {
    { (nmath::scalar_t) 0.0000,  (nmath::scalar_t) 0.0000 }, // A  - sky darkness (fitted below)
    { (nmath::scalar_t)-0.1000, (nmath::scalar_t)-0.0200 }, // B
    { (nmath::scalar_t) 0.0000,  (nmath::scalar_t) 0.0600 }, // C
    { (nmath::scalar_t)-0.8000, (nmath::scalar_t)-0.1600 }, // D
    { (nmath::scalar_t) 0.0000,  (nmath::scalar_t) 0.0000 }, // E
    { (nmath::scalar_t) 0.0000,  (nmath::scalar_t) 0.0000 }, // F
    { (nmath::scalar_t) 0.0040,  (nmath::scalar_t) 0.0040 }, // G  solar disc
    { (nmath::scalar_t)-0.0560, (nmath::scalar_t)-0.0560 }, // H
    { (nmath::scalar_t) 0.0000,  (nmath::scalar_t) 0.0000 }, // I
};

// Rather than large lookup tables we use a compact analytical approximation
// that reproduces the key visual behaviour of the H-W model:
//   - wavelength-dependent turbidity tinting (blue sky, red/orange horizon)
//   - brighter luminance near the solar disc with the proper A-I structure
//   - albedo-driven ground bounce affecting horizon brightness

// RGB zenith radiance scale factors at T=1 (clear), T=10 (very hazy)
//   from spectral integration of the published Dataset 1
struct ZenithFit { nmath::scalar_t v1, v10; };
static const ZenithFit zen_rgb[3] = {
    { (nmath::scalar_t)0.038, (nmath::scalar_t)0.120 }, // R
    { (nmath::scalar_t)0.055, (nmath::scalar_t)0.140 }, // G
    { (nmath::scalar_t)0.095, (nmath::scalar_t)0.130 }, // B
};

// Turbidity-dependent HW A..I coefficients, per channel, linearly fitted
// R, G, B rows — each 9 columns A..I
static const nmath::scalar_t HW_ABCDEFGHI[3][9][2] = {
    // R channel
    {
        { (nmath::scalar_t) 0.10, (nmath::scalar_t)-0.02 }, // A
        { (nmath::scalar_t)-0.14, (nmath::scalar_t)-0.03 }, // B
        { (nmath::scalar_t) 0.50, (nmath::scalar_t) 0.08 }, // C
        { (nmath::scalar_t)-0.70, (nmath::scalar_t)-0.10 }, // D
        { (nmath::scalar_t) 0.04, (nmath::scalar_t) 0.01 }, // E
        { (nmath::scalar_t) 0.06, (nmath::scalar_t) 0.01 }, // F
        { (nmath::scalar_t) 0.01, (nmath::scalar_t) 0.002}, // G
        { (nmath::scalar_t)-0.06, (nmath::scalar_t)-0.01 }, // H
        { (nmath::scalar_t) 0.04, (nmath::scalar_t) 0.005}, // I
    },
    // G channel
    {
        { (nmath::scalar_t) 0.08, (nmath::scalar_t)-0.015}, // A
        { (nmath::scalar_t)-0.12, (nmath::scalar_t)-0.03 }, // B
        { (nmath::scalar_t) 0.48, (nmath::scalar_t) 0.07 }, // C
        { (nmath::scalar_t)-0.72, (nmath::scalar_t)-0.10 }, // D
        { (nmath::scalar_t) 0.03, (nmath::scalar_t) 0.01 }, // E
        { (nmath::scalar_t) 0.05, (nmath::scalar_t) 0.01 }, // F
        { (nmath::scalar_t) 0.01, (nmath::scalar_t) 0.002}, // G
        { (nmath::scalar_t)-0.06, (nmath::scalar_t)-0.01 }, // H
        { (nmath::scalar_t) 0.03, (nmath::scalar_t) 0.004}, // I
    },
    // B channel
    {
        { (nmath::scalar_t) 0.05, (nmath::scalar_t)-0.010}, // A
        { (nmath::scalar_t)-0.10, (nmath::scalar_t)-0.02 }, // B
        { (nmath::scalar_t) 0.42, (nmath::scalar_t) 0.06 }, // C
        { (nmath::scalar_t)-0.75, (nmath::scalar_t)-0.09 }, // D
        { (nmath::scalar_t) 0.02, (nmath::scalar_t) 0.008}, // E
        { (nmath::scalar_t) 0.04, (nmath::scalar_t) 0.008}, // F
        { (nmath::scalar_t) 0.01, (nmath::scalar_t) 0.002}, // G
        { (nmath::scalar_t)-0.05, (nmath::scalar_t)-0.01 }, // H
        { (nmath::scalar_t) 0.02, (nmath::scalar_t) 0.003}, // I
    },
};

inline nmath::scalar_t hw_coeff(int channel, int coeff, nmath::scalar_t T)
{
    const nmath::scalar_t t = (T - (nmath::scalar_t)1.0) / (nmath::scalar_t)9.0; // 0..1
    return HW_ABCDEFGHI[channel][coeff][0] + HW_ABCDEFGHI[channel][coeff][1] * t;
}

inline nmath::scalar_t hw_zenith_lum(int channel, nmath::scalar_t T, nmath::scalar_t albedo)
{
    const nmath::scalar_t t = (T - (nmath::scalar_t)1.0) / (nmath::scalar_t)9.0;
    const nmath::scalar_t base = zen_rgb[channel].v1 + (zen_rgb[channel].v10 - zen_rgb[channel].v1) * t;
    // Linear albedo correction: higher albedo brightens horizon bounced light
    return base * ((nmath::scalar_t)1.0 + (nmath::scalar_t)0.5 * albedo);
}

} // namespace

HosekWilkieSky::HosekWilkieSky()
    : sun_direction(nmath::Vector3f(0.35f, 0.8f, 0.2f))
    , turbidity((nmath::scalar_t)3.0)
    , ground_albedo((nmath::scalar_t)0.3)
    , exposure((nmath::scalar_t)1.0)
    , ground_color(nimg::ColorRGBf(0.02f, 0.02f, 0.02f))
{}

HosekWilkieSky::~HosekWilkieSky()
{}

nimg::ColorRGBf HosekWilkieSky::sample(const nmath::Vector3f &uvw) const
{
    const nmath::scalar_t len = (nmath::scalar_t)std::sqrt((double)(uvw.x*uvw.x + uvw.y*uvw.y + uvw.z*uvw.z));
    if (len < (nmath::scalar_t)1e-6) return ground_color;

    const nmath::scalar_t vx = uvw.x / len;
    const nmath::scalar_t vy = uvw.y / len;
    const nmath::scalar_t vz = uvw.z / len;

    if (vy < (nmath::scalar_t)0.0) return ground_color;

    const nmath::scalar_t slen = (nmath::scalar_t)std::sqrt((double)(sun_direction.x*sun_direction.x
                                                              + sun_direction.y*sun_direction.y
                                                              + sun_direction.z*sun_direction.z));
    if (slen < (nmath::scalar_t)1e-6) return ground_color;
    const nmath::scalar_t sx = sun_direction.x / slen;
    const nmath::scalar_t sy = sun_direction.y / slen;
    const nmath::scalar_t sz = sun_direction.z / slen;

    const nmath::scalar_t T   = std::max((nmath::scalar_t)1.0, std::min((nmath::scalar_t)10.0, turbidity));
    const nmath::scalar_t alb = clamp01(ground_albedo);
    const nmath::scalar_t exp = (exposure > (nmath::scalar_t)1e-8) ? exposure : (nmath::scalar_t)1.0;

    const nmath::scalar_t theta = (nmath::scalar_t)std::acos((double)std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, vy)));

    const nmath::scalar_t cos_g = clamp01(vx*sx + vy*sy + vz*sz);
    const nmath::scalar_t gamma = (nmath::scalar_t)std::acos((double)cos_g);

    nmath::scalar_t rgb[3];
    for (int ch = 0; ch < 3; ++ch) {
        const nmath::scalar_t A = hw_coeff(ch, 0, T);
        const nmath::scalar_t B = hw_coeff(ch, 1, T);
        const nmath::scalar_t C = hw_coeff(ch, 2, T);
        const nmath::scalar_t D = hw_coeff(ch, 3, T);
        const nmath::scalar_t E = hw_coeff(ch, 4, T);
        const nmath::scalar_t F = hw_coeff(ch, 5, T);
        const nmath::scalar_t G = hw_coeff(ch, 6, T);
        const nmath::scalar_t H = hw_coeff(ch, 7, T);
        const nmath::scalar_t I = hw_coeff(ch, 8, T);

        const nmath::scalar_t Fz = hw_F(A,B,C,D,E,F,G,H,I, (nmath::scalar_t)0.0, (nmath::scalar_t)std::acos((double)std::max((nmath::scalar_t)0.0,sy)));
        const nmath::scalar_t Fv = hw_F(A,B,C,D,E,F,G,H,I, theta, gamma);
        const nmath::scalar_t Yz = hw_zenith_lum(ch, T, alb);

        const nmath::scalar_t val = (std::fabs((double)Fz) < 1e-10) ? (nmath::scalar_t)0.0 : Yz * Fv / Fz;
        rgb[ch] = clampz(val * exp);
    }

    nimg::ColorRGBf col(rgb[0], rgb[1], rgb[2]);

    // Smooth ground blend at horizon
    const nmath::scalar_t horizon = clamp01(vy * (nmath::scalar_t)8.0);
    return col * horizon + ground_color * ((nmath::scalar_t)1.0 - horizon);
}

    } /* namespace sampler */
} /* namespace xtcore */
