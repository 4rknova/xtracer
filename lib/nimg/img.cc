#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#define TINYEXR_IMPLEMENTATION

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <stb_image.h>
#include <stb_image_write.h>
#include <tinyexr.h>
#include "img.h"
#include "conversion.h"

namespace nimg {
    namespace io {
        namespace load {

namespace {

int decode_image_buffer(const unsigned char *data, int w, int h, int bpp, Pixmap &map)
{
    if (!data || w <= 0 || h <= 0) return 2;

    map.init(w,h);

    int res = 0;

    if (bpp == 1) {
        for (size_t x = 0; x < (size_t)w; ++x) {
            for (size_t y = 0; y < (size_t)h; ++y) {
                float v = srgb_to_linear(data[y * w + x] / 255.f);
                ColorRGBAf pixel;
                pixel.r(v); pixel.g(v); pixel.b(v); pixel.a(1.f);
                map.pixel(x, y) = pixel;
            }
        }
    }
    else if (bpp == 3) {
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
    else {
        res = -1;
    }

    return res;
}

} // namespace

int image(const char *filename, Pixmap &map)
{
    if (!filename) return 1;

    int w, h, bpp;
    unsigned char *data = stbi_load(filename, &w, &h, &bpp, 0);

    if (data == NULL) return 2;

    int res = decode_image_buffer(data, w, h, bpp, map);

    stbi_image_free(data);

    return res;
}

int image_memory(const unsigned char *buffer, size_t size, Pixmap &map)
{
    if (!buffer || size == 0u) return 1;

    int w, h, bpp;
    unsigned char *data = stbi_load_from_memory(buffer, (int)size, &w, &h, &bpp, 0);
    if (data == NULL) return 2;

    const int res = decode_image_buffer(data, w, h, bpp, map);
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

bool build_rgba8_srgb_buffer(Pixmap &map, std::vector<unsigned char> &out)
{
    const int w = (int)map.width();
    const int h = (int)map.height();
    if (w < 0 || h < 0) return false;

    out.resize((size_t)w * (size_t)h * 4u);
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            const size_t idx = (size_t)(y * w + x) * 4u;
            unsigned char pixel[4];
            pack_rgba8_srgb(map.pixel(x, y), pixel);
            out[idx  ] = pixel[0];
            out[idx+1] = pixel[1];
            out[idx+2] = pixel[2];
            out[idx+3] = pixel[3];
        }
    }
    return true;
}

bool build_rgba32f_buffer(Pixmap &map, std::vector<float> &out)
{
    const int w = (int)map.width();
    const int h = (int)map.height();
    if (w < 0 || h < 0) return false;

    out.resize((size_t)w * (size_t)h * 4u);
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            const size_t idx = (size_t)(y * w + x) * 4u;
            out[idx  ] = map.pixel(x, y).r();
            out[idx+1] = map.pixel(x, y).g();
            out[idx+2] = map.pixel(x, y).b();
            out[idx+3] = map.pixel(x, y).a();
        }
    }
    return true;
}

void append_write_callback(void *context, void *data, int size)
{
    if (!context || !data || size <= 0) return;
    std::vector<unsigned char> *out = (std::vector<unsigned char> *)context;
    const unsigned char *bytes = (const unsigned char *)data;
    out->insert(out->end(), bytes, bytes + size);
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

int exr_memory(Pixmap &map, std::vector<unsigned char> &out)
{
    const int w = (int)map.width();
    const int h = (int)map.height();
    const int sz = w * h;

    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);

    image.num_channels = 3;

    std::vector<float> images[3];
    images[0].resize(sz);
    images[1].resize(sz);
    images[2].resize(sz);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const int idx = y * w + x;
            images[0][idx] = map.pixel(x, y).r();
            images[1][idx] = map.pixel(x, y).g();
            images[2][idx] = map.pixel(x, y).b();
        }
    }

    float* image_ptr[3];
    image_ptr[0] = &(images[2].at(0));
    image_ptr[1] = &(images[1].at(0));
    image_ptr[2] = &(images[0].at(0));

    image.images = (unsigned char**)image_ptr;
    image.width  = w;
    image.height = h;

    header.num_channels = 3;
    header.channels = (EXRChannelInfo *)malloc(sizeof(EXRChannelInfo) * header.num_channels);
    strncpy(header.channels[0].name, "B", 255); header.channels[0].name[strlen("B")] = '\0';
    strncpy(header.channels[1].name, "G", 255); header.channels[1].name[strlen("G")] = '\0';
    strncpy(header.channels[2].name, "R", 255); header.channels[2].name[strlen("R")] = '\0';

    header.pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
    for (int i = 0; i < header.num_channels; i++) {
        header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
    }

    const char *err = NULL;
    unsigned char *mem = NULL;
    const size_t mem_size = SaveEXRImageToMemory(&image, &header, &mem, &err);
    int res = 0;
    if (mem_size == 0 || !mem) {
        if (err) {
            printf("%s\n", err);
            FreeEXRErrorMessage(err);
        }
        res = 1;
    } else {
        out.assign(mem, mem + mem_size);
        free(mem);
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

int png_memory(Pixmap &map, std::vector<unsigned char> &out)
{
    const int w = (int)map.width();
    const int h = (int)map.height();
    std::vector<unsigned char> data;
    if (!build_rgba8_srgb_buffer(map, data)) return 1;

    int len = 0;
    unsigned char *png = stbi_write_png_to_mem(data.data(), w * 4, w, h, 4, &len);
    if (!png || len <= 0) return 1;
    out.assign(png, png + len);
    STBIW_FREE(png);
    return 0;
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

int jpg_memory(Pixmap &map, std::vector<unsigned char> &out)
{
    const int w = (int)map.width();
    const int h = (int)map.height();
    std::vector<unsigned char> data;
    if (!build_rgba8_srgb_buffer(map, data)) return 1;
    out.clear();
    const int res = stbi_write_jpg_to_func(append_write_callback, &out, w, h, 4, data.data(), 90);
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

int bmp_memory(Pixmap &map, std::vector<unsigned char> &out)
{
    std::vector<unsigned char> data;
    if (!build_rgba8_srgb_buffer(map, data)) return 1;
    out.clear();
    const int res = stbi_write_bmp_to_func(append_write_callback, &out, (int)map.width(), (int)map.height(), 4, data.data());
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

int tga_memory(Pixmap &map, std::vector<unsigned char> &out)
{
    std::vector<unsigned char> data;
    if (!build_rgba8_srgb_buffer(map, data)) return 1;
    out.clear();
    const int res = stbi_write_tga_to_func(append_write_callback, &out, (int)map.width(), (int)map.height(), 4, data.data());
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

int hdr_memory(Pixmap &map, std::vector<unsigned char> &out)
{
    std::vector<float> data;
    if (!build_rgba32f_buffer(map, data)) return 1;
    out.clear();
    const int res = stbi_write_hdr_to_func(append_write_callback, &out, (int)map.width(), (int)map.height(), 4, data.data());
    return res == 0 ? 1 : 0;
}

        } /* namespace save */

    } /* namespace io */
} /* namespace nimg */
