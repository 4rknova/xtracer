#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#define TINYEXR_IMPLEMENTATION

#include <cstdio>
#include <vector>
#include <stb_image.h>
#include <stb_image_write.h>
#include <tinyexr.h>
#include "img.h"
#include "conversion.h"

namespace nimg {
    namespace io {
        namespace load {

int image(const char *filename, Pixmap &map)
{
    if (!filename) return 1;

    int w, h, bpp;
    unsigned char *data = stbi_load(filename, &w, &h, &bpp, 0);

    if (data == NULL) return 2;

    map.init(w,h);

    int res = 0;

    if (bpp == 3) {
        for (size_t x = 0; x < (size_t)w; ++x) {
            for (size_t y = 0; y < (size_t)h; ++y) {
                size_t i = 3 * (y * w + x);

                ColorRGBAf pixel;
                pixel.r(srgb_to_linear(data[i  ] / 255.f));
                pixel.g(srgb_to_linear(data[i+1] / 255.f));
                pixel.b(srgb_to_linear(data[i+2] / 255.f));
                pixel.a(1.f);
                map.pixel(x, y) = pixel;
            }
        }
    }
    else if (bpp == 4) {
        for (size_t x = 0; x < (size_t)w; ++x) {
            for (size_t y = 0; y < (size_t)h; ++y) {
                size_t i = 4 * (y * w + x);

                ColorRGBAf pixel;
                pixel.r(srgb_to_linear(data[i  ] / 255.f));
                pixel.g(srgb_to_linear(data[i+1] / 255.f));
                pixel.b(srgb_to_linear(data[i+2] / 255.f));
                pixel.a(data[i+3] / 255.f);

                map.pixel(x, y) = pixel;
            }
        }

    }

    else  res = -1; // incompatible format

    stbi_image_free(data);

    return res;
}

        } /* namespace load */

        namespace save {

namespace {

unsigned char to_u8(float v)
{
    if (v <= 0.0f) return 0;
    if (v >= 1.0f) return 255;
    return (unsigned char)(v * 255.0f + 0.5f);
}

void pack_rgba8_srgb(const ColorRGBAf &pixel, unsigned char out[4])
{
    out[0] = to_u8(linear_to_srgb(pixel.r()));
    out[1] = to_u8(linear_to_srgb(pixel.g()));
    out[2] = to_u8(linear_to_srgb(pixel.b()));
    out[3] = to_u8(pixel.a());
}

} // namespace

int exr(const char *filename, Pixmap &map)
{
    if (!filename) return 1;

    int w = map.width();
    int h = map.height();

    int res = 0;
    int sz = w * h;

    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);

    image.num_channels = 3;

    std::vector<float> images[3];
    images[0].resize(sz);
    images[1].resize(sz);
    images[2].resize(sz);

    // Split RGBRGBRGB... into R, G, B layers
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            images[0][idx] = map.pixel(x, y).r();
            images[1][idx] = map.pixel(x, y).g();
            images[2][idx] = map.pixel(x, y).b();
        }
    }

    float* image_ptr[3];
    image_ptr[0] = &(images[2].at(0)); // B
    image_ptr[1] = &(images[1].at(0)); // G
    image_ptr[2] = &(images[0].at(0)); // R


    image.images = (unsigned char**)image_ptr;
    image.width  = w;
    image.height = h;

    header.num_channels = 3;
    header.channels = (EXRChannelInfo *)malloc(sizeof(EXRChannelInfo) * header.num_channels);
    // Must be (A)BGR order, since most of EXR viewers expect this channel order.
    strncpy(header.channels[0].name, "B", 255); header.channels[0].name[strlen("B")] = '\0';
    strncpy(header.channels[1].name, "G", 255); header.channels[1].name[strlen("G")] = '\0';
    strncpy(header.channels[2].name, "R", 255); header.channels[2].name[strlen("R")] = '\0';

    header.pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
    for (int i = 0; i < header.num_channels; i++) {
        header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of output image to be stored in .EXR
    }

    const char* err = NULL;

    int exr_res = SaveEXRImageToFile(&image, &header, filename, &err);

    if (exr_res != TINYEXR_SUCCESS) {
        printf("%s\n", err);
        FreeEXRErrorMessage(err); // free's buffer for an error message
        res = 1;
    }

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);

    return res;
}

int png(const char *filename, Pixmap &map)
{
    if (!filename) return 1;

    int w = map.width();
    int h = map.height();

    int res = 0;

    unsigned char *data = (unsigned char *)malloc(w*h*4);

    if (data != NULL) {

        for (int x = 0; x < w; ++x) {
            for (int y = 0; y < h; ++y) {
                size_t idx = (y*w+x)*4;
                unsigned char pixel[4];
                pack_rgba8_srgb(map.pixel(x, y), pixel);
                data[idx  ] = pixel[0];
                data[idx+1] = pixel[1];
                data[idx+2] = pixel[2];
                data[idx+3] = pixel[3];
            }
        }
        res = stbi_write_png(filename, w, h, 4, data, w*4);
    }

    free(data);

    return res == 0 ? 1 : 0;
}

int jpg(const char *filename, Pixmap &map)
{
    if (!filename) return 1;

    int w = map.width();
    int h = map.height();

    int res = 0;

    unsigned char *data = (unsigned char *)malloc(w*h*4);

    if (data != NULL) {

        for (int x = 0; x < w; ++x) {
            for (int y = 0; y < h; ++y) {
                size_t idx = (y*w+x)*4;
                unsigned char pixel[4];
                pack_rgba8_srgb(map.pixel(x, y), pixel);
                data[idx  ] = pixel[0];
                data[idx+1] = pixel[1];
                data[idx+2] = pixel[2];
                data[idx+3] = pixel[3];
            }
        }
        res = stbi_write_jpg(filename, w, h, 4, data, w*4);
    }

    free(data);

    return res == 0 ? 1 : 0;
}

int bmp(const char *filename, Pixmap &map)
{
    if (!filename) return 1;

    int w = map.width();
    int h = map.height();

    int res = 0;

    unsigned char *data = (unsigned char *)malloc(w*h*4);

    if (data != NULL) {

        for (int x = 0; x < w; ++x) {
            for (int y = 0; y < h; ++y) {
                size_t idx = (y*w+x)*4;
                unsigned char pixel[4];
                pack_rgba8_srgb(map.pixel(x, y), pixel);
                data[idx  ] = pixel[0];
                data[idx+1] = pixel[1];
                data[idx+2] = pixel[2];
                data[idx+3] = pixel[3];
            }
        }

        res = stbi_write_bmp(filename, w, h, 4, data);
    }

    free(data);

    return res == 0 ? 1 : 0;
}

int tga(const char *filename, Pixmap &map)
{
    if (!filename) return 1;

    int w = map.width();
    int h = map.height();

    int res = 0;

    unsigned char *data = (unsigned char *)malloc(w*h*4);

    if (data != NULL) {

        for (int x = 0; x < w; ++x) {
            for (int y = 0; y < h; ++y) {
                size_t idx = (y*w+x)*4;
                unsigned char pixel[4];
                pack_rgba8_srgb(map.pixel(x, y), pixel);
                data[idx  ] = pixel[0];
                data[idx+1] = pixel[1];
                data[idx+2] = pixel[2];
                data[idx+3] = pixel[3];
            }
        }

        res = stbi_write_tga(filename, w, h, 4, data);
    }

    free(data);

    return res == 0 ? 1 : 0;
}

int hdr(const char *filename, Pixmap &map)
{
    if (!filename) return 1;

    int w = map.width();
    int h = map.height();

    int res = 0;

    float *data = (float *)malloc(w*h*4*sizeof(float));

    if (data != NULL) {

        for (int x = 0; x < w; ++x) {
            for (int y = 0; y < h; ++y) {
                size_t idx = (y*w+x)*4;
                data[idx  ] = map.pixel(x,y).r();
                data[idx+1] = map.pixel(x,y).g();
                data[idx+2] = map.pixel(x,y).b();
                data[idx+3] = map.pixel(x,y).a();
            }
        }

        res = stbi_write_hdr(filename, w, h, 4, data);
    }

    free(data);

    return res == 0 ? 1 : 0;
}

        } /* namespace save */

    } /* namespace io */
} /* namespace nimg */
