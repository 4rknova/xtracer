#include "job_manager.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <iterator>
#include <sstream>
#include <thread>
#include <chrono>
#include <unistd.h>

#include <nimg/img.h>
#include <nimg/conversion.h>
#include <xtcore/filter/postfx.h>
#include <xtcore/filter/desaturate.h>
#include <xtcore/tonemapping/tonemapping.h>

#include "backend_log.h"
#include "pixmap_util.h"
#include "post_filters.h"

namespace xtracer {
namespace frontend {
namespace web {

namespace {

static uint32_t fnv1a_32(const std::string &s)
{
    uint32_t h = 2166136261u;
    for (unsigned char c : s) {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}

static std::string make_gallery_entry_id(long long created_at_ms, const std::string &scene_name)
{
    const time_t t = static_cast<time_t>(created_at_ms / 1000);
    const int ms   = static_cast<int>(created_at_ms % 1000);
    struct tm tm_buf = {};
    gmtime_r(&t, &tm_buf);
    char ts_buf[32];
    std::strftime(ts_buf, sizeof(ts_buf), "%Y%m%d-%H%M%S", &tm_buf);
    char id_buf[64];
    std::snprintf(id_buf, sizeof(id_buf), "%s-%03d_%08x", ts_buf, ms, fnv1a_32(scene_name));
    return std::string(id_buf);
}

bool read_file_bytes(const char *path, std::vector<unsigned char> &out)
{
    std::ifstream in(path, std::ios::binary);
    if (!in.good()) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

bool write_file_bytes(const char *path, const std::vector<unsigned char> &bytes)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.good()) return false;
    if (!bytes.empty()) {
        out.write((const char *)bytes.data(), (std::streamsize)bytes.size());
    }
    return out.good();
}

struct post_filter_entry_t
{
    bool before_tm;
    std::string id;
    float ca_amount;
    float ca_center_x;
    float ca_center_y;
    float ca_falloff;
    float vignette_strength;
    float vignette_radius;
    float vignette_softness;
    float vignette_center_x;
    float vignette_center_y;
    float grain_amount;
    float grain_size;
    float grain_seed;
    float grain_luma_weighted;
    float denoise_strength;
    float denoise_radius;
    float denoise_sigma;
    float fxaa_subpix;
    float fxaa_edge_threshold;
    float fxaa_edge_threshold_min;
    float sharpen_amount;
    float sharpen_radius;
    float sharpen_threshold;
    float brightness_amount;
    float contrast_amount;
    float contrast_pivot;
    float raindrops_density;
    float raindrops_size;
    float raindrops_distortion;
    float raindrops_seed;
};

typedef std::vector<post_filter_entry_t> post_filter_chain_t;

std::string lower_ascii_copy(std::string s)
{
    for (size_t i = 0; i < s.size(); ++i) {
        s[i] = (char)std::tolower((unsigned char)s[i]);
    }
    return s;
}

std::string trim_ascii_copy(const std::string &s)
{
    size_t b = 0;
    size_t e = s.size();
    while (b < e && std::isspace((unsigned char)s[b])) ++b;
    while (e > b && std::isspace((unsigned char)s[e - 1])) --e;
    return s.substr(b, e - b);
}

bool parse_post_filter_chain(bool enabled,
                             const std::string &raw,
                             post_filter_chain_t &out_chain,
                             std::string &out_normalized_key)
{
    out_chain.clear();
    out_normalized_key.clear();
    if (!enabled) return true;

    const std::string trimmed_all = trim_ascii_copy(raw);
    if (trimmed_all.empty()) return true;

    size_t start = 0;
    while (start <= trimmed_all.size()) {
        size_t comma = trimmed_all.find(',', start);
        const std::string part = (comma == std::string::npos)
            ? trimmed_all.substr(start)
            : trimmed_all.substr(start, comma - start);
        const std::string token = lower_ascii_copy(trim_ascii_copy(part));
        if (!token.empty()) {
            std::vector<std::string> tokens;
            size_t tstart = 0;
            while (tstart <= token.size()) {
                size_t tsep = token.find(':', tstart);
                if (tsep == std::string::npos) {
                    tokens.push_back(token.substr(tstart));
                    break;
                }
                tokens.push_back(token.substr(tstart, tsep - tstart));
                tstart = tsep + 1;
            }
            if (tokens.empty()) return false;

            size_t idx = 0;
            bool before_tm = false;
            if (tokens[0] == "before" || tokens[0] == "after") {
                before_tm = (tokens[0] == "before");
                idx = 1;
            }
            if (idx >= tokens.size()) return false;
            const std::string filter_id = trim_ascii_copy(tokens[idx]);
            idx += 1;
            const post_filter_info_t *filter_info = find_post_filter_info(filter_id);
            if (!filter_info) return false;
            if ((before_tm && !filter_info->allow_before_tm) || (!before_tm && !filter_info->allow_after_tm)) {
                return false;
            }

            post_filter_entry_t e;
            e.before_tm = before_tm;
            e.id = filter_id;
            e.ca_amount = 1.5f;
            e.ca_center_x = 0.5f;
            e.ca_center_y = 0.5f;
            e.ca_falloff = 1.0f;
            e.vignette_strength = 0.35f;
            e.vignette_radius = 0.5f;
            e.vignette_softness = 0.35f;
            e.vignette_center_x = 0.5f;
            e.vignette_center_y = 0.5f;
            e.grain_amount = 0.06f;
            e.grain_size = 1.0f;
            e.grain_seed = 1.0f;
            e.grain_luma_weighted = 1.0f;
            e.denoise_strength = 0.65f;
            e.denoise_radius = 2.0f;
            e.denoise_sigma = 0.12f;
            e.fxaa_subpix = 0.75f;
            e.fxaa_edge_threshold = 0.125f;
            e.fxaa_edge_threshold_min = 0.0312f;
            e.sharpen_amount = 0.8f;
            e.sharpen_radius = 1.0f;
            e.sharpen_threshold = 0.02f;
            e.brightness_amount = 0.0f;
            e.contrast_amount = 1.0f;
            e.contrast_pivot = 0.5f;
            e.raindrops_density = 0.35f;
            e.raindrops_size = 0.45f;
            e.raindrops_distortion = 12.0f;
            e.raindrops_seed = 1.0f;

            for (; idx < tokens.size(); ++idx) {
                const std::string kv = trim_ascii_copy(tokens[idx]);
                if (kv.empty()) continue;
                const size_t eq = kv.find('=');
                if (eq == std::string::npos) return false;
                const std::string key = trim_ascii_copy(kv.substr(0, eq));
                const std::string val = trim_ascii_copy(kv.substr(eq + 1));
                if (key.empty() || val.empty()) return false;

                std::istringstream vs(val);
                float f = 0.0f;
                vs >> f;
                if (vs.fail()) return false;

                if (filter_id == "chromatic_aberration") {
                    if (key == "amount") e.ca_amount = std::max(0.0f, std::min(64.0f, f));
                    else if (key == "center_x") e.ca_center_x = std::max(0.0f, std::min(1.0f, f));
                    else if (key == "center_y") e.ca_center_y = std::max(0.0f, std::min(1.0f, f));
                    else if (key == "falloff") e.ca_falloff = std::max(0.0f, std::min(8.0f, f));
                    else return false;
                } else if (filter_id == "vignette") {
                    if (key == "strength") e.vignette_strength = std::max(0.0f, std::min(1.0f, f));
                    else if (key == "radius") e.vignette_radius = std::max(0.0f, std::min(1.0f, f));
                    else if (key == "softness") e.vignette_softness = std::max(0.001f, std::min(1.0f, f));
                    else if (key == "center_x") e.vignette_center_x = std::max(0.0f, std::min(1.0f, f));
                    else if (key == "center_y") e.vignette_center_y = std::max(0.0f, std::min(1.0f, f));
                    else return false;
                } else if (filter_id == "film_grain") {
                    if (key == "amount") e.grain_amount = std::max(0.0f, std::min(1.0f, f));
                    else if (key == "size") e.grain_size = std::max(1.0f, std::min(16.0f, f));
                    else if (key == "seed") e.grain_seed = std::max(0.0f, std::min(1000000.0f, std::floor(f)));
                    else if (key == "luma_weighted") e.grain_luma_weighted = (f >= 0.5f) ? 1.0f : 0.0f;
                    else return false;
                } else if (filter_id == "denoise") {
                    if (key == "strength") e.denoise_strength = std::max(0.0f, std::min(1.0f, f));
                    else if (key == "radius") e.denoise_radius = std::max(1.0f, std::min(6.0f, std::round(f)));
                    else if (key == "sigma") e.denoise_sigma = std::max(0.001f, std::min(2.0f, f));
                    else return false;
                } else if (filter_id == "fxaa") {
                    if (key == "subpix") e.fxaa_subpix = std::max(0.0f, std::min(1.0f, f));
                    else if (key == "edge_threshold") e.fxaa_edge_threshold = std::max(0.001f, std::min(1.0f, f));
                    else if (key == "edge_threshold_min") e.fxaa_edge_threshold_min = std::max(0.0001f, std::min(1.0f, f));
                    else return false;
                } else if (filter_id == "sharpen") {
                    if (key == "amount") e.sharpen_amount = std::max(0.0f, std::min(4.0f, f));
                    else if (key == "radius") e.sharpen_radius = std::max(1.0f, std::min(4.0f, std::round(f)));
                    else if (key == "threshold") e.sharpen_threshold = std::max(0.0f, std::min(1.0f, f));
                    else return false;
                } else if (filter_id == "brightness") {
                    if (key == "amount") e.brightness_amount = std::max(-4.0f, std::min(4.0f, f));
                    else return false;
                } else if (filter_id == "contrast") {
                    if (key == "amount") e.contrast_amount = std::max(0.0f, std::min(4.0f, f));
                    else if (key == "pivot") e.contrast_pivot = std::max(0.0f, std::min(4.0f, f));
                    else return false;
                } else if (filter_id == "raindrops_lens") {
                    if (key == "density") e.raindrops_density = std::max(0.0f, std::min(1.0f, f));
                    else if (key == "size") e.raindrops_size = std::max(0.0f, std::min(1.0f, f));
                    else if (key == "distortion") e.raindrops_distortion = std::max(0.0f, std::min(64.0f, f));
                    else if (key == "seed") e.raindrops_seed = std::max(0.0f, std::min(1000000.0f, std::floor(f)));
                    else return false;
                } else if (filter_id == "desaturate") {
                    return false;
                } else {
                    return false;
                }
            }

            if (filter_id != "desaturate"
                && filter_id != "chromatic_aberration"
                && filter_id != "vignette"
                && filter_id != "film_grain"
                && filter_id != "denoise"
                && filter_id != "fxaa"
                && filter_id != "sharpen"
                && filter_id != "brightness"
                && filter_id != "contrast"
                && filter_id != "raindrops_lens") {
                return false;
            }

            out_chain.push_back(e);

            if (!out_normalized_key.empty()) out_normalized_key += ",";
            out_normalized_key += (before_tm ? "before:" : "after:");
            out_normalized_key += filter_id;
            if (filter_id == "chromatic_aberration") {
                std::ostringstream ps;
                ps << std::fixed << std::setprecision(3)
                   << ":amount=" << e.ca_amount
                   << ":center_x=" << e.ca_center_x
                   << ":center_y=" << e.ca_center_y
                   << ":falloff=" << e.ca_falloff;
                out_normalized_key += ps.str();
            } else if (filter_id == "vignette") {
                std::ostringstream ps;
                ps << std::fixed << std::setprecision(3)
                   << ":strength=" << e.vignette_strength
                   << ":radius=" << e.vignette_radius
                   << ":softness=" << e.vignette_softness
                   << ":center_x=" << e.vignette_center_x
                   << ":center_y=" << e.vignette_center_y;
                out_normalized_key += ps.str();
            } else if (filter_id == "film_grain") {
                std::ostringstream ps;
                ps << std::fixed << std::setprecision(3)
                   << ":amount=" << e.grain_amount
                   << ":size=" << e.grain_size
                   << ":seed=" << e.grain_seed
                   << ":luma_weighted=" << e.grain_luma_weighted;
                out_normalized_key += ps.str();
            } else if (filter_id == "denoise") {
                std::ostringstream ps;
                ps << std::fixed << std::setprecision(3)
                   << ":strength=" << e.denoise_strength
                   << ":radius=" << e.denoise_radius
                   << ":sigma=" << e.denoise_sigma;
                out_normalized_key += ps.str();
            } else if (filter_id == "fxaa") {
                std::ostringstream ps;
                ps << std::fixed << std::setprecision(3)
                   << ":subpix=" << e.fxaa_subpix
                   << ":edge_threshold=" << e.fxaa_edge_threshold
                   << ":edge_threshold_min=" << e.fxaa_edge_threshold_min;
                out_normalized_key += ps.str();
            } else if (filter_id == "sharpen") {
                std::ostringstream ps;
                ps << std::fixed << std::setprecision(3)
                   << ":amount=" << e.sharpen_amount
                   << ":radius=" << e.sharpen_radius
                   << ":threshold=" << e.sharpen_threshold;
                out_normalized_key += ps.str();
            } else if (filter_id == "brightness") {
                std::ostringstream ps;
                ps << std::fixed << std::setprecision(3)
                   << ":amount=" << e.brightness_amount;
                out_normalized_key += ps.str();
            } else if (filter_id == "contrast") {
                std::ostringstream ps;
                ps << std::fixed << std::setprecision(3)
                   << ":amount=" << e.contrast_amount
                   << ":pivot=" << e.contrast_pivot;
                out_normalized_key += ps.str();
            } else if (filter_id == "raindrops_lens") {
                std::ostringstream ps;
                ps << std::fixed << std::setprecision(3)
                   << ":density=" << e.raindrops_density
                   << ":size=" << e.raindrops_size
                   << ":distortion=" << e.raindrops_distortion
                   << ":seed=" << e.raindrops_seed;
                out_normalized_key += ps.str();
            }
        }

        if (comma == std::string::npos) break;
        start = comma + 1;
    }

    return true;
}

void apply_post_filters_for_stage(nimg::Pixmap &pixmap,
                                  const post_filter_chain_t &chain,
                                  bool before_tm)
{
    for (size_t i = 0; i < chain.size(); ++i) {
        const post_filter_entry_t &e = chain[i];
        if (e.before_tm != before_tm) continue;
        if (e.id == "desaturate") {
            xtcore::filter::Desaturate op;
            op.render(&pixmap);
        } else if (e.id == "chromatic_aberration") {
            xtcore::filter::ChromaticAberration op;
            op.amount = e.ca_amount;
            op.center_x = e.ca_center_x;
            op.center_y = e.ca_center_y;
            op.falloff = e.ca_falloff;
            op.render(&pixmap);
        } else if (e.id == "vignette") {
            xtcore::filter::Vignette op;
            op.strength = e.vignette_strength;
            op.radius = e.vignette_radius;
            op.softness = e.vignette_softness;
            op.center_x = e.vignette_center_x;
            op.center_y = e.vignette_center_y;
            op.render(&pixmap);
        } else if (e.id == "film_grain") {
            xtcore::filter::FilmGrain op;
            op.amount = e.grain_amount;
            op.size = e.grain_size;
            op.seed = e.grain_seed;
            op.luma_weighted = e.grain_luma_weighted;
            op.render(&pixmap);
        } else if (e.id == "denoise") {
            xtcore::filter::Denoise op;
            op.strength = e.denoise_strength;
            op.radius = e.denoise_radius;
            op.sigma = e.denoise_sigma;
            op.render(&pixmap);
        } else if (e.id == "fxaa") {
            xtcore::filter::FXAA op;
            op.subpix = e.fxaa_subpix;
            op.edge_threshold = e.fxaa_edge_threshold;
            op.edge_threshold_min = e.fxaa_edge_threshold_min;
            op.render(&pixmap);
        } else if (e.id == "sharpen") {
            xtcore::filter::Sharpen op;
            op.amount = e.sharpen_amount;
            op.radius = e.sharpen_radius;
            op.threshold = e.sharpen_threshold;
            op.render(&pixmap);
        } else if (e.id == "brightness") {
            xtcore::filter::Brightness op;
            op.amount = e.brightness_amount;
            op.render(&pixmap);
        } else if (e.id == "contrast") {
            xtcore::filter::Contrast op;
            op.amount = e.contrast_amount;
            op.pivot = e.contrast_pivot;
            op.render(&pixmap);
        } else if (e.id == "raindrops_lens") {
            xtcore::filter::RaindropsLens op;
            op.density = e.raindrops_density;
            op.size = e.raindrops_size;
            op.distortion = e.raindrops_distortion;
            op.seed = e.raindrops_seed;
            op.render(&pixmap);
        }
    }
}

bool encode_rgba_memory(nimg::Pixmap &pixmap,
                        const xtcore::tonemapping::settings_t &tm_settings,
                        const post_filter_chain_t &post_filters,
                        std::vector<unsigned char> &out)
{
    nimg::Pixmap work = pixmap;
    apply_post_filters_for_stage(work, post_filters, true);
    xtcore::tonemapping::apply(work, tm_settings);
    apply_post_filters_for_stage(work, post_filters, false);
    encode_rgba8_srgb(work, out);
    return !out.empty();
}

bool encode_jpg_memory(nimg::Pixmap &pixmap,
                       const post_filter_chain_t &post_filters,
                       std::vector<unsigned char> &out)
{
    nimg::Pixmap work = pixmap;
    apply_post_filters_for_stage(work, post_filters, true);
    xtcore::tonemapping::apply(work);
    apply_post_filters_for_stage(work, post_filters, false);
    return nimg::io::save::jpg_memory(work, out) == 0;
}

void build_preview_pixmap(nimg::Pixmap &pixmap,
                          const xtcore::tonemapping::settings_t &tm_settings,
                          const post_filter_chain_t &post_filters)
{
    apply_post_filters_for_stage(pixmap, post_filters, true);
    xtcore::tonemapping::apply(pixmap, tm_settings);
    apply_post_filters_for_stage(pixmap, post_filters, false);
}

void copy_tile_to_framebuffer(const xtcore::render::tile_t *tile, nimg::Pixmap &fb)
{
    if (!tile) return;
    nimg::ColorRGBAf col;
    for (size_t y = tile->y0(); y < tile->y1(); ++y) {
        for (size_t x = tile->x0(); x < tile->x1(); ++x) {
            tile->read(x, y, col);
            fb.pixel(x, y) = col;
        }
    }
}

void copy_rect_from_framebuffer(nimg::Pixmap &src,
                                size_t x0, size_t y0, size_t x1, size_t y1,
                                nimg::Pixmap &dst)
{
    if (x1 <= x0 || y1 <= y0) return;
    const size_t max_w = std::min(src.width(), dst.width());
    const size_t max_h = std::min(src.height(), dst.height());
    if (x0 >= max_w || y0 >= max_h) return;
    const size_t cx1 = std::min(x1, max_w);
    const size_t cy1 = std::min(y1, max_h);
    for (size_t y = y0; y < cy1; ++y) {
        for (size_t x = x0; x < cx1; ++x) {
            dst.pixel(x, y) = src.pixel(x, y);
        }
    }
}

bool extract_rect_from_framebuffer(const nimg::Pixmap &src,
                                   size_t x0, size_t y0, size_t x1, size_t y1,
                                   nimg::Pixmap &dst)
{
    if (x1 <= x0 || y1 <= y0) return false;
    if (x0 >= src.width() || y0 >= src.height()) return false;

    const size_t cx1 = std::min(x1, src.width());
    const size_t cy1 = std::min(y1, src.height());
    if (cx1 <= x0 || cy1 <= y0) return false;

    dst.init(cx1 - x0, cy1 - y0);
    for (size_t y = y0; y < cy1; ++y) {
        for (size_t x = x0; x < cx1; ++x) {
            dst.pixel(x - x0, y - y0) = src.pixel_ro(x, y);
        }
    }
    return true;
}

size_t post_filter_padding_pixels(const post_filter_chain_t &chain)
{
    float max_radius = 0.0f;
    for (size_t i = 0; i < chain.size(); ++i) {
        const post_filter_entry_t &e = chain[i];
        if (e.id == "chromatic_aberration") {
            max_radius = std::max(max_radius, std::max(0.0f, e.ca_amount));
        } else if (e.id == "denoise") {
            max_radius = std::max(max_radius, std::max(0.0f, e.denoise_radius));
        } else if (e.id == "fxaa") {
            max_radius = std::max(max_radius, 1.0f);
        } else if (e.id == "raindrops_lens") {
            max_radius = std::max(max_radius, std::max(0.0f, e.raindrops_distortion));
        } else if (e.id == "sharpen") {
            max_radius = std::max(max_radius, std::max(0.0f, e.sharpen_radius));
        }
    }
    return (size_t)std::ceil(max_radius);
}

void expand_rect_with_padding(size_t width,
                              size_t height,
                              size_t padding,
                              size_t &x0,
                              size_t &y0,
                              size_t &x1,
                              size_t &y1)
{
    if (width == 0 || height == 0 || padding == 0) return;
    x0 = (x0 > padding) ? (x0 - padding) : 0;
    y0 = (y0 > padding) ? (y0 - padding) : 0;
    x1 = std::min(width, x1 + padding);
    y1 = std::min(height, y1 + padding);
}

unsigned long long parse_job_sequence(const std::string &id)
{
    if (id.size() <= 4 || id.substr(0, 4) != "job_") return 0;
    unsigned long long v = 0;
    for (size_t i = 4; i < id.size(); ++i) {
        const char c = id[i];
        if (c < '0' || c > '9') return 0;
        v = v * 10ULL + (unsigned long long)(c - '0');
    }
    return v;
}

const char *render_mode_label(common::render_request_t::render_mode_t mode)
{
    switch (mode) {
        case common::render_request_t::RENDER_MODE_PROGRESSIVE: return "progressive";
        case common::render_request_t::RENDER_MODE_INCREMENTAL: return "incremental";
        case common::render_request_t::RENDER_MODE_INTERACTIVE: return "interactive";
        case common::render_request_t::RENDER_MODE_DIRECT:
        default: return "direct";
    }
}

size_t progressive_pass_sample_step(size_t total_samples)
{
    if (total_samples <= 4) return 1;
    if (total_samples <= 16) return 2;
    if (total_samples <= 64) return 4;
    return 8;
}

size_t progressive_pass_count(size_t total_samples)
{
    const size_t samples = (total_samples > 0) ? total_samples : 1;
    const size_t step = progressive_pass_sample_step(samples);
    if (samples <= 1) return 1;
    const size_t rem = samples - 1;
    return 1 + ((rem + step - 1) / step);
}

size_t incremental_pass_count(size_t total_samples)
{
    return (total_samples > 0) ? total_samples : 1;
}

} // namespace

job_manager_t::job_t::active_tile_key_t job_manager_t::make_active_tile_key(size_t x0, size_t y0, size_t x1, size_t y1)
{
    job_t::active_tile_key_t key;
    key.x0 = x0;
    key.y0 = y0;
    key.x1 = x1;
    key.y1 = y1;
    return key;
}

job_manager_t::job_t::active_tile_key_t job_manager_t::make_active_tile_key(const xtcore::render::tile_t *tile,
                                                                             const common::progress_tile_update_t *upd)
{
    const bool has_upd_rect = upd && upd->has_rect;
    return make_active_tile_key(has_upd_rect ? upd->x0 : tile->x0(),
                                has_upd_rect ? upd->y0 : tile->y0(),
                                has_upd_rect ? upd->x1 : tile->x1(),
                                has_upd_rect ? upd->y1 : tile->y1());
}

void job_manager_t::add_active_tile(job_t &job, const job_t::active_tile_key_t &key)
{
    if (job.active_tile_index.find(key) != job.active_tile_index.end()) return;

    job_snapshot_t::tile_rect_t rect;
    rect.x0 = key.x0;
    rect.y0 = key.y0;
    rect.x1 = key.x1;
    rect.y1 = key.y1;
    job.active_tiles.push_back(rect);
    job.active_tile_index[key] = job.active_tiles.size() - 1;
}

void job_manager_t::remove_active_tile(job_t &job, const job_t::active_tile_key_t &key)
{
    std::map<job_t::active_tile_key_t, size_t>::iterator it = job.active_tile_index.find(key);
    if (it == job.active_tile_index.end()) return;

    const size_t idx = it->second;
    const size_t last_idx = job.active_tiles.size() - 1;
    if (idx != last_idx) {
        job.active_tiles[idx] = job.active_tiles[last_idx];
        const job_snapshot_t::tile_rect_t &moved = job.active_tiles[idx];
        job.active_tile_index[make_active_tile_key(moved.x0, moved.y0, moved.x1, moved.y1)] = idx;
    }
    job.active_tiles.pop_back();
    job.active_tile_index.erase(it);
}

job_manager_t::job_t::job_t()
    : mut()
    , id()
    , workspace_id()
    , owner_client_id()
    , scene()
    , integrator()
    , state(JOB_QUEUED)
    , cancel_requested(false)
    , tiles_done(0)
    , tiles_total(0)
    , error()
    , elapsed_ms(0.0)
    , started_at()
    , has_started(false)
    , final_fb()
    , image_exr()
    , image_hdr()
    , image_jpg()
    , image_bmp()
    , image_tga()
    , progressive_fb()
    , finished_tiles()
    , photon_diffuse_points()
    , photon_caustic_points()
    , active_tiles()
    , progressive_ready(false)
    , preview_last_encoded_done(0)
    , preview_last_from_final(false)
    , preview_last_tm_op(xtcore::tonemapping::OP_ACES_FITTED)
    , preview_last_tm_exposure(1.0f)
    , preview_last_tm_white_point(1.0f)
    , preview_last_tm_mantiuk_contrast(0.1f)
    , preview_last_tm_mantiuk_saturation(0.8f)
    , preview_last_tm_mantiuk_detail(1.0f)
    , preview_last_post_filters_enabled(false)
    , preview_last_post_filters()
    , preview_rgba_cache()
    , effective_threads(0)
    , request()
    , cleanup_scene_path()
{}

job_manager_t::job_manager_t()
    : gallery_manager_(nullptr)
    , jobs_mut()
    , jobs()
    , completed_job_order()
    , max_completed_jobs(8)
    , evicted_jobs()
    , evicted_job_order()
    , max_evicted_jobs(64)
    , max_queued_jobs(32)
    , next_id(0)
    , render_slots_mut()
    , render_slots_cv()
    , max_concurrent_renders(1)
    , active_renders(0)
    , render_thread_budget(1)
    , active_render_threads(0)
    , queued_job_order()
    , stopping_(false)
{}

job_manager_t::~job_manager_t()
{
    shutdown();
}

void job_manager_t::shutdown()
{
    if (stopping_.exchange(true)) return;

    size_t n_running = 0;
    size_t n_queued  = 0;
    {
        std::lock_guard<std::mutex> lock(jobs_mut);
        for (auto &kv : jobs) {
            const job_state_t s = kv.second->state.load();
            if (s == JOB_RUNNING || s == JOB_ABORTING) ++n_running;
            else if (s == JOB_QUEUED || s == JOB_PREPARING) ++n_queued;
            kv.second->cancel_requested.store(true);
        }
    }

    if (n_running > 0 || n_queued > 0) {
        backend_log_t::handle().add("info",
            "shutdown: cancelling " + std::to_string(n_running) + " running, "
            + std::to_string(n_queued) + " queued job(s)");
    } else {
        backend_log_t::handle().add("info", "shutdown: no active jobs");
    }

    // Wait for every detached render thread to finish.  Each thread
    // decrements active_renders (via render_slot_guard) as its very last
    // action, so reaching 0 guarantees no thread is still touching *this.
    {
        std::unique_lock<std::mutex> lock(render_slots_mut);
        const bool all_stopped = render_slots_cv.wait_for(
            lock,
            std::chrono::seconds(30),
            [this]() { return active_renders == 0; });
        if (!all_stopped) {
            backend_log_t::handle().add("warn",
                "shutdown: timed out waiting for render threads, exiting anyway");
        }
    }

    backend_log_t::handle().add("info", "shutdown: all render threads stopped");
}

void job_manager_t::set_gallery_manager(gallery_manager_t *gm)
{
    gallery_manager_ = gm;
}

void job_manager_t::set_max_concurrent_renders(size_t max_concurrent)
{
    if (max_concurrent == 0) max_concurrent = 1;
    {
        std::lock_guard<std::mutex> lock(render_slots_mut);
        max_concurrent_renders = max_concurrent;
        render_slots_cv.notify_all();
    }
    dispatch_queued_jobs();
}

void job_manager_t::set_push_callback(
    std::function<void(const std::string &job_id,
                       const job_snapshot_t &snap,
                       const std::vector<unsigned char> &tile_xtdr)> cb)
{
    push_callback_ = std::move(cb);
}

void job_manager_t::set_render_thread_budget(size_t max_threads)
{
    if (max_threads == 0) max_threads = 1;
    {
        std::lock_guard<std::mutex> lock(render_slots_mut);
        render_thread_budget = max_threads;
        render_slots_cv.notify_all();
    }
    dispatch_queued_jobs();
}

size_t job_manager_t::get_max_concurrent_renders() const
{
    std::lock_guard<std::mutex> lock(render_slots_mut);
    return max_concurrent_renders;
}

size_t job_manager_t::get_active_render_count() const
{
    std::lock_guard<std::mutex> lock(render_slots_mut);
    return active_renders;
}

std::string job_manager_t::create(const common::render_request_t &request,
                                  const std::string &scene_name,
                                  const std::string &workspace_id,
                                  const std::string &owner_client_id,
                                  const std::string &cleanup_scene_path,
                                  const xtcore::tonemapping::settings_t &initial_tm_settings)
{
    std::shared_ptr<job_t> job(new job_t());
    unsigned long long id = ++next_id;

    std::ostringstream ss;
    ss << "job_" << id;
    job->id = ss.str();
    job->workspace_id = workspace_id;
    job->owner_client_id = owner_client_id;
    job->scene = scene_name;
    job->integrator = request.integrator;
    job->request = request;
    job->cleanup_scene_path = cleanup_scene_path;
    job->live_tm_settings = initial_tm_settings;

    {
        std::lock_guard<std::mutex> lock(jobs_mut);
        jobs[job->id] = job;
    }
    bool queue_full = false;
    {
        std::lock_guard<std::mutex> lock(render_slots_mut);
        if (queued_job_order.size() >= max_queued_jobs) {
            queue_full = true;
        } else {
            queued_job_order.push_back(job->id);
            render_slots_cv.notify_all();
        }
    }
    if (queue_full) {
        std::lock_guard<std::mutex> lock(jobs_mut);
        jobs.erase(job->id);
        return "";
    }
    dispatch_queued_jobs();

    std::ostringstream log;
    log << "job accepted id=" << job->id
        << " workspace=" << workspace_id
        << " client=" << owner_client_id
        << " scene=" << scene_name
        << " integrator=" << request.integrator
        << " mode=" << render_mode_label(request.render_mode);
    backend_log_t::handle().add("info", log.str());

    return job->id;
}

void job_manager_t::dispatch_queued_jobs()
{
    if (stopping_.load()) return;
    std::vector<scheduled_job_t> to_start;
    while (true) {
        std::string next_id;
        {
            std::lock_guard<std::mutex> lock(render_slots_mut);
            if (active_renders >= max_concurrent_renders) break;
            if (queued_job_order.empty()) break;
            next_id = queued_job_order.front();
        }

        std::shared_ptr<job_t> job = get_job(next_id);
        if (!job) {
            std::lock_guard<std::mutex> lock(render_slots_mut);
            if (!queued_job_order.empty() && queued_job_order.front() == next_id) {
                queued_job_order.pop_front();
            }
            continue;
        }

        if (job->cancel_requested.load()) {
            std::lock_guard<std::mutex> lock(render_slots_mut);
            if (!queued_job_order.empty() && queued_job_order.front() == next_id) {
                queued_job_order.pop_front();
            }
            continue;
        }

        size_t granted_threads = 1;
        {
            std::lock_guard<std::mutex> lock(render_slots_mut);
            if (active_renders >= max_concurrent_renders) break;
            if (queued_job_order.empty() || queued_job_order.front() != next_id) continue;

            const size_t requested_threads = job->request.threads;
            if (requested_threads > 0) {
                if (active_render_threads + requested_threads > render_thread_budget) break;
                granted_threads = requested_threads;
            } else {
                if (active_render_threads >= render_thread_budget) break;
                const size_t free_threads = (render_thread_budget > active_render_threads)
                    ? (render_thread_budget - active_render_threads)
                    : 0;
                granted_threads = (free_threads > 0) ? free_threads : 1;
            }
            queued_job_order.pop_front();
            ++active_renders;
            active_render_threads += granted_threads;
            render_slots_cv.notify_all();
        }

        {
            std::lock_guard<std::mutex> lock(job->mut);
            job->state = JOB_PREPARING;
            job->effective_threads = granted_threads;
        }

        scheduled_job_t entry;
        entry.job = job;
        entry.granted_threads = granted_threads;
        to_start.push_back(entry);
    }

    for (size_t i = 0; i < to_start.size(); ++i) {
        std::thread t(&job_manager_t::run, this, to_start[i].job, to_start[i].granted_threads);
        t.detach();
    }
}

void job_manager_t::release_render_slots(size_t released_threads)
{
    {
        std::lock_guard<std::mutex> lock(render_slots_mut);
        if (active_renders > 0) --active_renders;
        if (active_render_threads >= released_threads) active_render_threads -= released_threads;
        else active_render_threads = 0;
        render_slots_cv.notify_all();
    }
    dispatch_queued_jobs();
}

void job_manager_t::run(const std::shared_ptr<job_t> &job, size_t granted_threads)
{
    if (!job) {
        release_render_slots(granted_threads);
        return;
    }

    struct render_slot_guard_t {
        job_manager_t *owner;
        size_t release_threads;

        render_slot_guard_t(job_manager_t *o, size_t rt)
            : owner(o)
            , release_threads(rt)
        {}

        ~render_slot_guard_t()
        {
            if (owner) owner->release_render_slots(release_threads);
        }
    } render_slot_guard(this, granted_threads);

    if (job->cancel_requested.load()) {
        std::lock_guard<std::mutex> lock(job->mut);
        job->error = "render aborted";
        job->state = JOB_ABORTED;
        backend_log_t::handle().add("info", "job aborted id=" + job->id + " state=queued");
        on_job_finished(job->id);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(job->mut);
        if (job->progressive_fb.init(job->request.width, job->request.height) != 0) {
            job->error = "failed to allocate progressive framebuffer";
            job->state = JOB_ERROR;
        } else {
            for (size_t y = 0; y < job->request.height; ++y) {
                for (size_t x = 0; x < job->request.width; ++x) {
                    // Start unfinished pixels as fully transparent in progressive previews.
                    job->progressive_fb.pixel(x, y) = nimg::ColorRGBAf(0, 0, 0, 0);
                }
            }
            job->started_at = std::chrono::steady_clock::now();
            job->has_started = true;
            job->effective_threads = granted_threads;
            job->state = JOB_PREPARING;
        }
    }
    if (job->state.load() == JOB_ERROR) {
        backend_log_t::handle().add("error", "job failed id=" + job->id + " reason=" + job->error);
        on_job_finished(job->id);
        return;
    }
    backend_log_t::handle().add("info", "job started id=" + job->id);

    common::render_request_t request = job->request;
    request.threads = granted_threads;

    gallery_manager_t *gm = gallery_manager_;
    const std::string gallery_workspace_id = job->workspace_id;
    const std::string gallery_integrator = job->integrator;
    const long long gallery_created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const std::string gallery_job_id = make_gallery_entry_id(gallery_created_at_ms, job->scene);
    bool gallery_entry_created = false;

    auto push_cb = push_callback_;
    if (push_cb) {
        job_snapshot_t preparing_snap;
        snapshot(job->id, preparing_snap);
        push_cb(job->id, preparing_snap, {});
    }

    common::render_result_t rr = common::render_scene_to_png(request,
        [this, job, gm, push_cb, &gallery_job_id, &gallery_workspace_id, &gallery_integrator,
         gallery_created_at_ms, &gallery_entry_created]
        (common::progress_event_t event, size_t done, size_t total,
         const xtcore::render::tile_t *tile, const common::progress_tile_update_t *upd) {
            bool entered_running = false;
            {
                std::lock_guard<std::mutex> lock(job->mut);
                if (event == common::PROGRESS_EVENT_TILE_STARTED) {
                    if (job->state.load() == JOB_PREPARING) {
                        job->state = JOB_RUNNING;
                        entered_running = true;
                    }
                    const bool has_upd_rect = upd && upd->has_rect;
                    if (tile || has_upd_rect) add_active_tile(*job, make_active_tile_key(tile, upd));
                } else if (event == common::PROGRESS_EVENT_TILE_FINISHED) {
                    const bool has_upd_rect = upd && upd->has_rect;
                    if (has_upd_rect && upd->source_fb) {
                        copy_rect_from_framebuffer(*upd->source_fb, upd->x0, upd->y0, upd->x1, upd->y1, job->progressive_fb);
                        job->progressive_ready = true;
                    } else {
                        copy_tile_to_framebuffer(tile, job->progressive_fb);
                        if (tile) job->progressive_ready = true;
                    }
                    if (tile || has_upd_rect) {
                        job_t::finished_tile_t finished;
                        finished.rect.x0 = has_upd_rect ? upd->x0 : tile->x0();
                        finished.rect.y0 = has_upd_rect ? upd->y0 : tile->y0();
                        finished.rect.x1 = has_upd_rect ? upd->x1 : tile->x1();
                        finished.rect.y1 = has_upd_rect ? upd->y1 : tile->y1();
                        finished.done_index = done;
                        job->finished_tiles.push_back(finished);
                    }
                    if (tile || has_upd_rect) remove_active_tile(*job, make_active_tile_key(tile, upd));
                }
            }
            if (entered_running && push_cb) {
                job_snapshot_t running_snap;
                this->snapshot(job->id, running_snap);
                push_cb(job->id, running_snap, {});
            }
            if (event == common::PROGRESS_EVENT_TILE_STARTED ||
                event == common::PROGRESS_EVENT_TILE_FINISHED) {
                job->tiles_total = total;
                if (event == common::PROGRESS_EVENT_TILE_FINISHED) {
                    job->tiles_done = done;
                }
            }
            if ((event == common::PROGRESS_EVENT_TILE_STARTED ||
                 event == common::PROGRESS_EVENT_TILE_FINISHED) && push_cb) {
                const bool has_upd_rect = (upd != nullptr && upd->has_rect);
                const bool has_tile = (tile != nullptr);
                const bool has_location = has_tile || has_upd_rect;

                auto push_u32le = [](std::vector<unsigned char> &buf, uint32_t v) {
                    buf.push_back((unsigned char)(v & 0xFF));
                    buf.push_back((unsigned char)((v >> 8) & 0xFF));
                    buf.push_back((unsigned char)((v >> 16) & 0xFF));
                    buf.push_back((unsigned char)((v >> 24) & 0xFF));
                };

                // For TILE_FINISHED events with a known rect, extract and encode the pixel data.
                std::vector<unsigned char> tile_rgba;
                size_t tx0 = 0, ty0 = 0, tx1 = 0, ty1 = 0;
                if (event == common::PROGRESS_EVENT_TILE_FINISHED && has_location) {
                    tx0 = has_upd_rect ? upd->x0 : (size_t)tile->x0();
                    ty0 = has_upd_rect ? upd->y0 : (size_t)tile->y0();
                    tx1 = has_upd_rect ? upd->x1 : (size_t)tile->x1();
                    ty1 = has_upd_rect ? upd->y1 : (size_t)tile->y1();

                    nimg::Pixmap tile_fb;
                    bool extracted = false;
                    xtcore::tonemapping::settings_t tm;
                    {
                        std::lock_guard<std::mutex> lk(job->mut);
                        extracted = extract_rect_from_framebuffer(
                            job->progressive_fb, tx0, ty0, tx1, ty1, tile_fb);
                        tm = job->live_tm_settings;
                    }
                    if (extracted) {
                        xtcore::tonemapping::apply(tile_fb, tm);
                        encode_rgba8_srgb(tile_fb, tile_rgba);
                    }
                }

                // Snapshot includes the current active_tiles list (already updated above
                // under the mutex for both STARTED and FINISHED events).
                job_snapshot_t snap;
                this->snapshot(job->id, snap);

                // Build XTDR binary packet (all integers u32 little-endian).
                //
                // XTDR — xtracer tile-data raw
                // ============================================================
                // Header (32 bytes)
                //   [0:4]   magic        "XTDR" (0x58 54 44 52) — raw RGBA variant
                //   [4:8]   width        image width in pixels
                //   [8:12]  height       image height in pixels
                //   [12:16] tiles_done   cumulative finished-tile count
                //   [16:20] tiles_total  total tiles for this job
                //   [20:24] state        JOB_RUNNING (4) always; terminal state goes as text JSON
                //   [24:28] tile_count   number of finished-tile records below (0 or 1)
                //   [28:32] elapsed_ms   server-authoritative render time (milliseconds, u32)
                //
                // Finished-tile record (24 + data_size bytes, repeated tile_count times)
                //   [+0:4]  x0           left edge, pixels, 0-based inclusive
                //   [+4:8]  y0           top edge, inclusive
                //   [+8:12] x1           right edge, exclusive
                //   [+12:16] y1          bottom edge, exclusive
                //   [+16:20] done_index  tiles_done value when this tile finished
                //   [+20:24] data_size   (x1-x0)*(y1-y0)*4 bytes
                //   [+24:N]  data        raw RGBA pixels, row-major, 8 bpc, alpha=255
                //                        default tonemapping applied server-side
                //
                // Active-tile section (4 + active_count*16 bytes)
                //   active_count  tiles currently in progress
                //   per entry: x0 y0 x1 y1 (4 bytes each, same coord convention)
                //
                // Firing rules
                //   TILE_STARTED:  tile_count=0; active section includes the new tile
                //   TILE_FINISHED: tile_count=1; active section already excludes it
                //   Terminal:      text JSON snapshot only, no binary frame
                //
                // Client parser: src/apps/web-client/app/preview.js parseImageDeltaPacket()
                // Protocol doc:  AGENTS.md § WebSocket Protocol
                std::vector<unsigned char> xtdr;
                xtdr.reserve(32 + 24 + tile_rgba.size() + 4 + snap.active_tiles.size() * 16);
                xtdr.push_back('X'); xtdr.push_back('T');
                xtdr.push_back('D'); xtdr.push_back('R'); // 'R' = raw RGBA
                push_u32le(xtdr, (uint32_t)job->request.width);
                push_u32le(xtdr, (uint32_t)job->request.height);
                push_u32le(xtdr, (uint32_t)done);
                push_u32le(xtdr, (uint32_t)total);
                push_u32le(xtdr, (uint32_t)JOB_RUNNING);
                push_u32le(xtdr, tile_rgba.empty() ? 0u : 1u);
                push_u32le(xtdr, (uint32_t)std::min(snap.elapsed_ms, (double)UINT32_MAX));
                if (!tile_rgba.empty()) {
                    push_u32le(xtdr, (uint32_t)tx0);
                    push_u32le(xtdr, (uint32_t)ty0);
                    push_u32le(xtdr, (uint32_t)tx1);
                    push_u32le(xtdr, (uint32_t)ty1);
                    push_u32le(xtdr, (uint32_t)done);
                    push_u32le(xtdr, (uint32_t)tile_rgba.size());
                    xtdr.insert(xtdr.end(), tile_rgba.begin(), tile_rgba.end());
                }
                // Active tile section: clients use this to draw in-progress tile markers.
                push_u32le(xtdr, (uint32_t)snap.active_tiles.size());
                for (const auto &r : snap.active_tiles) {
                    push_u32le(xtdr, (uint32_t)r.x0);
                    push_u32le(xtdr, (uint32_t)r.y0);
                    push_u32le(xtdr, (uint32_t)r.x1);
                    push_u32le(xtdr, (uint32_t)r.y1);
                }

                push_cb(job->id, snap, xtdr);
            }
            if (event == common::PROGRESS_EVENT_PASS_FINISHED && push_cb) {
                // Broadcast a text JSON snapshot so WS clients update pass_current,
                // pass_total, elapsed_ms, and the progress bar after each pass.
                job_snapshot_t pass_snap;
                snapshot(job->id, pass_snap);
                push_cb(job->id, pass_snap, {});
            }
            if (event == common::PROGRESS_EVENT_PASS_FINISHED
                && gm && gm->is_initialized()
                && upd && upd->source_fb
                && job->request.save_to_gallery
                && job->request.render_mode != common::render_request_t::RENDER_MODE_INTERACTIVE) {
                const size_t pass_index = done - 1;
                const auto now = std::chrono::steady_clock::now();
                const double elapsed_ms = std::chrono::duration<double, std::milli>(
                    now - job->started_at).count();

                nimg::Pixmap fb_copy = *upd->source_fb;
                std::vector<unsigned char> exr;
                nimg::io::save::exr_memory(fb_copy, exr);

                if (!gallery_entry_created) {
                    gallery_entry_meta_t meta;
                    meta.id           = gallery_job_id;
                    meta.scene        = job->request.scene_path;
                    meta.workspace_id = gallery_workspace_id;
                    meta.integrator   = gallery_integrator;
                    meta.render_mode  = render_mode_label(job->request.render_mode);
                    meta.width        = job->request.width;
                    meta.height       = job->request.height;
                    meta.samples      = job->request.samples;
                    meta.aa           = job->request.aa;
                    meta.rdepth       = job->request.rdepth;
                    meta.threads      = job->effective_threads;
                    meta.tile_size    = job->request.tile_size;
                    meta.elapsed_ms   = elapsed_ms;
                    meta.created_at_ms = gallery_created_at_ms;
                    gm->create_entry(meta, exr);
                    gallery_entry_created = true;
                }
                gm->save_pass(gallery_job_id, pass_index, exr, elapsed_ms);
            }
        },
        &job->cancel_requested
    );

    {
        std::lock_guard<std::mutex> lock(job->mut);
        job->elapsed_ms = rr.elapsed_ms;
        if (rr.aborted || job->cancel_requested.load()) {
            job->error = "render aborted";
            job->active_tiles.clear();
            job->state = JOB_ABORTED;
            backend_log_t::handle().add("info", "job aborted id=" + job->id);
        } else if (rr.ok) {
            job->final_fb = rr.framebuffer;
            job->image_exr.clear();
            job->image_hdr.clear();
            job->image_jpg.clear();
            job->image_bmp.clear();
            job->image_tga.clear();
            job->photon_diffuse_points = rr.photon_diffuse_points;
            job->photon_caustic_points = rr.photon_caustic_points;
            job->tiles_done = rr.tiles_done;
            job->tiles_total = rr.tiles_total;
            job->preview_rgba_cache.clear();
            job->preview_last_encoded_done = 0;
            job->preview_last_from_final = false;
            job->preview_last_tm_op = xtcore::tonemapping::OP_ACES_FITTED;
            job->preview_last_tm_exposure = 1.0f;
            job->preview_last_tm_white_point = 1.0f;
            job->preview_last_tm_mantiuk_contrast = 0.1f;
            job->preview_last_tm_mantiuk_saturation = 0.8f;
            job->preview_last_tm_mantiuk_detail = 1.0f;
            job->preview_last_post_filters_enabled = false;
            job->preview_last_post_filters.clear();
            job->progressive_fb.init(0, 0);
            job->active_tiles.clear();
            job->active_tile_index.clear();
            job->state = JOB_DONE;
            std::ostringstream log;
            log << "job completed id=" << job->id
                << " elapsed_ms=" << std::fixed << std::setprecision(0) << rr.elapsed_ms;
            backend_log_t::handle().add("info", log.str());

            // Save to gallery for direct mode (no pass events fired); skip interactive
            if (gm && gm->is_initialized() && !gallery_entry_created
                && job->request.save_to_gallery
                && job->request.render_mode != common::render_request_t::RENDER_MODE_INTERACTIVE) {
                nimg::Pixmap fb_copy = rr.framebuffer;
                std::vector<unsigned char> exr;
                nimg::io::save::exr_memory(fb_copy, exr);
                gallery_entry_meta_t meta;
                meta.id           = gallery_job_id;
                meta.scene        = job->request.scene_path;
                meta.workspace_id = gallery_workspace_id;
                meta.integrator   = gallery_integrator;
                meta.render_mode  = render_mode_label(job->request.render_mode);
                meta.width        = job->request.width;
                meta.height       = job->request.height;
                meta.samples      = job->request.samples;
                meta.aa           = job->request.aa;
                meta.rdepth       = job->request.rdepth;
                meta.threads      = job->effective_threads;
                meta.tile_size    = job->request.tile_size;
                meta.elapsed_ms   = rr.elapsed_ms;
                meta.created_at_ms = gallery_created_at_ms;
                gm->create_entry(meta, exr);
            } else if (gm && gm->is_initialized() && gallery_entry_created) {
                // Progressive/incremental: update render.exr with final framebuffer
                nimg::Pixmap fb_copy = job->final_fb;
                std::vector<unsigned char> exr;
                nimg::io::save::exr_memory(fb_copy, exr);
                gm->update_render(gallery_job_id, exr, rr.elapsed_ms);
            }
        } else {
            job->error = rr.error;
            job->active_tiles.clear();
            job->active_tile_index.clear();
            job->state = JOB_ERROR;
            backend_log_t::handle().add("error", "job failed id=" + job->id + " reason=" + rr.error);
        }
    }

    if (!job->cleanup_scene_path.empty()) {
        unlink(job->cleanup_scene_path.c_str());
    }

    // Notify WS subscribers of the terminal state (done/aborted/error).
    // Tile push callbacks only fire during rendering, so we need an explicit
    // final push here so connected clients can fetch the full image.
    if (push_cb) {
        job_snapshot_t terminal_snap;
        snapshot(job->id, terminal_snap);
        push_cb(job->id, terminal_snap, {});
    }

    on_job_finished(job->id);
}

std::shared_ptr<job_manager_t::job_t> job_manager_t::get_job(const std::string &id)
{
    std::lock_guard<std::mutex> lock(jobs_mut);
    auto it = jobs.find(id);
    if (it == jobs.end()) return std::shared_ptr<job_t>();
    return it->second;
}

void job_manager_t::prune_completed_jobs_locked()
{
    while (completed_job_order.size() > max_completed_jobs) {
        const std::string evict_id = completed_job_order.front();
        completed_job_order.pop_front();

        auto it = jobs.find(evict_id);
        if (it == jobs.end()) continue;

        const job_state_t st = it->second->state.load();
        if (st == JOB_RUNNING || st == JOB_PREPARING || st == JOB_QUEUED) continue;

        cache_evicted_job_locked(it->second);
        jobs.erase(it);
        backend_log_t::handle().add("debug", "job evicted id=" + evict_id);
    }
}

void job_manager_t::cache_evicted_job_locked(const std::shared_ptr<job_t> &job)
{
    if (!job) return;

    evicted_job_t rec;
    rec.id = job->id;
    rec.workspace_id = job->workspace_id;
    rec.scene = job->scene;
    rec.integrator = job->integrator;
    rec.state = job->state.load();
    rec.error = job->error;
    rec.elapsed_ms = job->elapsed_ms;
    rec.width = job->request.width;
    rec.height = job->request.height;
    rec.threads = (job->effective_threads > 0) ? job->effective_threads : job->request.threads;

    {
        std::lock_guard<std::mutex> lock(job->mut);
        if (job->final_fb.width() > 0 && job->final_fb.height() > 0
            && job->request.render_mode != common::render_request_t::RENDER_MODE_INTERACTIVE) {
            const std::string exr_path = "/tmp/xtracer_job_cache_" + job->id + ".exr";
            nimg::Pixmap fb_copy = job->final_fb;
            std::vector<unsigned char> exr;
            if (nimg::io::save::exr_memory(fb_copy, exr) == 0) {
                if (write_file_bytes(exr_path.c_str(), exr)) {
                    rec.exr_path = exr_path;
                }
            }
        }
    }

    evicted_jobs[rec.id] = rec;
    evicted_job_order.push_back(rec.id);
    prune_evicted_jobs_locked();
}

void job_manager_t::prune_evicted_jobs_locked()
{
    while (evicted_job_order.size() > max_evicted_jobs) {
        const std::string id = evicted_job_order.front();
        evicted_job_order.pop_front();
        auto it = evicted_jobs.find(id);
        if (it == evicted_jobs.end()) continue;
        if (!it->second.exr_path.empty()) {
            unlink(it->second.exr_path.c_str());
        }
        evicted_jobs.erase(it);
    }
}

void job_manager_t::on_job_finished(const std::string &id)
{
    std::lock_guard<std::mutex> lock(jobs_mut);
    auto it = jobs.find(id);
    if (it == jobs.end()) return;
    const job_state_t st = it->second->state.load();
    if (st != JOB_DONE && st != JOB_ABORTED && st != JOB_ERROR) return;
    completed_job_order.push_back(id);
    prune_completed_jobs_locked();
}

bool job_manager_t::snapshot(const std::string &id, job_snapshot_t &out)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) {
        std::lock_guard<std::mutex> lock(jobs_mut);
        auto eit = evicted_jobs.find(id);
        if (eit == evicted_jobs.end()) return false;
        const evicted_job_t &e = eit->second;
        out.id = e.id;
        out.workspace_id = e.workspace_id;
        out.scene = e.scene;
        out.integrator = e.integrator;
        out.render_mode = "direct";
        out.state = e.state;
        out.error = e.error;
        out.elapsed_ms = e.elapsed_ms;
        out.has_image = !e.exr_path.empty();
        out.width = e.width;
        out.height = e.height;
        out.threads = e.threads;
        out.tiles_done = 0;
        out.tiles_total = 0;
        out.pass_current = 0;
        out.pass_total = 0;
        out.queue_index = -1;
        out.active_tiles.clear();
        out.progress = (e.state == JOB_DONE) ? 1.0f : 0.0f;
        return true;
    }

    std::lock_guard<std::mutex> lock(job->mut);

    out.id = job->id;
    out.workspace_id = job->workspace_id;
    out.scene = job->scene;
    out.integrator = job->integrator;
    out.render_mode = render_mode_label(job->request.render_mode);
    out.state = job->state.load();
    out.error = job->error;
    out.elapsed_ms = job->elapsed_ms;
    if ((out.state == JOB_RUNNING || out.state == JOB_ABORTING || out.state == JOB_PREPARING || out.state == JOB_QUEUED) && job->has_started) {
        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        out.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - job->started_at).count();
    }
    out.has_image = (job->final_fb.width() > 0 || job->progressive_ready);
    out.width = job->request.width;
    out.height = job->request.height;
    out.threads = (job->effective_threads > 0) ? job->effective_threads : job->request.threads;
    out.tiles_done = job->tiles_done.load();
    out.tiles_total = job->tiles_total.load();
    out.pass_current = 0;
    out.pass_total = 0;
    out.queue_index = -1;
    out.active_tiles = job->active_tiles;

    if (job->request.render_mode == common::render_request_t::RENDER_MODE_PROGRESSIVE ||
        job->request.render_mode == common::render_request_t::RENDER_MODE_INCREMENTAL) {
        const size_t ptotal = (job->request.render_mode == common::render_request_t::RENDER_MODE_INCREMENTAL)
            ? incremental_pass_count(job->request.samples)
            : progressive_pass_count(job->request.samples);
        out.pass_total = ptotal;
        size_t tiles_per_pass = (ptotal > 0) ? (out.tiles_total / ptotal) : 0;
        if (tiles_per_pass == 0) {
            const size_t tile_size = (job->request.tile_size > 0) ? job->request.tile_size : 1;
            const size_t nx = (job->request.width + tile_size - 1) / tile_size;
            const size_t ny = (job->request.height + tile_size - 1) / tile_size;
            tiles_per_pass = nx * ny;
        }
        if (out.state == JOB_DONE) {
            out.pass_current = out.pass_total;
        } else if (out.pass_total > 0) {
            const size_t completed_passes = (tiles_per_pass > 0) ? (out.tiles_done / tiles_per_pass) : 0;
            const bool in_pass = (tiles_per_pass > 0) ? ((out.tiles_done % tiles_per_pass) != 0) : false;
            size_t curr = completed_passes + (in_pass ? 1 : 0);
            if ((out.state == JOB_QUEUED || out.state == JOB_PREPARING || out.state == JOB_RUNNING || out.state == JOB_ABORTING) && curr == 0) curr = 1;
            if (curr > out.pass_total) curr = out.pass_total;
            out.pass_current = curr;
        }
    }

    size_t total = out.tiles_total;
    size_t done = out.tiles_done;
    if (total == 0) {
        out.progress = (out.state == JOB_DONE) ? 1.0f : 0.0f;
    } else {
        out.progress = (float)done / (float)total;
        if (out.progress > 1.0f) out.progress = 1.0f;
    }

    return true;
}

bool job_manager_t::image(const std::string &id,
                          std::vector<unsigned char> &out,
                          bool allow_partial,
                          const xtcore::tonemapping::settings_t &tm_settings,
                          bool post_filters_enabled,
                          const std::string &post_filters)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) {
        if (allow_partial) return false;
        std::string exr_path;
        {
            std::lock_guard<std::mutex> lock(jobs_mut);
            auto eit = evicted_jobs.find(id);
            if (eit == evicted_jobs.end() || eit->second.exr_path.empty()) return false;
            exr_path = eit->second.exr_path;
        }
        std::vector<unsigned char> exr;
        if (!read_file_bytes(exr_path.c_str(), exr)) return false;
        nimg::Pixmap fb;
        if (nimg::io::load::exr_memory(exr.data(), exr.size(), fb) != 0) return false;
        xtcore::tonemapping::apply(fb, tm_settings);
        encode_rgba8_srgb(fb, out);
        return !out.empty();
    }

    post_filter_chain_t post_chain;
    std::string post_key;
    if (!parse_post_filter_chain(post_filters_enabled, post_filters, post_chain, post_key)) return false;

    bool use_final = false;
    size_t done = 0;
    bool cache_invalid = false;
    nimg::Pixmap work;

    {
        std::lock_guard<std::mutex> lock(job->mut);

        use_final = (job->state == JOB_DONE
                     && job->final_fb.width() > 0
                     && job->final_fb.height() > 0);
        if (!use_final && (!allow_partial || !job->progressive_ready)) return false;

        done = use_final ? job->tiles_total.load() : job->tiles_done.load();
        cache_invalid = job->preview_rgba_cache.empty()
                     || job->preview_last_encoded_done != done
                     || job->preview_last_from_final != use_final
                     || job->preview_last_tm_op != tm_settings.op
                     || std::fabs(job->preview_last_tm_exposure - tm_settings.exposure) > 1e-6f
                     || std::fabs(job->preview_last_tm_white_point - tm_settings.white_point) > 1e-6f
                     || std::fabs(job->preview_last_tm_mantiuk_contrast - tm_settings.mantiuk_contrast) > 1e-6f
                     || std::fabs(job->preview_last_tm_mantiuk_saturation - tm_settings.mantiuk_saturation) > 1e-6f
                     || std::fabs(job->preview_last_tm_mantiuk_detail - tm_settings.mantiuk_detail) > 1e-6f
                     || job->preview_last_post_filters_enabled != post_filters_enabled
                     || job->preview_last_post_filters != post_key;

        if (!cache_invalid) {
            out = job->preview_rgba_cache;
            return true;
        }

        work = use_final ? job->final_fb : job->progressive_fb;
    }

    std::vector<unsigned char> encoded;
    if (!encode_rgba_memory(work, tm_settings, post_chain, encoded)) return false;

    {
        std::lock_guard<std::mutex> lock(job->mut);
        const bool still_use_final = (job->state == JOB_DONE
                                      && job->final_fb.width() > 0
                                      && job->final_fb.height() > 0);
        const size_t still_done = still_use_final ? job->tiles_total.load() : job->tiles_done.load();
        if (still_use_final == use_final && still_done == done) {
            job->preview_rgba_cache = encoded;
            job->preview_last_encoded_done = done;
            job->preview_last_from_final = use_final;
            job->preview_last_tm_op = tm_settings.op;
            job->preview_last_tm_exposure = tm_settings.exposure;
            job->preview_last_tm_white_point = tm_settings.white_point;
            job->preview_last_tm_mantiuk_contrast = tm_settings.mantiuk_contrast;
            job->preview_last_tm_mantiuk_saturation = tm_settings.mantiuk_saturation;
            job->preview_last_tm_mantiuk_detail = tm_settings.mantiuk_detail;
            job->preview_last_post_filters_enabled = post_filters_enabled;
            job->preview_last_post_filters = post_key;
            out = job->preview_rgba_cache;
            return true;
        }
    }

    out.swap(encoded);
    return true;
}

bool job_manager_t::image_rgba(const std::string &id,
                               std::vector<unsigned char> &rgba_out,
                               size_t &width_out,
                               size_t &height_out,
                               size_t &tiles_done_out,
                               size_t &tiles_total_out)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;

    xtcore::tonemapping::settings_t tm;
    {
        std::lock_guard<std::mutex> lock(job->mut);
        tm = job->live_tm_settings;
    }
    return image_rgba(id,
                      rgba_out,
                      width_out,
                      height_out,
                      tiles_done_out,
                      tiles_total_out,
                      true,
                      tm,
                      false,
                      "");
}

bool job_manager_t::image_rgba(const std::string &id,
                               std::vector<unsigned char> &rgba_out,
                               size_t &width_out,
                               size_t &height_out,
                               size_t &tiles_done_out,
                               size_t &tiles_total_out,
                               bool allow_partial,
                               const xtcore::tonemapping::settings_t &tm_settings,
                               bool post_filters_enabled,
                               const std::string &post_filters)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) {
        if (allow_partial) return false;

        std::string exr_path;
        {
            std::lock_guard<std::mutex> lock(jobs_mut);
            auto eit = evicted_jobs.find(id);
            if (eit == evicted_jobs.end() || eit->second.exr_path.empty()) return false;
            exr_path = eit->second.exr_path;
        }

        post_filter_chain_t post_chain;
        std::string post_key_dummy;
        if (!parse_post_filter_chain(post_filters_enabled, post_filters, post_chain, post_key_dummy)) return false;

        std::vector<unsigned char> exr;
        if (!read_file_bytes(exr_path.c_str(), exr)) return false;

        nimg::Pixmap fb;
        if (nimg::io::load::exr_memory(exr.data(), exr.size(), fb) != 0) return false;

        build_preview_pixmap(fb, tm_settings, post_chain);
        width_out = fb.width();
        height_out = fb.height();
        tiles_done_out = 0;
        tiles_total_out = 0;
        encode_rgba8_srgb(fb, rgba_out);
        return true;
    }

    post_filter_chain_t post_chain;
    std::string post_key_dummy;
    if (!parse_post_filter_chain(post_filters_enabled, post_filters, post_chain, post_key_dummy)) return false;

    nimg::Pixmap fb;
    {
        std::lock_guard<std::mutex> lock(job->mut);
        const bool use_final = (job->state == JOB_DONE
                                && job->final_fb.width() > 0
                                && job->final_fb.height() > 0);
        if (!use_final && (!allow_partial || !job->progressive_ready)) return false;

        width_out = job->request.width;
        height_out = job->request.height;
        tiles_done_out = use_final ? job->tiles_total.load() : job->tiles_done.load();
        tiles_total_out = job->tiles_total.load();
        fb = use_final ? job->final_fb : job->progressive_fb;
    }

    build_preview_pixmap(fb, tm_settings, post_chain);
    encode_rgba8_srgb(fb, rgba_out);
    return true;
}

bool job_manager_t::image_delta(const std::string &id,
                                size_t since_done,
                                size_t max_tiles,
                                const xtcore::tonemapping::settings_t &tm_settings,
                                bool post_filters_enabled,
                                const std::string &post_filters,
                                job_image_delta_t &out)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;

    post_filter_chain_t post_chain;
    std::string post_key_dummy;
    if (!parse_post_filter_chain(post_filters_enabled, post_filters, post_chain, post_key_dummy)) return false;

    struct pending_tile_t {
        size_t x0;
        size_t y0;
        size_t x1;
        size_t y1;
        size_t done_index;
        nimg::Pixmap fb;
    };

    std::vector<pending_tile_t> pending;
    pending.reserve(max_tiles);
    nimg::Pixmap preview_fb;
    const size_t filter_padding = post_filter_padding_pixels(post_chain);
    const bool can_process_tiles_individually =
        post_chain.empty() && tm_settings.op != xtcore::tonemapping::OP_MANTIUK_2006;

    {
        std::lock_guard<std::mutex> lock(job->mut);
        out.state = job->state.load();
        out.width = job->request.width;
        out.height = job->request.height;
        out.tiles_done = job->tiles_done.load();
        out.tiles_total = job->tiles_total.load();
        out.tiles.clear();

        const bool use_final = (out.state == JOB_DONE
                                && job->final_fb.width() > 0
                                && job->final_fb.height() > 0);
        if (!use_final && !job->progressive_ready) return true;

        const nimg::Pixmap &src = use_final ? job->final_fb : job->progressive_fb;
        if (!can_process_tiles_individually) {
            preview_fb = src;
        }
        const size_t target_done = since_done + 1;
        std::vector<job_t::finished_tile_t>::const_iterator begin_it = std::lower_bound(
            job->finished_tiles.begin(),
            job->finished_tiles.end(),
            target_done,
            [](const job_t::finished_tile_t &tile, size_t target) {
                return tile.done_index < target;
            });
        for (std::vector<job_t::finished_tile_t>::const_iterator it = begin_it;
             it != job->finished_tiles.end(); ++it) {
            const job_t::finished_tile_t &t = *it;
            if (t.done_index <= since_done) continue;
            if (max_tiles > 0 && pending.size() >= max_tiles) break;

            const size_t tw = (t.rect.x1 > t.rect.x0) ? (t.rect.x1 - t.rect.x0) : 0;
            const size_t th = (t.rect.y1 > t.rect.y0) ? (t.rect.y1 - t.rect.y0) : 0;
            if (tw == 0 || th == 0) continue;

            pending_tile_t entry;
            entry.x0 = t.rect.x0;
            entry.y0 = t.rect.y0;
            entry.x1 = t.rect.x1;
            entry.y1 = t.rect.y1;
            expand_rect_with_padding(src.width(),
                                     src.height(),
                                     filter_padding,
                                     entry.x0,
                                     entry.y0,
                                     entry.x1,
                                     entry.y1);
            entry.done_index = t.done_index;
            if (can_process_tiles_individually
                && !extract_rect_from_framebuffer(src,
                                                  entry.x0,
                                                  entry.y0,
                                                  entry.x1,
                                                  entry.y1,
                                                  entry.fb)) {
                continue;
            }
            pending.push_back(entry);
        }
    }

    if (pending.empty()) return true;

    if (!can_process_tiles_individually) {
        build_preview_pixmap(preview_fb, tm_settings, post_chain);
    }

    out.tiles.reserve(pending.size());
    for (size_t i = 0; i < pending.size(); ++i) {
        nimg::Pixmap tile_fb;
        if (can_process_tiles_individually) {
            tile_fb = std::move(pending[i].fb);
            xtcore::tonemapping::apply(tile_fb, tm_settings);
        } else {
            if (!extract_rect_from_framebuffer(preview_fb,
                                               pending[i].x0,
                                               pending[i].y0,
                                               pending[i].x1,
                                               pending[i].y1,
                                               tile_fb)) {
                continue;
            }
        }
        std::vector<unsigned char> encoded;
        encode_rgba8_srgb(tile_fb, encoded);
        if (encoded.empty()) continue;

        job_image_delta_t::tile_t tile;
        tile.x0 = pending[i].x0;
        tile.y0 = pending[i].y0;
        tile.x1 = pending[i].x1;
        tile.y1 = pending[i].y1;
        tile.done_index = pending[i].done_index;
        tile.rgba.swap(encoded);
        out.tiles.push_back(tile);
    }

    return true;
}

bool job_manager_t::image_export(const std::string &id,
                                 const std::string &format,
                                 std::vector<unsigned char> &out,
                                 std::string &mime_type,
                                 std::string &extension,
                                 bool post_filters_enabled,
                                 const std::string &post_filters)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) {
        if (format != "png" && format != "exr") return false;
        std::string exr_path;
        {
            std::lock_guard<std::mutex> lock(jobs_mut);
            auto eit = evicted_jobs.find(id);
            if (eit == evicted_jobs.end() || eit->second.exr_path.empty()) return false;
            exr_path = eit->second.exr_path;
        }
        if (format == "exr") {
            if (!read_file_bytes(exr_path.c_str(), out)) return false;
            mime_type = "image/x-exr";
            extension = "exr";
            return true;
        }
        // format == "png": decode EXR, apply default tonemapping, encode PNG
        std::vector<unsigned char> exr;
        if (!read_file_bytes(exr_path.c_str(), exr)) return false;
        nimg::Pixmap fb;
        if (nimg::io::load::exr_memory(exr.data(), exr.size(), fb) != 0) return false;
        xtcore::tonemapping::settings_t tm_defaults;
        xtcore::tonemapping::apply(fb, tm_defaults);
        if (nimg::io::save::png_memory(fb, out) != 0) return false;
        mime_type = "image/png";
        extension = "png";
        return true;
    }

    std::lock_guard<std::mutex> lock(job->mut);
    if (job->state != JOB_DONE) return false;

    post_filter_chain_t post_chain;
    std::string post_key_dummy;
    if (!parse_post_filter_chain(post_filters_enabled, post_filters, post_chain, post_key_dummy)) return false;

    if (format == "png") {
        if (job->final_fb.width() == 0 || job->final_fb.height() == 0) return false;
        nimg::Pixmap work = job->final_fb;
        xtcore::tonemapping::settings_t tm_settings;
        apply_post_filters_for_stage(work, post_chain, true);
        xtcore::tonemapping::apply(work, tm_settings);
        apply_post_filters_for_stage(work, post_chain, false);
        if (nimg::io::save::png_memory(work, out) != 0) return false;
        mime_type = "image/png";
        extension = "png";
        return true;
    }

    if (job->final_fb.width() == 0 || job->final_fb.height() == 0) return false;

    if (format == "exr") {
        nimg::Pixmap work = job->final_fb;
        apply_post_filters_for_stage(work, post_chain, true);
        if (nimg::io::save::exr_memory(work, out) != 0) return false;
        mime_type = "image/x-exr";
        extension = "exr";
        return true;
    }

    if (format == "hdr") {
        nimg::Pixmap work = job->final_fb;
        apply_post_filters_for_stage(work, post_chain, true);
        if (nimg::io::save::hdr_memory(work, out) != 0) return false;
        mime_type = "image/vnd.radiance";
        extension = "hdr";
        return true;
    }

    if (format == "jpg") {
        nimg::Pixmap work = job->final_fb;
        if (!encode_jpg_memory(work, post_chain, out)) return false;
        mime_type = "image/jpeg";
        extension = "jpg";
        return true;
    }

    if (format == "bmp") {
        nimg::Pixmap work = job->final_fb;
        apply_post_filters_for_stage(work, post_chain, true);
        xtcore::tonemapping::apply(work);
        apply_post_filters_for_stage(work, post_chain, false);
        if (nimg::io::save::bmp_memory(work, out) != 0) return false;
        mime_type = "image/bmp";
        extension = "bmp";
        return true;
    }

    if (format == "tga") {
        nimg::Pixmap work = job->final_fb;
        apply_post_filters_for_stage(work, post_chain, true);
        xtcore::tonemapping::apply(work);
        apply_post_filters_for_stage(work, post_chain, false);
        if (nimg::io::save::tga_memory(work, out) != 0) return false;
        mime_type = "image/x-tga";
        extension = "tga";
        return true;
    }

    return false;
}

bool job_manager_t::photons(const std::string &id,
                            std::vector<common::render_result_t::point3_t> &diffuse_out,
                            std::vector<common::render_result_t::point3_t> &caustic_out,
                            size_t limit_per_set)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;

    std::lock_guard<std::mutex> lock(job->mut);
    if (job->state != JOB_DONE) return false;
    if (job->integrator != "photon_mapping") return false;

    const size_t cap = (limit_per_set == 0) ? (size_t)100000 : limit_per_set;
    auto append_capped = [cap](const std::vector<common::render_result_t::point3_t> &src,
                               std::vector<common::render_result_t::point3_t> &dst) {
        dst.clear();
        if (src.empty()) return;
        if (src.size() <= cap) {
            dst = src;
            return;
        }
        dst.reserve(cap);
        const double step = (double)src.size() / (double)cap;
        for (size_t i = 0; i < cap; ++i) {
            size_t idx = (size_t)(i * step);
            if (idx >= src.size()) idx = src.size() - 1;
            dst.push_back(src[idx]);
        }
    };

    append_capped(job->photon_diffuse_points, diffuse_out);
    append_capped(job->photon_caustic_points, caustic_out);
    return true;
}

bool job_manager_t::abort(const std::string &id)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;

    job->cancel_requested.store(true);
    {
        std::lock_guard<std::mutex> lock(render_slots_mut);
        for (auto it = queued_job_order.begin(); it != queued_job_order.end(); ++it) {
            if (*it == id) {
                queued_job_order.erase(it);
                break;
            }
        }
        render_slots_cv.notify_all();
    }
    const job_state_t st = job->state.load();
    if (st == JOB_QUEUED) {
        std::lock_guard<std::mutex> lock(job->mut);
        if (job->state.load() == JOB_QUEUED) {
            job->error = "render aborted";
            job->state = JOB_ABORTED;
            if (!job->cleanup_scene_path.empty()) {
                unlink(job->cleanup_scene_path.c_str());
                job->cleanup_scene_path.clear();
            }
            backend_log_t::handle().add("info", "job aborted id=" + job->id + " state=queued");
            on_job_finished(job->id);
        }
    } else if (st == JOB_RUNNING || st == JOB_PREPARING) {
        {
            std::lock_guard<std::mutex> lock(job->mut);
            const job_state_t st2 = job->state.load();
            if (st2 == JOB_RUNNING || st2 == JOB_PREPARING) {
                job->state = JOB_ABORTING;
            }
        }
        auto push_cb = push_callback_;
        if (push_cb) {
            job_snapshot_t snap;
            snapshot(job->id, snap);
            push_cb(job->id, snap, {});
        }
        backend_log_t::handle().add("info", "job aborting id=" + job->id);
    }
    dispatch_queued_jobs();
    return true;
}

bool job_manager_t::set_live_tm_settings(const std::string &id,
                                         const xtcore::tonemapping::settings_t &tm)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;
    std::lock_guard<std::mutex> lock(job->mut);
    job->live_tm_settings = tm;
    return true;
}

bool job_manager_t::belongs_to_client(const std::string &id, const std::string &client_id)
{
    if (client_id.empty()) return false;
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;
    std::lock_guard<std::mutex> lock(job->mut);
    return job->owner_client_id == client_id;
}

bool job_manager_t::move_queue_up(const std::string &id)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;
    if (job->state.load() != JOB_QUEUED) return false;

    std::lock_guard<std::mutex> lock(render_slots_mut);
    for (size_t i = 0; i < queued_job_order.size(); ++i) {
        if (queued_job_order[i] != id) continue;
        if (i == 0) return true;
        std::swap(queued_job_order[i - 1], queued_job_order[i]);
        render_slots_cv.notify_all();
        return true;
    }
    return false;
}

bool job_manager_t::move_queue_down(const std::string &id)
{
    std::shared_ptr<job_t> job = get_job(id);
    if (!job) return false;
    if (job->state.load() != JOB_QUEUED) return false;

    std::lock_guard<std::mutex> lock(render_slots_mut);
    for (size_t i = 0; i < queued_job_order.size(); ++i) {
        if (queued_job_order[i] != id) continue;
        if (i + 1 >= queued_job_order.size()) return true;
        std::swap(queued_job_order[i], queued_job_order[i + 1]);
        render_slots_cv.notify_all();
        return true;
    }
    return false;
}

bool job_manager_t::list_active(std::vector<job_snapshot_t> &out)
{
    std::vector<std::string> running_ids;
    std::vector<std::string> queued_ids;
    std::map<std::string, int> queue_index_by_id;

    {
        std::lock_guard<std::mutex> lock(render_slots_mut);
        for (size_t i = 0; i < queued_job_order.size(); ++i) {
            queue_index_by_id[queued_job_order[i]] = static_cast<int>(i);
        }
    }

    {
        std::lock_guard<std::mutex> lock(jobs_mut);
        for (auto it = jobs.begin(); it != jobs.end(); ++it) {
            const std::shared_ptr<job_t> &job = it->second;
            if (!job) continue;
            const job_state_t st = job->state.load();
            if (st == JOB_RUNNING || st == JOB_ABORTING || st == JOB_PREPARING) {
                running_ids.push_back(job->id);
            } else if (st == JOB_QUEUED) {
                queued_ids.push_back(job->id);
            }
        }
    }

    std::sort(running_ids.begin(), running_ids.end(), [](const std::string &a, const std::string &b) {
        const unsigned long long sa = parse_job_sequence(a);
        const unsigned long long sb = parse_job_sequence(b);
        if (sa != sb) return sa > sb;
        return a > b;
    });
    std::sort(queued_ids.begin(), queued_ids.end(), [&queue_index_by_id](const std::string &a, const std::string &b) {
        const auto ia = queue_index_by_id.find(a);
        const auto ib = queue_index_by_id.find(b);
        const int qa = (ia == queue_index_by_id.end()) ? 0x3fffffff : ia->second;
        const int qb = (ib == queue_index_by_id.end()) ? 0x3fffffff : ib->second;
        if (qa != qb) return qa < qb;
        const unsigned long long sa = parse_job_sequence(a);
        const unsigned long long sb = parse_job_sequence(b);
        if (sa != sb) return sa > sb;
        return a > b;
    });

    out.clear();
    for (size_t i = 0; i < running_ids.size(); ++i) {
        job_snapshot_t snap;
        if (snapshot(running_ids[i], snap)) {
            snap.queue_index = -1;
            out.push_back(snap);
        }
    }
    for (size_t i = 0; i < queued_ids.size(); ++i) {
        job_snapshot_t snap;
        if (snapshot(queued_ids[i], snap)) {
            const auto qit = queue_index_by_id.find(snap.id);
            snap.queue_index = (qit == queue_index_by_id.end()) ? static_cast<int>(i) : qit->second;
            out.push_back(snap);
        }
    }
    return !out.empty();
}

bool job_manager_t::has_active_jobs()
{
    std::lock_guard<std::mutex> lock(jobs_mut);
    for (auto it = jobs.begin(); it != jobs.end(); ++it) {
        if (!it->second) continue;
        const job_state_t st = it->second->state.load();
        if (st == JOB_RUNNING || st == JOB_ABORTING || st == JOB_PREPARING || st == JOB_QUEUED) return true;
    }
    return false;
}

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */
