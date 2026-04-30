#include "post_filters.h"

#include <memory>

#include <xtcore/filter/brightness.h>
#include <xtcore/filter/chromatic_aberration.h>
#include <xtcore/filter/contrast.h>
#include <xtcore/filter/denoise.h>
#include <xtcore/filter/desaturate.h>
#include <xtcore/filter/film_grain.h>
#include <xtcore/filter/fxaa.h>
#include <xtcore/filter/raindrops_lens.h>
#include <xtcore/filter/sharpen.h>
#include <xtcore/filter/vignette.h>

namespace xtracer {
namespace frontend {
namespace web {

namespace {

static const post_filter_param_info_t k_no_params[] = {};

static const post_filter_param_info_t k_chromatic_aberration_params[] = {
      { "amount", "Amount", "float", "Channel shift amount", "1.5", "0", "64", "0.1", nullptr, 0 }
    , { "center_x", "Center X", "float", "Normalized center X", "0.5", "0", "1", "0.01", nullptr, 0 }
    , { "center_y", "Center Y", "float", "Normalized center Y", "0.5", "0", "1", "0.01", nullptr, 0 }
    , { "falloff", "Falloff", "float", "Radial falloff exponent", "1.0", "0", "8", "0.1", nullptr, 0 }
};

static const post_filter_param_info_t k_vignette_params[] = {
      { "strength", "Strength", "float", "Edge darkening strength", "0.35", "0", "1", "0.01", nullptr, 0 }
    , { "radius", "Radius", "float", "Inner unaffected radius", "0.5", "0", "1", "0.01", nullptr, 0 }
    , { "softness", "Softness", "float", "Transition softness", "0.35", "0.001", "1", "0.01", nullptr, 0 }
    , { "center_x", "Center X", "float", "Normalized center X", "0.5", "0", "1", "0.01", nullptr, 0 }
    , { "center_y", "Center Y", "float", "Normalized center Y", "0.5", "0", "1", "0.01", nullptr, 0 }
};

static const post_filter_param_info_t k_film_grain_params[] = {
      { "amount", "Amount", "float", "Grain intensity", "0.06", "0", "1", "0.01", nullptr, 0 }
    , { "size", "Size", "float", "Grain cell size", "1.0", "1", "16", "0.5", nullptr, 0 }
    , { "seed", "Seed", "int", "Noise seed", "1", "0", "1000000", "1", nullptr, 0 }
    , { "luma_weighted", "Luma Weighted (1/0)", "bool", "Bias grain by luminance", "1", "0", "1", "1", nullptr, 0 }
};

static const post_filter_param_info_t k_denoise_params[] = {
      { "strength", "Strength", "float", "Blend toward filtered result", "0.65", "0", "1", "0.01", nullptr, 0 }
    , { "radius", "Radius", "int", "Neighborhood radius", "2.0", "1", "6", "1", nullptr, 0 }
    , { "sigma", "Sigma", "float", "Range sensitivity", "0.12", "0.001", "2", "0.01", nullptr, 0 }
};

static const post_filter_param_info_t k_fxaa_params[] = {
      { "subpix", "Subpix", "float", "Subpixel blend strength", "0.75", "0", "1", "0.01", nullptr, 0 }
    , { "edge_threshold", "Edge Threshold", "float", "Main edge threshold", "0.125", "0.001", "1", "0.01", nullptr, 0 }
    , { "edge_threshold_min", "Edge Threshold Min", "float", "Minimum edge threshold", "0.031", "0.0001", "1", "0.01", nullptr, 0 }
};

static const post_filter_param_info_t k_sharpen_params[] = {
      { "amount", "Amount", "float", "Sharpen strength", "0.8", "0", "4", "0.1", nullptr, 0 }
    , { "radius", "Radius", "int", "Kernel radius", "1.0", "1", "4", "1", nullptr, 0 }
    , { "threshold", "Threshold", "float", "Minimum contrast threshold", "0.02", "0", "1", "0.01", nullptr, 0 }
};

static const post_filter_param_info_t k_brightness_params[] = {
      { "amount", "Amount", "float", "Brightness offset", "0.0", "-4", "4", "0.05", nullptr, 0 }
};

static const post_filter_param_info_t k_contrast_params[] = {
      { "amount", "Amount", "float", "Contrast scale", "1.0", "0", "4", "0.05", nullptr, 0 }
    , { "pivot", "Pivot", "float", "Contrast pivot point", "0.5", "0", "4", "0.05", nullptr, 0 }
};

static const post_filter_param_info_t k_raindrops_params[] = {
      { "density", "Density", "float", "Droplet density", "0.35", "0", "1", "0.01", nullptr, 0 }
    , { "size", "Size", "float", "Droplet size", "0.45", "0", "1", "0.01", nullptr, 0 }
    , { "distortion", "Distortion", "float", "Refraction strength", "12.0", "0", "64", "0.1", nullptr, 0 }
    , { "seed", "Seed", "int", "Pattern seed", "1", "0", "1000000", "1", nullptr, 0 }
};

struct post_filter_registry_entry_t
{
    const char *id;
    xtcore::filter::IFilter *(*create_fn)();
    const post_filter_param_info_t *params;
    size_t params_count;
    bool allow_before_tm;
    bool allow_after_tm;
};

template <typename T>
xtcore::filter::IFilter *create_filter_instance()
{
    return new T();
}

static const post_filter_registry_entry_t k_registry[] = {
      { "desaturate", &create_filter_instance<xtcore::filter::Desaturate>, k_no_params, 0, true, true }
    , { "chromatic_aberration", &create_filter_instance<xtcore::filter::ChromaticAberration>, k_chromatic_aberration_params, sizeof(k_chromatic_aberration_params) / sizeof(k_chromatic_aberration_params[0]), false, true }
    , { "vignette", &create_filter_instance<xtcore::filter::Vignette>, k_vignette_params, sizeof(k_vignette_params) / sizeof(k_vignette_params[0]), false, true }
    , { "film_grain", &create_filter_instance<xtcore::filter::FilmGrain>, k_film_grain_params, sizeof(k_film_grain_params) / sizeof(k_film_grain_params[0]), false, true }
    , { "denoise", &create_filter_instance<xtcore::filter::Denoise>, k_denoise_params, sizeof(k_denoise_params) / sizeof(k_denoise_params[0]), true, false }
    , { "fxaa", &create_filter_instance<xtcore::filter::FXAA>, k_fxaa_params, sizeof(k_fxaa_params) / sizeof(k_fxaa_params[0]), false, true }
    , { "sharpen", &create_filter_instance<xtcore::filter::Sharpen>, k_sharpen_params, sizeof(k_sharpen_params) / sizeof(k_sharpen_params[0]), true, false }
    , { "brightness", &create_filter_instance<xtcore::filter::Brightness>, k_brightness_params, sizeof(k_brightness_params) / sizeof(k_brightness_params[0]), false, true }
    , { "contrast", &create_filter_instance<xtcore::filter::Contrast>, k_contrast_params, sizeof(k_contrast_params) / sizeof(k_contrast_params[0]), false, true }
    , { "raindrops_lens", &create_filter_instance<xtcore::filter::RaindropsLens>, k_raindrops_params, sizeof(k_raindrops_params) / sizeof(k_raindrops_params[0]), false, true }
};

} // namespace

std::vector<post_filter_info_t> list_post_filters()
{
    std::vector<post_filter_info_t> out;
    out.reserve(sizeof(k_registry) / sizeof(k_registry[0]));
    for (size_t i = 0; i < sizeof(k_registry) / sizeof(k_registry[0]); ++i) {
        const post_filter_registry_entry_t &entry = k_registry[i];
        std::unique_ptr<xtcore::filter::IFilter> filter(entry.create_fn ? entry.create_fn() : nullptr);
        post_filter_info_t info;
        if (filter) info.metadata = filter->metadata();
        else info.metadata.id = entry.id ? entry.id : "";
        info.params = entry.params;
        info.params_count = entry.params_count;
        info.allow_before_tm = entry.allow_before_tm;
        info.allow_after_tm = entry.allow_after_tm;
        out.push_back(info);
    }
    return out;
}

const post_filter_info_t *find_post_filter_info(const std::string &id)
{
    static std::vector<post_filter_info_t> cache = list_post_filters();
    for (size_t i = 0; i < cache.size(); ++i) {
        if (cache[i].metadata.id == id) return &cache[i];
    }
    return nullptr;
}

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */
