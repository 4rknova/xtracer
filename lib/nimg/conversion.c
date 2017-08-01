#include "conversion.h"

#ifdef __cplusplus
extern "C" {
#endif

// ref: http://www.equasys.de/colorconversion.html

void rgb_to_ycbcr(float r, float g, float b, float *y, float *cb, float *cr)
{
	float k0 =  16./255.;
    float k1 = 128./255.;
    *y  = (r *  0.257 + g *  0.504 + b *  0.098) + k0; // Luminance
    *cb = (r * -0.148 + g * -0.291 + b *  0.439) + k1; // Chrominance Red
    *cr = (r *  0.439 + g * -0.368 + b * -0.071) + k1; // Chrominance Blue
}

void rgb_to_ycgco(float r, float g, float b, float *y, float *cg, float *co)
{
    *y  =  .25 * r + .50 * g + .25 * b; // Luminance
    *cg = -.25 * r + .50 * g - .25 * b; // Chrominance Green
    *co =  .50 * r - .50 * g;           // Chrominance Orange
}

void rgb_to_yuv(float r, float g, float b, float *y, float *u, float *v)
{
    *y = .299 * r + .587 * g + .114 * b; // Luma
    *u =-.147 * r - .289 * g + .436 * b; // Delta Blue
    *v = .615 * r - .515 * g - .100 * b; // Delta Red
}

#ifdef __cplusplus
}
#endif
