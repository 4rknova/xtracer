#ifndef XTCORE_TONEMAPPING_TONEMAPPING_H_INCLUDED
#define XTCORE_TONEMAPPING_TONEMAPPING_H_INCLUDED

#include <nimg/color.h>
#include <nimg/pixmap.h>

namespace xtcore {
namespace tonemapping {

enum operator_t
{
      OP_NONE
    , OP_REINHARD
    , OP_REINHARD_LUMINANCE
    , OP_MANTIUK_2006
    , OP_ACES_FITTED
};

struct settings_t
{
    operator_t op;
    float exposure;
    float white_point;
    float mantiuk_contrast;
    float mantiuk_saturation;
    float mantiuk_detail;

    settings_t()
        : op(OP_ACES_FITTED)
        , exposure(1.0f)
        , white_point(1.0f)
        , mantiuk_contrast(0.1f)
        , mantiuk_saturation(0.8f)
        , mantiuk_detail(1.0f)
    {}
};

nimg::ColorRGBf apply(const nimg::ColorRGBf &color, const settings_t &settings = settings_t());
void apply(nimg::Pixmap &pixmap, const settings_t &settings = settings_t());

} /* namespace tonemapping */
} /* namespace xtcore */

#endif /* XTCORE_TONEMAPPING_TONEMAPPING_H_INCLUDED */
