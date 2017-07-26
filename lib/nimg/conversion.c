#include "conversion.h"

#ifdef __cplusplus
extern "C" {
#endif

void rgb_to_ycbcr(float r, float g, float b, float *y, float *cb, float *cr)
{
	float d = 128./255.;
  	*y  =     .299 * r + .587 * g + .114 * b; // Luminance
	*cb = d - .169 * r - .331 * g + .500 * b; // Chrominance Blue
	*cr = d + .500 * r - .419 * g - .081 * b; // Chrominance Red
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
