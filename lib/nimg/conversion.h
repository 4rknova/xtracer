#ifndef NIMG_CONVERSION_H_INCLUDED
#define NIMG_CONVERSION_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

void rgb_to_ycbcr  (float r, float g, float b, float *y, float *cb, float *cr);
void rgb_to_ycgco  (float r, float g, float b, float *y, float *cg, float *co);
void rgb_to_yuv    (float r, float g, float b, float *y, float * u, float * v);

#ifdef __cplusplus
}
#endif

#endif /* NIMG_CONVERSION_H_INCLUDED */
