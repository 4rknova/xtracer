#ifndef YUV4MPEG2_H_INCLUDED
#define YUV4MPEG2_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

void start_video(const char *file, int width, int height, int frame_rate);
void write_frame(const char *file, int width, int height, float *rgb);

#ifdef __cplusplus
}
#endif

#endif /* YUV4MPEG2_H_INCLUDED */
