#include <stdlib.h>
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
    if (!f) return;
    fprintf(f, "%s", HID);
    fprintf(f, "W%i ", width);
    fprintf(f, "H%i ", height);
    fprintf(f, "F%i:1 ", frame_rate);
    fprintf(f, "Ip ");
    fprintf(f, "A1:1 ");
    fprintf(f, "C444");
    fprintf(f, "\n");
    fclose(f);
}

static unsigned char clamp_u8(float v)
{
    if (v < 0.0f) return 0u;
    if (v > 1.0f) return 255u;
    return (unsigned char)(v * 255.0f);
}

void write_frame(const char *file, int width, int height, const float *rgb)
{
    FILE *f = fopen(file, "ab");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    fprintf(f, "FRAME\n");

    int pc1 = width * height;
    int pc2 = pc1 * 2;
    int sz  = pc1 * 3;

    unsigned char *buffer = (unsigned char*)malloc((size_t)sz * sizeof(unsigned char));
    if (!buffer) {
        fclose(f);
        return;
    }
    for (int i = 0; i < sz; i+=3) {
        float r = rgb[i], g = rgb[i+1], b = rgb[i+2], y = 0, cb = 0, cr = 0;
        rgb_to_ycbcr(r, g, b, &y, &cb, &cr);
        int idx = i / 3;
        buffer[idx      ] = clamp_u8(y);
        buffer[idx + pc1] = clamp_u8(cb);
        buffer[idx + pc2] = clamp_u8(cr);
    }
    fwrite(buffer, sizeof(unsigned char), (size_t)sz, f);
    fclose(f);
    free(buffer);
}

#ifdef __cplusplus
}
#endif
