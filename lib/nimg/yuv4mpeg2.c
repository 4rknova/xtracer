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

#define MIN(a,b) (a > b ? b : a)

void write_frame(const char *file, int width, int height, float *rgb)
{
    FILE *f = fopen(file, "ab");
    fseek(f, 0, SEEK_END);
    fprintf(f, "FRAME\n");

    int pc1 = width * height;
    int pc2 = pc1 * 2;
    int sz  = pc1 * 3;

    unsigned char *buffer = (char*)malloc(sz*sizeof(unsigned char));
    for (int i = 0; i < sz; i+=3) {
        float r = rgb[i], g = rgb[i+1], b = rgb[i+2], y = 0, cb = 0, cr = 0;
        rgb_to_ycbcr(r, g, b, &y, &cb, &cr);
        int idx = i / 3;
        buffer[idx      ] = MIN(255, (char)(y  * 255.));
        buffer[idx + pc1] = MIN(255, (char)(cb * 255.));
        buffer[idx + pc2] = MIN(255, (char)(cr * 255.));
    }
    fwrite(buffer, sizeof(char), sz, f);
    fclose(f);
    free(buffer);
}

#ifdef __cplusplus
}
#endif
