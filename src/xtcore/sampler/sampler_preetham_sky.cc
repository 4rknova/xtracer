#include <cmath>
#include <algorithm>

#include "sampler_preetham_sky.h"

// Preetham, Shirley, Smits 1999: "A Practical Analytic Model for Daylight"
// https://doi.org/10.1145/311535.311545
//
// Full Perez-function sky luminance and chromaticity model with sRGB output.

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

inline nmath::scalar_t perez(nmath::scalar_t A, nmath::scalar_t B, nmath::scalar_t C,
                              nmath::scalar_t D, nmath::scalar_t E,
                              nmath::scalar_t theta, nmath::scalar_t gamma)
{
    const nmath::scalar_t cos_theta = (nmath::scalar_t)std::cos((double)theta);
    const nmath::scalar_t cos_gamma = (nmath::scalar_t)std::cos((double)gamma);
    const nmath::scalar_t denom = (std::fabs((double)cos_theta) < 1e-6) ? (nmath::scalar_t)1e-6 : cos_theta;
    return ((nmath::scalar_t)1.0 + A * (nmath::scalar_t)std::exp((double)(B / denom))) *
           ((nmath::scalar_t)1.0 + C * (nmath::scalar_t)std::exp((double)(D * gamma)) + E * cos_gamma * cos_gamma);
}

// Turbidity-dependent Perez coefficients from Preetham Table 2
struct PerezCoeffs { nmath::scalar_t A, B, C, D, E; };

inline PerezCoeffs luminance_coeffs(nmath::scalar_t T)
{
    PerezCoeffs c;
    c.A = (nmath::scalar_t)0.1787 * T - (nmath::scalar_t)1.4630;
    c.B = (nmath::scalar_t)-0.3554 * T + (nmath::scalar_t)0.4275;
    c.C = (nmath::scalar_t)-0.0227 * T + (nmath::scalar_t)5.3251;
    c.D = (nmath::scalar_t)0.1206 * T - (nmath::scalar_t)2.5771;
    c.E = (nmath::scalar_t)-0.0670 * T + (nmath::scalar_t)0.3703;
    return c;
}
inline PerezCoeffs chroma_x_coeffs(nmath::scalar_t T)
{
    PerezCoeffs c;
    c.A = (nmath::scalar_t)-0.0193 * T - (nmath::scalar_t)0.2592;
    c.B = (nmath::scalar_t)-0.0665 * T + (nmath::scalar_t)0.0008;
    c.C = (nmath::scalar_t)-0.0004 * T + (nmath::scalar_t)0.2125;
    c.D = (nmath::scalar_t)-0.0641 * T - (nmath::scalar_t)0.8989;
    c.E = (nmath::scalar_t)-0.0033 * T + (nmath::scalar_t)0.0452;
    return c;
}
inline PerezCoeffs chroma_y_coeffs(nmath::scalar_t T)
{
    PerezCoeffs c;
    c.A = (nmath::scalar_t)-0.0167 * T - (nmath::scalar_t)0.2608;
    c.B = (nmath::scalar_t)-0.0950 * T + (nmath::scalar_t)0.0092;
    c.C = (nmath::scalar_t)-0.0079 * T + (nmath::scalar_t)0.2102;
    c.D = (nmath::scalar_t)-0.0441 * T - (nmath::scalar_t)1.6537;
    c.E = (nmath::scalar_t)-0.0109 * T + (nmath::scalar_t)0.0529;
    return c;
}

// Zenith luminance (kcd/m²) from Preetham equation 7
inline nmath::scalar_t zenith_luminance(nmath::scalar_t T, nmath::scalar_t theta_s)
{
    const nmath::scalar_t chi = ((nmath::scalar_t)(4.0/9.0) - T / (nmath::scalar_t)120.0)
                                * ((nmath::scalar_t)3.14159265358979323846 - (nmath::scalar_t)2.0 * theta_s);
    return ((nmath::scalar_t)4.0453 * T - (nmath::scalar_t)4.9710) * (nmath::scalar_t)std::tan((double)chi)
           - (nmath::scalar_t)0.2155 * T + (nmath::scalar_t)2.4192;
}

// Zenith chromaticities from Preetham Table 3 (polynomial in T and theta_s)
inline nmath::scalar_t zenith_x(nmath::scalar_t T, nmath::scalar_t t)
{
    const nmath::scalar_t t2 = t * t; const nmath::scalar_t t3 = t2 * t;
    return T * T * ((nmath::scalar_t)0.00166 * t3 - (nmath::scalar_t)0.00375 * t2 + (nmath::scalar_t)0.00209 * t)
         + T    * (-(nmath::scalar_t)0.02903 * t3 + (nmath::scalar_t)0.06377 * t2 - (nmath::scalar_t)0.03202 * t + (nmath::scalar_t)0.00394)
         +          (nmath::scalar_t)0.11693 * t3 - (nmath::scalar_t)0.21196 * t2 + (nmath::scalar_t)0.06052 * t + (nmath::scalar_t)0.25886;
}
inline nmath::scalar_t zenith_y(nmath::scalar_t T, nmath::scalar_t t)
{
    const nmath::scalar_t t2 = t * t; const nmath::scalar_t t3 = t2 * t;
    return T * T * ((nmath::scalar_t)0.00275 * t3 - (nmath::scalar_t)0.00610 * t2 + (nmath::scalar_t)0.00317 * t)
         + T    * (-(nmath::scalar_t)0.04214 * t3 + (nmath::scalar_t)0.08970 * t2 - (nmath::scalar_t)0.04153 * t + (nmath::scalar_t)0.00516)
         +          (nmath::scalar_t)0.15346 * t3 - (nmath::scalar_t)0.26756 * t2 + (nmath::scalar_t)0.06670 * t + (nmath::scalar_t)0.26688;
}

// CIE xyY -> linear sRGB (D65 white point)
inline nimg::ColorRGBf xyY_to_rgb(nmath::scalar_t x, nmath::scalar_t y, nmath::scalar_t Y)
{
    if (y < (nmath::scalar_t)1e-6) return nimg::ColorRGBf(0.f, 0.f, 0.f);
    const nmath::scalar_t X = x * Y / y;
    const nmath::scalar_t Z = ((nmath::scalar_t)1.0 - x - y) * Y / y;
    // sRGB from CIE XYZ (D65, IEC 61966-2-1)
    const nmath::scalar_t r = (nmath::scalar_t)3.2404542 * X - (nmath::scalar_t)1.5371385 * Y - (nmath::scalar_t)0.4985314 * Z;
    const nmath::scalar_t g = -(nmath::scalar_t)0.9692660 * X + (nmath::scalar_t)1.8760108 * Y + (nmath::scalar_t)0.0415560 * Z;
    const nmath::scalar_t b = (nmath::scalar_t)0.0556434 * X - (nmath::scalar_t)0.2040259 * Y + (nmath::scalar_t)1.0572252 * Z;
    return nimg::ColorRGBf(
        clampz(r),
        clampz(g),
        clampz(b)
    );
}

} // namespace

PreethamSky::PreethamSky()
    : sun_direction(nmath::Vector3f(0.35f, 0.8f, 0.2f))
    , turbidity((nmath::scalar_t)3.0)
    , exposure((nmath::scalar_t)0.04)
    , ground_color(nimg::ColorRGBf(0.02f, 0.02f, 0.02f))
{}

PreethamSky::~PreethamSky()
{}

nimg::ColorRGBf PreethamSky::sample(const nmath::Vector3f &uvw) const
{
    // Normalise view direction (uvw is used as a direction vector, same as other sky samplers)
    const nmath::scalar_t len = (nmath::scalar_t)std::sqrt((double)(uvw.x*uvw.x + uvw.y*uvw.y + uvw.z*uvw.z));
    if (len < (nmath::scalar_t)1e-6) return ground_color;

    const nmath::scalar_t vx = uvw.x / len;
    const nmath::scalar_t vy = uvw.y / len;
    const nmath::scalar_t vz = uvw.z / len;

    // Below horizon: return ground colour
    if (vy < (nmath::scalar_t)0.0) return ground_color;

    // Normalise sun direction
    const nmath::scalar_t slen = (nmath::scalar_t)std::sqrt((double)(sun_direction.x*sun_direction.x
                                                              + sun_direction.y*sun_direction.y
                                                              + sun_direction.z*sun_direction.z));
    if (slen < (nmath::scalar_t)1e-6) return ground_color;
    const nmath::scalar_t sx = sun_direction.x / slen;
    const nmath::scalar_t sy = sun_direction.y / slen;
    const nmath::scalar_t sz = sun_direction.z / slen;

    const nmath::scalar_t T  = std::max((nmath::scalar_t)1.7, std::min((nmath::scalar_t)10.0, turbidity));

    // theta_s = sun zenith angle
    const nmath::scalar_t cos_ts = std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, sy));
    const nmath::scalar_t theta_s = (nmath::scalar_t)std::acos((double)cos_ts);

    // gamma = angle between view and sun
    const nmath::scalar_t cos_g = clamp01(vx*sx + vy*sy + vz*sz);
    const nmath::scalar_t gamma = (nmath::scalar_t)std::acos((double)cos_g);

    // theta = view zenith angle
    const nmath::scalar_t theta = (nmath::scalar_t)std::acos((double)std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, vy)));

    const PerezCoeffs cY = luminance_coeffs(T);
    const PerezCoeffs cx = chroma_x_coeffs(T);
    const PerezCoeffs cy = chroma_y_coeffs(T);

    const nmath::scalar_t Pz_Y = perez(cY.A, cY.B, cY.C, cY.D, cY.E, (nmath::scalar_t)0.0, theta_s);
    const nmath::scalar_t Pz_x = perez(cx.A, cx.B, cx.C, cx.D, cx.E, (nmath::scalar_t)0.0, theta_s);
    const nmath::scalar_t Pz_y = perez(cy.A, cy.B, cy.C, cy.D, cy.E, (nmath::scalar_t)0.0, theta_s);

    if (std::fabs((double)Pz_Y) < 1e-10 || std::fabs((double)Pz_x) < 1e-10 || std::fabs((double)Pz_y) < 1e-10)
        return ground_color;

    const nmath::scalar_t Yze = zenith_luminance(T, theta_s);
    const nmath::scalar_t xze = zenith_x(T, theta_s);
    const nmath::scalar_t yze = zenith_y(T, theta_s);

    const nmath::scalar_t Y = Yze * perez(cY.A, cY.B, cY.C, cY.D, cY.E, theta, gamma) / Pz_Y;
    const nmath::scalar_t x = xze * perez(cx.A, cx.B, cx.C, cx.D, cx.E, theta, gamma) / Pz_x;
    const nmath::scalar_t y = yze * perez(cy.A, cy.B, cy.C, cy.D, cy.E, theta, gamma) / Pz_y;

    const nmath::scalar_t exp = (exposure > (nmath::scalar_t)1e-8) ? exposure : (nmath::scalar_t)0.04;
    nimg::ColorRGBf col = xyY_to_rgb(x, y, Y * exp);

    // Blend to ground at horizon
    const nmath::scalar_t horizon = clamp01(vy * (nmath::scalar_t)8.0);
    return col * horizon + ground_color * ((nmath::scalar_t)1.0 - horizon);
}

    } /* namespace sampler */
} /* namespace xtcore */
