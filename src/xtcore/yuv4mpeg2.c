#include <stdio.h>
#include <nimg/conversion.h>
#include "yuv4mpeg2.h"

#ifdef __cplusplus
extern "C" {
#endif

// Format details:
// https://wiki.multimedia.cx/index.php?title=YUV4MPEG2

#define HID "YUV4MPEG2 "

void start_video(const char *file, int width, int height, int frame_rate)
{
    FILE *f = fopen(file, "wb");
    fprintf(f, "%s", HID);
    fprintf(f, "W%i ", width);
    fprintf(f, "H%i ", height);
    fprintf(f, "F%i:1", frame_rate);
    fprintf(f, " Ip A0:0");
    fprintf(f, "\n");
    fclose(f);
}

void write_frame(const char *file, int width, int height, float *rgb)
{
    FILE *f = fopen(file, "ab");
    fseek(f, 0, SEEK_END);
    fprintf(f, "FRAME\n");

    int sz = width * height * 3;

    for (int i = 0; i < sz; i+=3) {
        float r = rgb[i+0], g = rgb[i+1], b = rgb[i+2]
            , y = 0, cb = 0, cr = 0;

        rgb_to_ycbcr(r, g, b, &y, &cb, &cr);

        if (y  > 1.) y  = 1.;
        if (cb > 1.) cb = 1.;
        if (cr > 1.) cr = 1.;

        fprintf(f, "%c", (unsigned char)(y  *  255.));
    }
    for (int i = 0; i < sz; i+=3) {
        float r = rgb[i+0], g = rgb[i+1], b = rgb[i+2]
            , y = 0, cb = 0, cr = 0;

        rgb_to_ycbcr(r, g, b, &y, &cb, &cr);

        if (y  > 1.) y  = 1.;
        if (cb > 1.) cb = 1.;
        if (cr > 1.) cr = 1.;

        fprintf(f, "%c", (unsigned char)(cb  *  255.));
    }
    for (int i = 0; i < sz; i+=3) {
        float r = rgb[i+0], g = rgb[i+1], b = rgb[i+2]
            , y = 0, cb = 0, cr = 0;

        rgb_to_ycbcr(r, g, b, &y, &cb, &cr);

        if (y  > 1.) y  = 1.;
        if (cb > 1.) cb = 1.;
        if (cr > 1.) cr = 1.;

        fprintf(f, "%c", (unsigned char)(cr  *  255.));
    }
    fclose(f);
}

#ifdef __cplusplus
}
#endif
