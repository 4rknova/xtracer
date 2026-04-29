#include "pixmap_util.h"

#include <nimg/conversion.h>

namespace xtracer {
namespace frontend {
namespace web {

void encode_rgba8_srgb(const nimg::Pixmap &pixmap, std::vector<unsigned char> &out)
{
    const size_t width  = pixmap.width();
    const size_t height = pixmap.height();
    out.resize(width * height * 4);

    auto to_u8_srgb = [](float v) -> unsigned char {
        const float s = linear_to_srgb(v);
        const int i = static_cast<int>(s * 255.0f + 0.5f);
        return static_cast<unsigned char>(i < 0 ? 0 : i > 255 ? 255 : i);
    };

    for (size_t py = 0; py < height; ++py) {
        for (size_t px = 0; px < width; ++px) {
            const nimg::ColorRGBAf &c = pixmap.pixel_ro(px, py);
            const size_t off = (py * width + px) * 4;
            out[off + 0] = to_u8_srgb(c.r());
            out[off + 1] = to_u8_srgb(c.g());
            out[off + 2] = to_u8_srgb(c.b());
            out[off + 3] = 255;
        }
    }
}

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */
