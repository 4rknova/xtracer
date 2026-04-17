#include "routes.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <iterator>
#include <list>
#include <ctime>
#include <vector>
#include <thread>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <sys/stat.h>
#include <unistd.h>
#if defined(_OPENMP)
#include <omp.h>
#endif

#include "ws_hub.h"
#include <xtcore/camera.h>
#include <xtcore/math/plane.h>
#include <xtcore/math/sphere.h>
#include <xtcore/math/triangle.h>
#include <xtcore/math/fractal.h>
#include <xtcore/math/csg.h>
#include <xtcore/material.h>
#include <xtcore/medium/homogeneous.h>
#include <xtcore/mesh.h>
#include <xtcore/parseutil.h>
#include <xtcore/resolution_preset.h>
#include <xtcore/sampler/sampler_checker.h>
#include <xtcore/sampler/sampler_col.h>
#include <xtcore/sampler/sampler_cubemap.h>
#include <xtcore/sampler/sampler_erp.h>
#include <xtcore/sampler/sampler_fbm_marble.h>
#include <xtcore/sampler/sampler_gradient.h>
#include <xtcore/sampler/sampler_rayleigh_sky.h>
#include <xtcore/sampler/sampler_graphpaper.h>
#include <xtcore/sampler/sampler_tex.h>
#include <xtcore/sampler/sampler_weave.h>
#include <xtcore/scene.h>
#include <xtcore/strpool.h>
#include <xtcore/tonemapping/tonemapping.h>
#include <xtcore/xtcore.h>
#include <xtcore/math/sampling_util.h>
#include <nmath/sample.h>
#include <nimg/img.h>

#include "backend_log.h"
#include "furnace_tests.h"
#include "gallery_manager.h"
#include "job_manager.h"
#include "post_filters.h"
#include "workspace_manager.h"

namespace xtracer {
namespace frontend {
namespace web {

namespace {

size_t runtime_omp_max_threads()
{
#if defined(_OPENMP)
    int n = omp_get_max_threads();
    return (n > 0) ? static_cast<size_t>(n) : 0;
#else
    return 0;
#endif
}

size_t runtime_logical_cores()
{
    const unsigned int logical_cores_raw = std::thread::hardware_concurrency();
    return (logical_cores_raw == 0) ? 1 : static_cast<size_t>(logical_cores_raw);
}

size_t compute_auto_render_threads(size_t reserve_threads)
{
    const size_t logical_cores = runtime_logical_cores();
    const size_t openmp_max_threads = runtime_omp_max_threads();
    const size_t capacity = (openmp_max_threads > 0) ? openmp_max_threads : logical_cores;
    return (capacity > reserve_threads) ? (capacity - reserve_threads) : 1;
}

struct pooled_hash_guard_t
{
    HASH_ID value;

    pooled_hash_guard_t()
        : value(HASH_ID_INVALID)
    {}

    ~pooled_hash_guard_t()
    {
        if (value != HASH_ID_INVALID) xtcore::pool::str::del(value);
    }

    void reset(HASH_ID next)
    {
        if (value != HASH_ID_INVALID) xtcore::pool::str::del(value);
        value = next;
    }
};

std::string json_escape(const std::string &s)
{
    std::ostringstream out;
    for (size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        switch (c) {
            case '\\': out << "\\\\"; break;
            case '"':  out << "\\\""; break;
            case '\n': out << "\\n";  break;
            case '\r': out << "\\r";  break;
            case '\t': out << "\\t";  break;
            default:
                out << c;
                break;
        }
    }
    return out.str();
}

// ---------------------------------------------------------------------------
// Unified request parameter accessor (merges URL query params + POST body).
// ---------------------------------------------------------------------------
struct params_view
{
    const crow::request &req;
    crow::query_string bp;

    explicit params_view(const crow::request &r)
        : req(r), bp(r.get_body_params()) {}

    const char *get(const char *key) const {
        const char *v = req.url_params.get(key);
        return v ? v : bp.get(key);
    }

    bool has(const char *key) const { return get(key) != nullptr; }

    std::string str(const char *key, const char *def = "") const {
        const char *v = get(key);
        return v ? std::string(v) : std::string(def);
    }

    std::vector<std::string> keys() const {
        std::vector<std::string> k = req.url_params.keys();
        for (const auto &bk : bp.keys()) {
            bool found = false;
            for (const auto &k1 : k) { if (k1 == bk) { found = true; break; } }
            if (!found) k.push_back(bk);
        }
        return k;
    }
};

void send_json(crow::response &res, const std::string &json, int status = 200)
{
    res.code = status;
    res.set_header("Cache-Control", "no-store");
    res.set_header("Content-Type", "application/json");
    res.body = json;
    res.end();
}

std::string backend_logs_to_json(const std::vector<backend_log_entry_t> &list)
{
    std::ostringstream out;
    out << "{\"entries\":[";
    for (size_t i = 0; i < list.size(); ++i) {
        const backend_log_entry_t &e = list[i];
        if (i) out << ',';
        out << "{"
            << "\"id\":" << e.id << ","
            << "\"ts\":\"" << json_escape(e.timestamp) << "\","
            << "\"level\":\"" << json_escape(e.level) << "\","
            << "\"message\":\"" << json_escape(e.message) << "\""
            << "}";
    }
    out << "]}";
    return out.str();
}

bool read_binary_file(const std::string &path, std::vector<char> &out)
{
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in.good()) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

bool read_text_file(const std::string &path, std::string &out)
{
    std::vector<char> bytes;
    if (!read_binary_file(path, bytes)) return false;
    out.assign(bytes.begin(), bytes.end());
    return true;
}

bool write_text_file(const std::string &path, const std::string &content)
{
    std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.good()) return false;
    out << content;
    return out.good();
}

bool file_exists(const std::string &path)
{
    std::ifstream in(path.c_str(), std::ios::binary);
    return in.good();
}

const char *integrator_status_name(xtcore::render::integrator_status_t status)
{
    switch (status) {
        case xtcore::render::INTEGRATOR_STATUS_RECOMMENDED: return "recommended";
        case xtcore::render::INTEGRATOR_STATUS_STABLE: return "stable";
        case xtcore::render::INTEGRATOR_STATUS_EXPERIMENTAL: return "experimental";
        case xtcore::render::INTEGRATOR_STATUS_LEGACY: return "legacy";
        case xtcore::render::INTEGRATOR_STATUS_HIDDEN: return "hidden";
        default: return "stable";
    }
}

const char *filter_status_name(xtcore::filter::filter_status_t status)
{
    switch (status) {
        case xtcore::filter::FILTER_STATUS_STABLE: return "stable";
        case xtcore::filter::FILTER_STATUS_EXPERIMENTAL: return "experimental";
        case xtcore::filter::FILTER_STATUS_LEGACY: return "legacy";
        case xtcore::filter::FILTER_STATUS_HIDDEN: return "hidden";
        default: return "stable";
    }
}

bool finite_aabb3(const xtcore::AABB3 &box)
{
    return std::isfinite((double)box.min.x) && std::isfinite((double)box.min.y) && std::isfinite((double)box.min.z)
        && std::isfinite((double)box.max.x) && std::isfinite((double)box.max.y) && std::isfinite((double)box.max.z);
}

bool parse_u64_param(const params_view &params, const char *key, size_t min_v, size_t max_v, size_t &out)
{
    if (!params.has(key)) return false;
    std::string s = params.str(key);
    if (s.empty()) return false;

    for (size_t i = 0; i < s.size(); ++i) {
        if (!std::isdigit((unsigned char)s[i])) return false;
    }

    std::istringstream ss(s);
    unsigned long long v = 0;
    ss >> v;
    if (ss.fail()) return false;
    if (v < min_v || v > max_v) return false;
    out = (size_t)v;
    return true;
}

bool parse_f64_param(const params_view &params, const char *key, double min_v, double max_v, double &out)
{
    if (!params.has(key)) return false;
    const std::string s = params.str(key);
    if (s.empty()) return false;
    std::istringstream ss(s);
    double v = 0.0;
    ss >> v;
    if (ss.fail() || !std::isfinite(v)) return false;
    if (v < min_v || v > max_v) return false;
    out = v;
    return true;
}

bool parse_tile_order_param(const params_view &params, const char *key, xtcore::render::TILE_ORDER &out)
{
    if (!params.has(key)) return false;
    std::string s = params.str(key);
    if (s == "scanline") {
        out = xtcore::render::TILE_ORDER_SCANLINE;
        return true;
    }
    if (s == "random") {
        out = xtcore::render::TILE_ORDER_RANDOM;
        return true;
    }
    if (s == "radial_in") {
        out = xtcore::render::TILE_ORDER_RADIAL_IN;
        return true;
    }
    if (s == "radial_out") {
        out = xtcore::render::TILE_ORDER_RADIAL_OUT;
        return true;
    }
    if (s == "spiral_in") {
        out = xtcore::render::TILE_ORDER_SPIRAL_IN;
        return true;
    }
    if (s == "spiral_out") {
        out = xtcore::render::TILE_ORDER_SPIRAL_OUT;
        return true;
    }
    return false;
}

bool parse_render_mode_param(const params_view &params,
                             const char *key,
                             common::render_request_t::render_mode_t &out)
{
    if (!params.has(key)) return false;
    std::string s = params.str(key);
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });
    if (s == "direct" || s == "normal") {
        out = common::render_request_t::RENDER_MODE_DIRECT;
        return true;
    }
    if (s == "progressive") {
        out = common::render_request_t::RENDER_MODE_PROGRESSIVE;
        return true;
    }
    if (s == "incremental") {
        out = common::render_request_t::RENDER_MODE_INCREMENTAL;
        return true;
    }
    if (s == "interactive") {
        out = common::render_request_t::RENDER_MODE_INTERACTIVE;
        return true;
    }
    return false;
}

bool parse_sample_distribution_param(const params_view &params,
                                     const char *key,
                                     xtcore::antialiasing::SAMPLE_DISTRIBUTION &out)
{
    if (!params.has(key)) return false;
    std::string s = params.str(key);
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });
    if (s == "grid" || s == "grid_aligned") {
        out = xtcore::antialiasing::SAMPLE_DISTRIBUTION_GRID;
        return true;
    }
    if (s == "random" || s == "jittered") {
        out = xtcore::antialiasing::SAMPLE_DISTRIBUTION_RANDOM;
        return true;
    }
    return false;
}

bool parse_tonemapping_operator(const std::string &s, xtcore::tonemapping::operator_t &out)
{
    if (s == "aces")          { out = xtcore::tonemapping::OP_ACES_FITTED;         return true; }
    if (s == "reinhard")      { out = xtcore::tonemapping::OP_REINHARD;            return true; }
    if (s == "reinhard_luma") { out = xtcore::tonemapping::OP_REINHARD_LUMINANCE;  return true; }
    if (s == "mantiuk_2006")  { out = xtcore::tonemapping::OP_MANTIUK_2006;        return true; }
    if (s == "hable")         { out = xtcore::tonemapping::OP_HABLE;               return true; }
    if (s == "exponential")   { out = xtcore::tonemapping::OP_EXPONENTIAL;         return true; }
    if (s == "lottes")        { out = xtcore::tonemapping::OP_LOTTES;              return true; }
    if (s == "cineon")        { out = xtcore::tonemapping::OP_CINEON;              return true; }
    if (s == "uchimura")      { out = xtcore::tonemapping::OP_UCHIMURA;            return true; }
    if (s == "agx")           { out = xtcore::tonemapping::OP_AGX;                 return true; }
    if (s == "khronos_pbr")   { out = xtcore::tonemapping::OP_KHRONOS_PBR_NEUTRAL; return true; }
    if (s == "none")          { out = xtcore::tonemapping::OP_NONE;                return true; }
    return false;
}

bool parse_tonemapping_settings(const params_view &params, xtcore::tonemapping::settings_t &tm_settings, std::string &error_json)
{
    if (params.has("tm")) {
        std::string tm = params.str("tm");
        std::transform(tm.begin(), tm.end(), tm.begin(),
            [](unsigned char c) { return (char)std::tolower(c); });
        if (!parse_tonemapping_operator(tm, tm_settings.op)) {
            error_json = "{\"error\":\"invalid tone mapping operator\"}";
            return false;
        }
    }
    if (params.has("tm_exposure")) {
        std::istringstream es(params.str("tm_exposure"));
        float exposure = 1.0f;
        es >> exposure;
        if (es.fail() || exposure <= 0.0f) {
            error_json = "{\"error\":\"invalid tone mapping exposure\"}";
            return false;
        }
        tm_settings.exposure = exposure;
    }
    if (params.has("tm_white_point")) {
        std::istringstream ws(params.str("tm_white_point"));
        float white_point = 1.0f;
        ws >> white_point;
        if (ws.fail() || white_point <= 0.0f) {
            error_json = "{\"error\":\"invalid tone mapping white point\"}";
            return false;
        }
        tm_settings.white_point = white_point;
    }
    if (params.has("tm_mantiuk_contrast")) {
        std::istringstream cs(params.str("tm_mantiuk_contrast"));
        float v = 0.1f;
        cs >> v;
        if (cs.fail() || v < 0.0f || v > 1.0f) {
            error_json = "{\"error\":\"invalid mantiuk contrast\"}";
            return false;
        }
        tm_settings.mantiuk_contrast = v;
    }
    if (params.has("tm_mantiuk_saturation")) {
        std::istringstream ss(params.str("tm_mantiuk_saturation"));
        float v = 0.8f;
        ss >> v;
        if (ss.fail() || v < 0.0f || v > 2.0f) {
            error_json = "{\"error\":\"invalid mantiuk saturation\"}";
            return false;
        }
        tm_settings.mantiuk_saturation = v;
    }
    if (params.has("tm_mantiuk_detail")) {
        std::istringstream ds(params.str("tm_mantiuk_detail"));
        float v = 1.0f;
        ds >> v;
        if (ds.fail() || v < 1.0f || v > 99.0f) {
            error_json = "{\"error\":\"invalid mantiuk detail\"}";
            return false;
        }
        tm_settings.mantiuk_detail = v;
    }
    return true;
}

bool parse_post_filter_settings(const params_view &params,
                                bool &enabled,
                                std::string &post_filters,
                                std::string &error_json)
{
    enabled = false;
    post_filters.clear();
    error_json.clear();

    if (params.has("post_filters_enabled")) {
        const std::string raw = params.str("post_filters_enabled");
        if (raw == "1" || raw == "true") enabled = true;
        else if (raw == "0" || raw == "false" || raw.empty()) enabled = false;
        else {
            error_json = "{\"error\":\"invalid post_filters_enabled\"}";
            return false;
        }
    }

    if (params.has("post_filters")) {
        post_filters = params.str("post_filters");
    }

    if (!enabled) {
        post_filters.clear();
        return true;
    }

    for (size_t i = 0; i < post_filters.size(); ++i) {
        const unsigned char c = (unsigned char)post_filters[i];
        if (std::isalnum(c) || c == '_' || c == ':' || c == ',' || c == '-' || c == '.' || c == '=') continue;
        if (std::isspace(c)) continue;
        error_json = "{\"error\":\"invalid post_filters\"}";
        return false;
    }
    return true;
}


bool has_suffix(const std::string &s, const std::string &suffix)
{
    if (s.size() < suffix.size()) return false;
    return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool has_prefix(const std::string &s, const char *prefix)
{
    if (!prefix) return false;
    const size_t n = std::strlen(prefix);
    return s.size() >= n && s.compare(0, n, prefix) == 0;
}

std::string trim_ascii(const std::string &s)
{
    size_t start = 0;
    while (start < s.size() && std::isspace((unsigned char)s[start])) ++start;

    size_t end = s.size();
    while (end > start && std::isspace((unsigned char)s[end - 1])) --end;

    return s.substr(start, end - start);
}

std::string third_party_licenses_data_path(const std::string &web_root)
{
    if (web_root.empty()) {
        return "src/frontend/web-server/app/data/third_party_licenses.json";
    }

    std::string path = web_root;
    while (!path.empty() && (path[path.size() - 1] == '/' || path[path.size() - 1] == '\\')) {
        path.erase(path.size() - 1, 1);
    }

    const std::string suffix = "/web-client";
    if (path.size() >= suffix.size() && path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0) {
        return path.substr(0, path.size() - suffix.size()) + "/web-server/app/data/third_party_licenses.json";
    }

    return path + "/../web-server/app/data/third_party_licenses.json";
}

void append_third_party_licenses_json(std::ostringstream &ss, const std::string &web_root)
{
    std::string json;
    if (!read_text_file(third_party_licenses_data_path(web_root), json)) {
        backend_log_t::handle().add("warning", "failed to read third-party dependency registry json");
        ss << "[]";
        return;
    }

    json = trim_ascii(json);
    if (json.empty()) {
        ss << "[]";
        return;
    }

    ss << json;
}

std::string utc_timestamp_for_filename()
{
    const std::time_t now = std::time(nullptr);
    std::tm tm_utc;
#if defined(_WIN32)
    gmtime_s(&tm_utc, &now);
#else
    gmtime_r(&now, &tm_utc);
#endif
    char buf[32];
    if (std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm_utc) == 0) {
        return "unknown_time";
    }
    return std::string(buf);
}

std::string sanitize_filename_token(const std::string &value, const char *fallback)
{
    std::string out;
    out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        const unsigned char c = (unsigned char)value[i];
        if (std::isalnum(c) || c == '_' || c == '-' || c == '.') out.push_back((char)c);
        else out.push_back('_');
    }

    while (!out.empty() && out[0] == '_') out.erase(out.begin());
    while (!out.empty() && out[out.size() - 1] == '_') out.erase(out.size() - 1, 1);

    if (out.empty()) return std::string(fallback ? fallback : "value");
    return out;
}

std::string scene_basename_for_filename(const std::string &scene)
{
    if (scene.size() >= 4 && scene.compare(scene.size() - 4, 4, ".scn") == 0) {
        return scene.substr(0, scene.size() - 4);
    }
    return scene;
}

std::string lower_ascii(std::string s)
{
    for (size_t i = 0; i < s.size(); ++i) {
        s[i] = (char)std::tolower((unsigned char)s[i]);
    }
    return s;
}

void append_integrator_controls_json(std::ostringstream &ss, const common::integrator_info_t &integrator)
{
    ss << "\"controls\":[";
    for (size_t j = 0; j < integrator.controls_count; ++j) {
        const common::integrator_control_info_t &ctrl = integrator.controls[j];
        if (j) ss << ',';
        ss << "{"
           << "\"id\":\"" << json_escape(ctrl.id ? ctrl.id : "") << "\","
           << "\"label\":\"" << json_escape(ctrl.label ? ctrl.label : "") << "\","
           << "\"type\":\"" << json_escape(ctrl.type ? ctrl.type : "") << "\"";
        if (ctrl.description && *(ctrl.description)) {
            ss << ",\"description\":\"" << json_escape(ctrl.description) << "\"";
        }
        if (ctrl.default_value && *(ctrl.default_value)) {
            ss << ",\"default\":\"" << json_escape(ctrl.default_value) << "\"";
        }
        if (ctrl.min_value && *(ctrl.min_value)) {
            ss << ",\"min\":\"" << json_escape(ctrl.min_value) << "\"";
        }
        if (ctrl.max_value && *(ctrl.max_value)) {
            ss << ",\"max\":\"" << json_escape(ctrl.max_value) << "\"";
        }
        if (ctrl.step_value && *(ctrl.step_value)) {
            ss << ",\"step\":\"" << json_escape(ctrl.step_value) << "\"";
        }
        if (ctrl.visible_when_id && *(ctrl.visible_when_id)
            && ctrl.visible_when_value && *(ctrl.visible_when_value)) {
            ss << ",\"visible_when\":{"
               << "\"id\":\"" << json_escape(ctrl.visible_when_id) << "\","
               << "\"value\":\"" << json_escape(ctrl.visible_when_value) << "\"}";
        }

        if (ctrl.options_count > 0 && ctrl.options) {
            ss << ",\"options\":[";
            for (size_t k = 0; k < ctrl.options_count; ++k) {
                if (k) ss << ',';
                const common::integrator_control_option_t &opt = ctrl.options[k];
                ss << "{\"value\":\"" << json_escape(opt.value ? opt.value : "")
                   << "\",\"label\":\"" << json_escape(opt.label ? opt.label : "") << "\"}";
            }
            ss << "]";
        }
        ss << "}";
    }
    ss << "]";
}

void append_post_filter_params_json(std::ostringstream &ss, const post_filter_info_t &filter)
{
    ss << "\"params\":[";
    for (size_t j = 0; j < filter.params_count; ++j) {
        const post_filter_param_info_t &param = filter.params[j];
        if (j) ss << ',';
        ss << "{"
           << "\"id\":\"" << json_escape(param.id ? param.id : "") << "\","
           << "\"label\":\"" << json_escape(param.label ? param.label : "") << "\","
           << "\"type\":\"" << json_escape(param.type ? param.type : "") << "\"";
        if (param.description && *(param.description)) {
            ss << ",\"description\":\"" << json_escape(param.description) << "\"";
        }
        if (param.default_value && *(param.default_value)) {
            ss << ",\"default\":\"" << json_escape(param.default_value) << "\"";
        }
        if (param.min_value && *(param.min_value)) {
            ss << ",\"min\":\"" << json_escape(param.min_value) << "\"";
        }
        if (param.max_value && *(param.max_value)) {
            ss << ",\"max\":\"" << json_escape(param.max_value) << "\"";
        }
        if (param.step_value && *(param.step_value)) {
            ss << ",\"step\":\"" << json_escape(param.step_value) << "\"";
        }
        if (param.options_count > 0 && param.options) {
            ss << ",\"options\":[";
            for (size_t k = 0; k < param.options_count; ++k) {
                if (k) ss << ',';
                const post_filter_param_option_t &opt = param.options[k];
                ss << "{\"value\":\"" << json_escape(opt.value ? opt.value : "")
                   << "\",\"label\":\"" << json_escape(opt.label ? opt.label : "") << "\"}";
            }
            ss << "]";
        }
        ss << "}";
    }
    ss << "]";
}

bool is_scene_name_safe(const std::string &scene)
{
    if (scene.empty()) return false;
    if (scene.find('/')  != std::string::npos) return false;
    if (scene.find('\\') != std::string::npos) return false;
    if (scene.find("..") != std::string::npos) return false;
    return has_suffix(scene, ".scn");
}

bool is_client_id_safe(const std::string &client_id)
{
    if (client_id.empty() || client_id.size() > 96) return false;
    for (size_t i = 0; i < client_id.size(); ++i) {
        const unsigned char c = (unsigned char)client_id[i];
        if (!(std::isalnum(c) || c == '_' || c == '-' || c == '.')) return false;
    }
    return true;
}

std::string read_client_id(const params_view &params)
{
    if (!params.has("client_id")) return "";
    const std::string client_id = params.str("client_id");
    if (!is_client_id_safe(client_id)) return "";
    return client_id;
}

bool is_variant_name_safe(const std::string &variant)
{
    if (variant.empty()) return false;
    if (variant.size() > 96) return false;
    if (variant.find('/') != std::string::npos) return false;
    if (variant.find('\\') != std::string::npos) return false;
    if (variant.find("..") != std::string::npos) return false;
    for (size_t i = 0; i < variant.size(); ++i) {
        const unsigned char c = (unsigned char)variant[i];
        if (!(std::isalnum(c) || c == '_' || c == '-' || c == '.')) return false;
    }
    return true;
}

bool read_variant_name(const params_view &params, std::string &variant, std::string &error)
{
    variant.clear();
    error.clear();
    if (!params.has("variant")) return true;
    variant = params.str("variant");
    if (variant.empty()) return true;
    if (!is_variant_name_safe(variant)) {
        error = "invalid variant";
        return false;
    }
    return true;
}

bool write_workspace_temp_scene(const std::string &workspace_id,
                                const std::string &scene_name,
                                const std::string &source,
                                std::string &out_path)
{
    std::string ws = workspace_id.empty() ? "ws" : workspace_id;
    for (size_t i = 0; i < ws.size(); ++i) {
        const unsigned char c = (unsigned char)ws[i];
        if (!(std::isalnum(c) || c == '_' || c == '-')) ws[i] = '_';
    }
    std::string scene = scene_name.empty() ? "scene" : scene_name;
    for (size_t i = 0; i < scene.size(); ++i) {
        const unsigned char c = (unsigned char)scene[i];
        if (!(std::isalnum(c) || c == '_' || c == '-')) scene[i] = '_';
    }
    std::string pattern = "/tmp/xtracer_ws_" + ws + "_" + scene + "_XXXXXX.scn";
    std::vector<char> buf(pattern.begin(), pattern.end());
    buf.push_back('\0');
    int fd = mkstemps(buf.data(), 4);
    if (fd < 0) return false;
    close(fd);
    out_path = std::string(buf.data());
    if (!write_text_file(out_path, source)) {
        unlink(out_path.c_str());
        out_path.clear();
        return false;
    }
    return true;
}

bool is_scene_basename_safe(const std::string &name)
{
    if (name.empty()) return false;
    if (name.find('/')  != std::string::npos) return false;
    if (name.find('\\') != std::string::npos) return false;
    if (name.find("..") != std::string::npos) return false;

    for (size_t i = 0; i < name.size(); ++i) {
        const unsigned char c = (unsigned char)name[i];
        if (!(std::isalnum(c) || c == '_' || c == '-' || c == '.')) return false;
    }
    return true;
}

std::string normalize_scene_name(const std::string &name)
{
    return has_suffix(name, ".scn") ? name : (name + ".scn");
}

std::string join_path(const std::string &a, const std::string &b)
{
    if (a.empty()) return b;
    if (a[a.size() - 1] == '/') return a + b;
    return a + "/" + b;
}

std::string dirname_path(const std::string &path)
{
    if (path.empty()) return "";
    size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) return "";
    if (slash == 0) return "/";
    return path.substr(0, slash);
}

std::string normalize_path_slashes(const std::string &path)
{
    if (path.empty()) return std::string();
    std::string out = path;
    for (size_t i = 0; i < out.size(); ++i) {
        if (out[i] == '\\') out[i] = '/';
    }
    return out;
}

std::string make_asset_relpath_for_scene(const std::string &scene_path, const std::string &asset_path)
{
    const std::string full = normalize_path_slashes(asset_path);
    if (full.empty()) return std::string();
    if (full[0] != '/') return full;

    const std::string scene_dir = normalize_path_slashes(dirname_path(scene_path));
    const std::string root_dir = normalize_path_slashes(dirname_path(scene_dir));
    if (root_dir.empty()) return std::string();

    std::string prefix = root_dir;
    if (!prefix.empty() && prefix[prefix.size() - 1] != '/') prefix += "/";
    if (full.size() <= prefix.size()) return std::string();
    if (full.compare(0, prefix.size(), prefix) != 0) return std::string();
    return full.substr(prefix.size());
}

bool is_asset_relpath_safe(const std::string &path)
{
    if (path.empty()) return false;
    if (path[0] == '/' || path[0] == '\\') return false;
    if (path.find("..") != std::string::npos) return false;
    if (path.find('\\') != std::string::npos) return false;
    if (path.find(':') != std::string::npos) return false;
    return true;
}

std::vector<std::string> list_scenes(const std::string &scene_dir)
{
    std::vector<std::string> out;
    DIR *dir = opendir(scene_dir.c_str());
    if (!dir) return out;

    struct dirent *entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (is_scene_name_safe(name)) out.push_back(name);
    }
    closedir(dir);

    std::sort(out.begin(), out.end());
    return out;
}

struct camera_list_info_t
{
    struct camera_entry_t {
        std::string name;
        std::string type;
    };

    std::vector<std::string> cameras;
    std::vector<camera_entry_t> camera_entries;
    std::string default_camera;
};

bool file_mtime(const std::string &path, std::uint64_t &out)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    std::uint64_t sec = (st.st_mtime >= 0) ? (std::uint64_t)st.st_mtime : 0ULL;
    std::uint64_t nsec = 0ULL;
#if defined(__linux__)
    nsec = (st.st_mtim.tv_nsec >= 0) ? (std::uint64_t)st.st_mtim.tv_nsec : 0ULL;
#elif defined(__APPLE__)
    nsec = (st.st_mtimespec.tv_nsec >= 0) ? (std::uint64_t)st.st_mtimespec.tv_nsec : 0ULL;
#endif
    const std::uint64_t size = (st.st_size >= 0) ? (std::uint64_t)st.st_size : 0ULL;
    out = ((sec & 0xffffffffULL) << 32) ^ (nsec & 0xffffffffULL) ^ (size * 0x9e3779b97f4a7c15ULL);
    return true;
}

HASH_ID find_camera_id_by_name(const xtcore::Scene &scene, const std::string &name)
{
    if (name.empty()) return HASH_ID_INVALID;
    for (auto it = scene.m_cameras.begin(); it != scene.m_cameras.end(); ++it) {
        const char *camera_name = xtcore::pool::str::get((*it).first);
        if (camera_name && name == camera_name) return (*it).first;
    }
    return HASH_ID_INVALID;
}

camera_list_info_t list_cameras_from_scene(const xtcore::Scene &scene)
{
    camera_list_info_t out;
    for (auto it = scene.m_cameras.begin(); it != scene.m_cameras.end(); ++it) {
        const char *name = xtcore::pool::str::get((*it).first);
        if (!name || !*name) continue;
        const xtcore::asset::ICamera *cam = (*it).second;
        camera_list_info_t::camera_entry_t entry;
        entry.name = name;
        entry.type = (cam && cam->get_type()) ? cam->get_type() : "camera";
        out.camera_entries.push_back(entry);
    }
    std::sort(out.camera_entries.begin(), out.camera_entries.end(),
              [](const camera_list_info_t::camera_entry_t &a, const camera_list_info_t::camera_entry_t &b) {
                  return a.name < b.name;
              });
    out.cameras.reserve(out.camera_entries.size());
    for (size_t i = 0; i < out.camera_entries.size(); ++i) {
        out.cameras.push_back(out.camera_entries[i].name);
    }
    out.default_camera = scene.m_default_camera;
    return out;
}

std::string scene_geometry_json_from_scene(xtcore::Scene &scene)
{
    std::ostringstream ss;
    ss << "{\"meshes\":{";
    bool first_mesh = true;

    for (auto it = scene.m_surface.begin(); it != scene.m_surface.end(); ++it) {
        const xtcore::asset::ISurface *surface = (*it).second;
        const xtcore::surface::Mesh *mesh = dynamic_cast<const xtcore::surface::Mesh *>(surface);
        if (!mesh) continue;

        const char *name = xtcore::pool::str::get((*it).first);
        if (!name || !*name) continue;

        if (!first_mesh) ss << ",";
        first_mesh = false;

        ss << "\"" << json_escape(name) << "\":{";
        ss << "\"positions\":[";

        const std::vector<xtcore::surface::Triangle> &tris = mesh->triangles();
        bool first_value = true;
        for (size_t i = 0; i < tris.size(); ++i) {
            const xtcore::surface::Triangle &tri = tris[i];
            for (size_t v = 0; v < 3; ++v) {
                if (!first_value) ss << ",";
                first_value = false;
                ss << tri.v[v].x << "," << tri.v[v].y << "," << tri.v[v].z;
            }
        }
        ss << "],\"normals\":[";

        first_value = true;
        for (size_t i = 0; i < tris.size(); ++i) {
            const xtcore::surface::Triangle &tri = tris[i];
            for (size_t v = 0; v < 3; ++v) {
                if (!first_value) ss << ",";
                first_value = false;
                ss << tri.n[v].x << "," << tri.n[v].y << "," << tri.n[v].z;
            }
        }
        ss << "],\"uvs\":[";

        first_value = true;
        for (size_t i = 0; i < tris.size(); ++i) {
            const xtcore::surface::Triangle &tri = tris[i];
            for (size_t v = 0; v < 3; ++v) {
                if (!first_value) ss << ",";
                first_value = false;
                ss << tri.tc[v].x << "," << tri.tc[v].y;
            }
        }
        ss << "]}";
    }

    ss << "},\"debug\":{";

    std::vector<xtcore::AABB3> global_bvh;
    scene.collect_tlas_aabbs(global_bvh);
    ss << "\"global_bvh\":[";
    for (size_t i = 0; i < global_bvh.size(); ++i) {
        if (i > 0) ss << ",";
        const xtcore::AABB3 &box = global_bvh[i];
        ss << "{\"min\":["
           << box.min.x << "," << box.min.y << "," << box.min.z
           << "],\"max\":["
           << box.max.x << "," << box.max.y << "," << box.max.z
           << "]}";
    }
    ss << "],\"mesh_bvh\":{";

    first_mesh = true;
    for (auto it = scene.m_surface.begin(); it != scene.m_surface.end(); ++it) {
        const xtcore::surface::Mesh *mesh = dynamic_cast<const xtcore::surface::Mesh *>((*it).second);
        if (!mesh) continue;
        const char *name = xtcore::pool::str::get((*it).first);
        if (!name || !*name) continue;
        if (!first_mesh) ss << ",";
        first_mesh = false;
        ss << "\"" << json_escape(name) << "\":[";
        std::vector<xtcore::AABB3> mesh_boxes;
        mesh->collect_bvh_aabbs(mesh_boxes);
        for (size_t i = 0; i < mesh_boxes.size(); ++i) {
            if (i > 0) ss << ",";
            const xtcore::AABB3 &box = mesh_boxes[i];
            ss << "{\"min\":["
               << box.min.x << "," << box.min.y << "," << box.min.z
               << "],\"max\":["
               << box.max.x << "," << box.max.y << "," << box.max.z
               << "]}";
        }
        ss << "]";
    }
    ss << "}}}";
    return ss.str();
}

const char *surface_type_name(const xtcore::asset::ISurface *surface)
{
    if (!surface) return "surface";
    if (dynamic_cast<const xtcore::surface::Plane *>(surface)) return "plane";
    if (dynamic_cast<const xtcore::surface::Sphere *>(surface)) return "sphere";
    if (dynamic_cast<const xtcore::surface::Triangle *>(surface)) return "triangle";
    if (dynamic_cast<const xtcore::surface::MengerSponge *>(surface)) return "menger_sponge";
    if (dynamic_cast<const xtcore::surface::SierpinskiTetrahedron *>(surface)) return "sierpinski_tetrahedron";
    if (dynamic_cast<const xtcore::surface::Mandelbulb *>(surface)) return "mandelbulb";
    if (dynamic_cast<const xtcore::surface::JuliaFractal *>(surface)) return "julia";
    if (dynamic_cast<const xtcore::surface::CSG *>(surface)) return "csg";
    if (dynamic_cast<const xtcore::surface::Mesh *>(surface)) return "mesh";
    return "surface";
}

const char *csg_op_name(xtcore::surface::CSG::op_t op)
{
    switch (op) {
        case xtcore::surface::CSG::OP_UNION:
            return "union";
        case xtcore::surface::CSG::OP_SOFT_UNION:
            return "soft_union";
        case xtcore::surface::CSG::OP_INTERSECTION:
            return "intersection";
        case xtcore::surface::CSG::OP_DIFFERENCE:
            return "difference";
        default:
            return "union";
    }
}

const char *material_type_name(const xtcore::asset::IMaterial *mat)
{
    if (!mat) return "material";
    if (dynamic_cast<const xtcore::asset::material::Lambert *>(mat)) return "lambert";
    if (dynamic_cast<const xtcore::asset::material::Phong *>(mat)) return "phong";
    if (dynamic_cast<const xtcore::asset::material::BlinnPhong *>(mat)) return "blinn_phong";
    if (dynamic_cast<const xtcore::asset::material::Emissive *>(mat)) return "emissive";
    if (dynamic_cast<const xtcore::asset::material::Dielectric *>(mat)) return "dielectric";
    if (dynamic_cast<const xtcore::asset::material::Principled *>(mat)) return "principled";
    if (dynamic_cast<const xtcore::asset::material::RoughDielectric *>(mat)) return "rough_dielectric";
    if (dynamic_cast<const xtcore::asset::material::ThinDielectric *>(mat)) return "thin_dielectric";
    if (dynamic_cast<const xtcore::asset::material::Subsurface *>(mat)) return "subsurface";
    if (dynamic_cast<const xtcore::asset::material::Sheen *>(mat)) return "sheen";
    if (dynamic_cast<const xtcore::asset::material::ThinTranslucent *>(mat)) return "thin_translucent";
    if (dynamic_cast<const xtcore::asset::material::Boundary *>(mat)) return "boundary";
    return "material";
}

const char *sampler_type_name(const xtcore::sampler::ISampler *sampler)
{
    if (!sampler) return "sampler";
    if (dynamic_cast<const xtcore::sampler::Texture2D *>(sampler)) return "texture";
    if (dynamic_cast<const xtcore::sampler::Cubemap *>(sampler)) return "cubemap";
    if (dynamic_cast<const xtcore::sampler::ERP *>(sampler)) return "erp";
    if (dynamic_cast<const xtcore::sampler::Gradient *>(sampler)) return "gradient";
    if (dynamic_cast<const xtcore::sampler::RayleighSky *>(sampler)) return "rayleigh_sky";
    if (dynamic_cast<const xtcore::sampler::Checker *>(sampler)) return "checker";
    if (dynamic_cast<const xtcore::sampler::GraphPaper *>(sampler)) return "graphpaper";
    if (dynamic_cast<const xtcore::sampler::Weave *>(sampler)) return "weave";
    if (dynamic_cast<const xtcore::sampler::FBMMarble *>(sampler)) return "fbm_marble";
    if (dynamic_cast<const xtcore::sampler::SolidColor *>(sampler)) return "color";
    return "sampler";
}

const char *medium_type_name(const xtcore::asset::medium::IMedium *medium)
{
    if (!medium) return "medium";
    if (dynamic_cast<const xtcore::asset::medium::Homogeneous *>(medium)) return "homogeneous";
    if (dynamic_cast<const xtcore::asset::medium::HeterogeneousNoise *>(medium)) return "heterogeneous_noise";
    return "medium";
}

std::string scene_runtime_graph_json_from_scene(const std::string &scene_path, const xtcore::Scene &scene)
{
    std::ostringstream ss;
    ss << "{";

    ss << "\"cameras\":[";
    bool first = true;
    for (auto it = scene.m_cameras.begin(); it != scene.m_cameras.end(); ++it) {
        const char *name = xtcore::pool::str::get((*it).first);
        if (!name || !*name) continue;
        const xtcore::asset::ICamera *cam = (*it).second;
        const xtcore::camera::Perspective *pcam  = dynamic_cast<const xtcore::camera::Perspective *>(cam);
        const xtcore::camera::TiltShift   *tscam = dynamic_cast<const xtcore::camera::TiltShift *>(cam);
        const xtcore::camera::ERP         *ecam  = dynamic_cast<const xtcore::camera::ERP *>(cam);
        const xtcore::camera::ODS         *ocam  = dynamic_cast<const xtcore::camera::ODS *>(cam);
        if (!first) ss << ",";
        first = false;
        ss << "{"
           << "\"id\":\"" << json_escape(name) << "\","
           << "\"type\":\"" << json_escape(cam ? cam->get_type() : "camera") << "\"";
        if (cam) {
            ss << ",\"position\":[" << cam->position.x << "," << cam->position.y << "," << cam->position.z << "]";
        }
        if (pcam) {
            ss << ",\"target\":[" << pcam->target.x << "," << pcam->target.y << "," << pcam->target.z << "]";
            ss << ",\"up\":[" << pcam->up.x << "," << pcam->up.y << "," << pcam->up.z << "]";
            ss << ",\"fov\":" << pcam->fov;
            ss << ",\"aperture\":" << pcam->aperture;
            ss << ",\"flength\":" << pcam->flength;
            ss << ",\"aperture_blades\":" << pcam->aperture_blades;
            ss << ",\"aperture_rotation\":" << pcam->aperture_rotation;
        } else if (tscam) {
            ss << ",\"target\":[" << tscam->target.x << "," << tscam->target.y << "," << tscam->target.z << "]";
            ss << ",\"up\":[" << tscam->up.x << "," << tscam->up.y << "," << tscam->up.z << "]";
            ss << ",\"fov\":" << tscam->fov;
            ss << ",\"aperture\":" << tscam->aperture;
            ss << ",\"flength\":" << tscam->flength;
            ss << ",\"aperture_blades\":" << tscam->aperture_blades;
            ss << ",\"aperture_rotation\":" << tscam->aperture_rotation;
            ss << ",\"tilt\":" << tscam->tilt;
            ss << ",\"shift_x\":" << tscam->shift_x;
            ss << ",\"shift_y\":" << tscam->shift_y;
        } else if (ecam) {
            ss << ",\"orientation\":[" << ecam->orientation.x << "," << ecam->orientation.y << "," << ecam->orientation.z << "]";
        } else if (ocam) {
            ss << ",\"orientation\":[" << ocam->orientation.x << "," << ocam->orientation.y << "," << ocam->orientation.z << "]";
            ss << ",\"ipd\":" << ocam->ipd;
        }
        ss << "}";
    }
    ss << "],";

    ss << "\"surfaces\":[";
    first = true;
    for (auto it = scene.m_surface.begin(); it != scene.m_surface.end(); ++it) {
        const char *name = xtcore::pool::str::get((*it).first);
        if (!name || !*name) continue;
        const xtcore::asset::ISurface *surface = (*it).second;
        if (!first) ss << ",";
        first = false;
        ss << "{"
           << "\"id\":\"" << json_escape(name) << "\","
           << "\"type\":\"" << surface_type_name(surface) << "\"";
        if (const xtcore::surface::Sphere *sphere = dynamic_cast<const xtcore::surface::Sphere *>(surface)) {
            ss << ",\"position\":[" << sphere->origin.x << "," << sphere->origin.y << "," << sphere->origin.z << "]";
            ss << ",\"radius\":" << sphere->radius;
        } else if (const xtcore::surface::Plane *plane = dynamic_cast<const xtcore::surface::Plane *>(surface)) {
            ss << ",\"normal\":[" << plane->normal.x << "," << plane->normal.y << "," << plane->normal.z << "]";
            ss << ",\"distance\":" << plane->offset;
        } else if (const xtcore::surface::Triangle *tri = dynamic_cast<const xtcore::surface::Triangle *>(surface)) {
            ss << ",\"v0\":[" << tri->v[0].x << "," << tri->v[0].y << "," << tri->v[0].z << "]";
            ss << ",\"v1\":[" << tri->v[1].x << "," << tri->v[1].y << "," << tri->v[1].z << "]";
            ss << ",\"v2\":[" << tri->v[2].x << "," << tri->v[2].y << "," << tri->v[2].z << "]";
        } else if (const xtcore::surface::CSG *csg = dynamic_cast<const xtcore::surface::CSG *>(surface)) {
            ss << ",\"op\":\"" << csg_op_name(csg->op) << "\"";
            ss << ",\"smoothness\":" << csg->smoothness;
            ss << ",\"left_type\":\"" << surface_type_name(csg->left) << "\"";
            ss << ",\"right_type\":\"" << surface_type_name(csg->right) << "\"";
        } else if (const xtcore::surface::Mesh *mesh = dynamic_cast<const xtcore::surface::Mesh *>(surface)) {
            ss << ",\"triangles\":" << mesh->triangles().size();
        }
        if (surface && finite_aabb3(surface->aabb)) {
            ss << ",\"bounds_min\":[" << surface->aabb.min.x << "," << surface->aabb.min.y << "," << surface->aabb.min.z << "]";
            ss << ",\"bounds_max\":[" << surface->aabb.max.x << "," << surface->aabb.max.y << "," << surface->aabb.max.z << "]";
        }
        ss
           << "}";
    }
    ss << "],";

    ss << "\"materials\":[";
    first = true;
    for (auto it = scene.m_materials.begin(); it != scene.m_materials.end(); ++it) {
        const char *name = xtcore::pool::str::get((*it).first);
        if (!name || !*name) continue;
        const xtcore::asset::IMaterial *mat = (*it).second;
        if (!first) ss << ",";
        first = false;
        ss << "{"
           << "\"id\":\"" << json_escape(name) << "\","
           << "\"type\":\"" << material_type_name(mat) << "\"";

        ss << ",\"scalars\":[";
        bool first_scalar = true;
        if (mat) {
            xtcore::asset::IMaterial *rw_mat = const_cast<xtcore::asset::IMaterial *>(mat);
            for (size_t i = 0; i < mat->get_scalar_count(); ++i) {
                std::string scalar_name;
                const float scalar_value = rw_mat->get_scalar_by_index(i, &scalar_name);
                if (!first_scalar) ss << ",";
                first_scalar = false;
                ss << "{"
                   << "\"name\":\"" << json_escape(scalar_name) << "\","
                   << "\"value\":" << scalar_value
                   << "}";
            }
        }
        ss << "]";

        ss << ",\"samplers\":[";
        bool first_sampler = true;
        if (mat) {
            xtcore::asset::IMaterial *rw_mat = const_cast<xtcore::asset::IMaterial *>(mat);
            for (size_t i = 0; i < mat->get_sampler_count(); ++i) {
                std::string sampler_name;
                xtcore::sampler::ISampler *sampler = rw_mat->get_sampler_by_index(i, &sampler_name);
                if (!sampler) continue;
                if (!first_sampler) ss << ",";
                first_sampler = false;

                std::string texture_asset_relpath;
                const xtcore::sampler::Texture2D *tex = dynamic_cast<const xtcore::sampler::Texture2D *>(sampler);
                if (tex) {
                    texture_asset_relpath = make_asset_relpath_for_scene(scene_path, tex->source_path());
                }

                ss << "{"
                   << "\"name\":\"" << json_escape(sampler_name) << "\","
                   << "\"type\":\"" << sampler_type_name(sampler) << "\"";
                const xtcore::sampler::SolidColor *solid = dynamic_cast<const xtcore::sampler::SolidColor *>(sampler);
                if (solid) {
                    nimg::ColorRGBf c;
                    xtcore::sampler::SolidColor *rw_solid = const_cast<xtcore::sampler::SolidColor *>(solid);
                    rw_solid->get(c);
                    ss << ",\"color\":[" << c.r() << "," << c.g() << "," << c.b() << "]";
                }
                if (!texture_asset_relpath.empty()) {
                    ss << ",\"asset\":\"" << json_escape(texture_asset_relpath) << "\"";
                }
                ss << "}";
            }
        }
        ss << "]";

        ss << "}";
    }
    ss << "],";

    ss << "\"objects\":[";
    first = true;
    for (auto it = scene.m_objects.begin(); it != scene.m_objects.end(); ++it) {
        const char *obj_name_c = xtcore::pool::str::get((*it).first);
        const std::string obj_name = obj_name_c ? std::string(obj_name_c) : std::string();
        if (obj_name.empty()) continue;
        const xtcore::asset::Object *obj = (*it).second;
        if (!obj) continue;
        const char *surface_name_c = xtcore::pool::str::get(obj->surface);
        const std::string surface_name = surface_name_c ? std::string(surface_name_c) : std::string();
        const char *material_name_c = xtcore::pool::str::get(obj->material);
        const std::string material_name = material_name_c ? std::string(material_name_c) : std::string();
        const xtcore::asset::medium::IMedium *medium = scene.get_object_medium((*it).first);
        const std::string medium_name = medium ? obj_name : std::string();
        if (!first) ss << ",";
        first = false;
        ss << "{"
           << "\"id\":\"" << json_escape(obj_name) << "\","
           << "\"surface\":\"" << json_escape(surface_name) << "\","
           << "\"material\":\"" << json_escape(material_name) << "\","
           << "\"medium\":\"" << json_escape(medium_name) << "\""
           << "}";
    }
    ss << "],";

    ss << "\"media\":[";
    first = true;
    for (auto it = scene.m_objects.begin(); it != scene.m_objects.end(); ++it) {
        const xtcore::asset::Object *obj = (*it).second;
        if (!obj) continue;
        const char *obj_name_c = xtcore::pool::str::get((*it).first);
        const std::string obj_name = obj_name_c ? std::string(obj_name_c) : std::string();
        if (obj_name.empty()) continue;
        const xtcore::asset::medium::IMedium *medium = scene.get_object_medium((*it).first);
        if (!medium) continue;
        if (!first) ss << ",";
        first = false;
        const nimg::ColorRGBf sigma_a = medium->sigma_a();
        const nimg::ColorRGBf sigma_s = medium->sigma_s();
        const nimg::ColorRGBf emission = medium->emission();
        ss << "{"
           << "\"id\":\"" << json_escape(obj_name) << "\","
           << "\"type\":\"" << medium_type_name(medium) << "\","
           << "\"sigma_a\":[" << sigma_a.r() << "," << sigma_a.g() << "," << sigma_a.b() << "],"
           << "\"sigma_s\":[" << sigma_s.r() << "," << sigma_s.g() << "," << sigma_s.b() << "],"
           << "\"emission\":[" << emission.r() << "," << emission.g() << "," << emission.b() << "],"
           << "\"g\":" << medium->asymmetry()
           << "}";
    }
    ss << "]";

    ss << "}";
    return ss.str();
}

struct scene_cache_entry_t
{
    std::string cache_key;
    std::string scene_path;
    std::string variant;
    std::uint64_t mtime;
    std::shared_ptr<xtcore::Scene> scene;
    camera_list_info_t cameras;
    std::string geometry_json;
    std::string runtime_graph_json;
};

std::string scene_cache_key(const std::string &scene_path, const std::string &variant)
{
    return scene_path + "\n" + variant;
}

bool scene_cache_key_matches_path(const std::string &key, const std::string &scene_path)
{
    const std::string prefix = scene_path + "\n";
    return key.compare(0, prefix.size(), prefix) == 0;
}

struct scene_cache_lookup_t
{
    std::shared_ptr<scene_cache_entry_t> entry;
    bool cache_hit;
    bool loading;
    unsigned long long load_job_id;
    std::string error;

    scene_cache_lookup_t()
        : entry()
        , cache_hit(false)
        , loading(false)
        , load_job_id(0ULL)
        , error()
    {}
};

struct scene_pending_load_t
{
    std::uint64_t mtime;
    unsigned long long job_id;
    long long completed_ms;

    scene_pending_load_t()
        : mtime(0ULL)
        , job_id(0ULL)
        , completed_ms(0LL)
    {}
};

class scene_cache_t
{
    public:
    static scene_cache_t &handle()
    {
        static scene_cache_t c;
        return c;
    }

    scene_cache_lookup_t get_or_load(const std::string &scene_path, const std::string &variant)
    {
        scene_cache_lookup_t out;
        const std::string key = scene_cache_key(scene_path, variant);
        const long long now = now_ms();

        std::uint64_t mtime = 0ULL;
        if (!file_mtime(scene_path, mtime)) {
            out.error = "scene file not found";
            return out;
        }

        unsigned long long pending_job_id = 0ULL;
        std::vector<unsigned long long> expired_jobs;
        {
            std::lock_guard<std::mutex> lock(mut);
            prune_completed_pending_locked(now, expired_jobs);
            auto it = entries.find(key);
            if (it != entries.end() && it->second && it->second->mtime == mtime) {
                out.cache_hit = true;
                touch_entry_locked(key);
                backend_log_t::handle().add("debug",
                                            "scene cache hit path=" + scene_path + " variant=" + variant);
                out.entry = it->second;
            } else {
                auto pit = pending.find(key);
                if (pit != pending.end()) {
                    if (pit->second.mtime == mtime) {
                        pending_job_id = pit->second.job_id;
                    } else {
                        xtcore::io::scn::load_async_discard(pit->second.job_id);
                        pending.erase(pit);
                    }
                }
            }
        }
        discard_pending_jobs(expired_jobs);
        if (out.entry) return out;

        if (!pending_job_id) {
            const char *variant_name = variant.empty() ? nullptr : variant.c_str();
            pending_job_id = xtcore::io::scn::load_async_start(scene_path.c_str(), nullptr, variant_name);
            if (!pending_job_id) {
                out.error = "failed to start async scene load";
                return out;
            }
            {
                std::lock_guard<std::mutex> lock(mut);
                scene_pending_load_t p;
                p.mtime = mtime;
                p.job_id = pending_job_id;
                p.completed_ms = 0LL;
                pending[key] = p;
            }
            std::ostringstream log;
            log << "scene async load started path=" << scene_path
                << " variant=" << variant
                << " job_id=" << pending_job_id;
            backend_log_t::handle().add("debug", log.str());
            out.loading = true;
            out.load_job_id = pending_job_id;
            return out;
        }

        xtcore::io::scn::async_load_snapshot_t snapshot;
        if (!xtcore::io::scn::load_async_snapshot(pending_job_id, &snapshot)) {
            std::lock_guard<std::mutex> lock(mut);
            auto pit = pending.find(key);
            if (pit != pending.end() && pit->second.job_id == pending_job_id) pending.erase(pit);
            out.error = "scene load job not found";
            return out;
        }

        if (snapshot.state == xtcore::io::scn::ASYNC_LOAD_QUEUED
            || snapshot.state == xtcore::io::scn::ASYNC_LOAD_RUNNING) {
            out.loading = true;
            out.load_job_id = pending_job_id;
            return out;
        }

        if (snapshot.state == xtcore::io::scn::ASYNC_LOAD_ERROR) {
            xtcore::io::scn::load_async_discard(pending_job_id);
            std::lock_guard<std::mutex> lock(mut);
            auto pit = pending.find(key);
            if (pit != pending.end() && pit->second.job_id == pending_job_id) pending.erase(pit);
            out.error = snapshot.error.empty() ? "failed to load scene" : snapshot.error;
            return out;
        }

        {
            std::lock_guard<std::mutex> lock(mut);
            mark_pending_completed_locked(key, pending_job_id, now);
        }
        std::shared_ptr<xtcore::Scene> loaded_scene = xtcore::io::scn::load_async_take_scene(pending_job_id);
        if (!loaded_scene) {
            out.loading = true;
            out.load_job_id = pending_job_id;
            return out;
        }

        std::shared_ptr<scene_cache_entry_t> entry(new scene_cache_entry_t());
        entry->cache_key = key;
        entry->scene_path = scene_path;
        entry->variant = variant;
        entry->mtime = mtime;
        entry->scene = loaded_scene;

        auto t_pack_0 = std::chrono::steady_clock::now();
        entry->cameras = list_cameras_from_scene(*entry->scene);
        entry->geometry_json = scene_geometry_json_from_scene(*entry->scene);
        entry->runtime_graph_json = scene_runtime_graph_json_from_scene(scene_path, *entry->scene);
        auto t_pack_1 = std::chrono::steady_clock::now();
        const long long pack_ms = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(t_pack_1 - t_pack_0).count();

        {
            std::lock_guard<std::mutex> lock(mut);
            entries[key] = entry;
            touch_entry_locked(key);
            prune_entries_locked();
            auto pit = pending.find(key);
            if (pit != pending.end() && pit->second.job_id == pending_job_id) pending.erase(pit);
        }
        xtcore::io::scn::load_async_discard(pending_job_id);
        xtcore::io::scn::load_async_gc_done(128);

        std::ostringstream log;
        log << "scene cache miss path=" << scene_path
            << " variant=" << variant
            << " async_job_id=" << pending_job_id
            << " pack_ms=" << pack_ms;
        backend_log_t::handle().add("debug", log.str());
        out.entry = entry;
        return out;
    }

    void invalidate(const std::string &scene_path)
    {
        std::lock_guard<std::mutex> lock(mut);
        for (auto pit = pending.begin(); pit != pending.end(); ) {
            if (scene_cache_key_matches_path(pit->first, scene_path)) {
                xtcore::io::scn::load_async_discard(pit->second.job_id);
                pit = pending.erase(pit);
            } else {
                ++pit;
            }
        }
        std::vector<std::string> keys_to_erase;
        for (auto eit = entries.begin(); eit != entries.end(); ++eit) {
            if (scene_cache_key_matches_path(eit->first, scene_path)) {
                keys_to_erase.push_back(eit->first);
            }
        }
        for (size_t i = 0; i < keys_to_erase.size(); ++i) {
            erase_entry_locked(keys_to_erase[i]);
        }
    }

    bool load_job_snapshot(unsigned long long id, xtcore::io::scn::async_load_snapshot_t &snapshot)
    {
        const long long now = now_ms();
        std::vector<unsigned long long> expired_jobs;
        {
            std::lock_guard<std::mutex> lock(mut);
            prune_completed_pending_locked(now, expired_jobs);
        }
        discard_pending_jobs(expired_jobs);

        if (!xtcore::io::scn::load_async_snapshot(id, &snapshot)) {
            std::lock_guard<std::mutex> lock(mut);
            erase_pending_by_job_locked(id);
            return false;
        }

        if (snapshot.state == xtcore::io::scn::ASYNC_LOAD_DONE
            || snapshot.state == xtcore::io::scn::ASYNC_LOAD_ERROR) {
            std::vector<unsigned long long> completed_expired_jobs;
            {
                std::lock_guard<std::mutex> lock(mut);
                mark_pending_completed_by_job_locked(id, now);
                prune_completed_pending_locked(now, completed_expired_jobs);
            }
            discard_pending_jobs(completed_expired_jobs);
        }
        return true;
    }

    private:
    scene_cache_t()
        : mut()
        , entries()
        , pending()
        , lru_order()
        , lru_index()
        , max_entries(8)
    {}
    scene_cache_t(const scene_cache_t &) = delete;
    scene_cache_t &operator=(const scene_cache_t &) = delete;

    long long now_ms() const
    {
        const auto now = std::chrono::system_clock::now();
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        return (long long)ms.count();
    }

    void touch_entry_locked(const std::string &key)
    {
        auto it = lru_index.find(key);
        if (it != lru_index.end()) {
            lru_order.erase(it->second);
        }
        lru_order.push_back(key);
        std::list<std::string>::iterator last = lru_order.end();
        --last;
        lru_index[key] = last;
    }

    void erase_entry_locked(const std::string &key)
    {
        auto it = lru_index.find(key);
        if (it != lru_index.end()) {
            lru_order.erase(it->second);
            lru_index.erase(it);
        }
        entries.erase(key);
    }

    void erase_pending_by_job_locked(unsigned long long job_id)
    {
        if (!job_id) return;
        for (auto pit = pending.begin(); pit != pending.end(); ++pit) {
            if (pit->second.job_id == job_id) {
                pending.erase(pit);
                return;
            }
        }
    }

    void mark_pending_completed_locked(const std::string &key,
                                       unsigned long long job_id,
                                       long long completed_ms)
    {
        if (!job_id) return;
        auto pit = pending.find(key);
        if (pit == pending.end()) return;
        if (pit->second.job_id != job_id) return;
        if (pit->second.completed_ms == 0LL) pit->second.completed_ms = completed_ms;
    }

    void mark_pending_completed_by_job_locked(unsigned long long job_id, long long completed_ms)
    {
        if (!job_id) return;
        for (auto pit = pending.begin(); pit != pending.end(); ++pit) {
            if (pit->second.job_id != job_id) continue;
            if (pit->second.completed_ms == 0LL) pit->second.completed_ms = completed_ms;
            return;
        }
    }

    void prune_completed_pending_locked(long long now, std::vector<unsigned long long> &jobs_to_discard)
    {
        static const long long k_completed_pending_ttl_ms = 30LL * 1000LL;
        for (auto pit = pending.begin(); pit != pending.end();) {
            if (pit->second.completed_ms > 0LL
                && (now - pit->second.completed_ms) > k_completed_pending_ttl_ms) {
                if (pit->second.job_id) jobs_to_discard.push_back(pit->second.job_id);
                pit = pending.erase(pit);
                continue;
            }
            ++pit;
        }
    }

    void discard_pending_jobs(const std::vector<unsigned long long> &jobs)
    {
        if (jobs.empty()) return;
        for (size_t i = 0; i < jobs.size(); ++i) {
            if (!jobs[i]) continue;
            xtcore::io::scn::load_async_discard(jobs[i]);
        }
        xtcore::io::scn::load_async_gc_done(128);
    }

    void prune_entries_locked()
    {
        while (max_entries > 0 && entries.size() > max_entries && !lru_order.empty()) {
            const std::string victim = lru_order.front();
            lru_order.pop_front();
            lru_index.erase(victim);
            entries.erase(victim);
        }
    }

    std::mutex mut;
    std::unordered_map<std::string, std::shared_ptr<scene_cache_entry_t> > entries;
    std::unordered_map<std::string, scene_pending_load_t> pending;
    std::list<std::string> lru_order;
    std::unordered_map<std::string, std::list<std::string>::iterator> lru_index;
    size_t max_entries;
};

camera_list_info_t list_cameras(const std::string &scene_path,
                                const std::string &variant,
                                std::string &error,
                                bool &loading,
                                unsigned long long &job_id)
{
    scene_cache_lookup_t lookup = scene_cache_t::handle().get_or_load(scene_path, variant);
    error = lookup.error;
    loading = lookup.loading;
    job_id = lookup.load_job_id;
    std::shared_ptr<scene_cache_entry_t> entry = lookup.entry;
    if (!entry) return camera_list_info_t();
    return entry->cameras;
}

std::string scene_geometry_json(const std::string &scene_path,
                                const std::string &variant,
                                std::string &error,
                                bool &loading,
                                unsigned long long &job_id)
{
    scene_cache_lookup_t lookup = scene_cache_t::handle().get_or_load(scene_path, variant);
    error = lookup.error;
    loading = lookup.loading;
    job_id = lookup.load_job_id;
    std::shared_ptr<scene_cache_entry_t> entry = lookup.entry;
    if (!entry) return std::string();
    return entry->geometry_json;
}

std::string scene_resolved_camera_json(const std::string &scene_path,
                                       const std::string &variant,
                                       const std::string &requested_camera,
                                       std::string &error,
                                       bool &loading,
                                       unsigned long long &job_id)
{
    scene_cache_lookup_t lookup = scene_cache_t::handle().get_or_load(scene_path, variant);
    error = lookup.error;
    loading = lookup.loading;
    job_id = lookup.load_job_id;
    std::shared_ptr<scene_cache_entry_t> entry = lookup.entry;
    if (!entry) return std::string();
    if (!entry->scene) {
        error = "scene not available";
        return std::string();
    }
    xtcore::Scene &scene = *(entry->scene);

    HASH_ID resolved_id = HASH_ID_INVALID;
    if (!requested_camera.empty()) {
        resolved_id = find_camera_id_by_name(scene, requested_camera);
    }
    if (resolved_id == HASH_ID_INVALID) {
        resolved_id = find_camera_id_by_name(scene, scene.m_default_camera);
        if (resolved_id == HASH_ID_INVALID) {
            auto first_cam = scene.m_cameras.begin();
            if (first_cam != scene.m_cameras.end()) resolved_id = (*first_cam).first;
        }
    }

    if (resolved_id == HASH_ID_INVALID || !scene.get_camera(resolved_id)) {
        error = "no valid camera found";
        return std::string();
    }

    const char *resolved_name = xtcore::pool::str::get(resolved_id);
    xtcore::asset::ICamera *cam = scene.get_camera(resolved_id);
    xtcore::camera::Perspective *pcam = dynamic_cast<xtcore::camera::Perspective *>(cam);

    std::ostringstream ss;
    ss << "{";
    ss << "\"resolved\":\"" << json_escape(resolved_name ? resolved_name : "") << "\",";
    ss << "\"type\":\"" << json_escape(cam ? cam->get_type() : "") << "\"";
    if (pcam) {
        nmath::Vector3f rz = (pcam->target - pcam->position).normalized();
        nmath::Vector3f rx = cross(pcam->up, rz).normalized();
        nmath::Vector3f ry = cross(rx, rz).normalized();

        ss << ",\"position\":[" << pcam->position.x << "," << pcam->position.y << "," << pcam->position.z << "]";
        ss << ",\"target\":[" << pcam->target.x << "," << pcam->target.y << "," << pcam->target.z << "]";
        ss << ",\"up\":[" << pcam->up.x << "," << pcam->up.y << "," << pcam->up.z << "]";
        ss << ",\"hfov\":" << pcam->fov;
        ss << ",\"basis\":{";
        ss << "\"rx\":[" << rx.x << "," << rx.y << "," << rx.z << "],";
        ss << "\"ry\":[" << ry.x << "," << ry.y << "," << ry.z << "],";
        ss << "\"rz\":[" << rz.x << "," << rz.y << "," << rz.z << "]";
        ss << "}";
    }
    ss << "}";
    return ss.str();
}

std::string scene_runtime_graph_json(const std::string &scene_path,
                                     const std::string &variant,
                                     std::string &error,
                                     bool &loading,
                                     unsigned long long &job_id)
{
    scene_cache_lookup_t lookup = scene_cache_t::handle().get_or_load(scene_path, variant);
    error = lookup.error;
    loading = lookup.loading;
    job_id = lookup.load_job_id;
    std::shared_ptr<scene_cache_entry_t> entry = lookup.entry;
    if (!entry) return std::string();
    return entry->runtime_graph_json;
}

bool encode_texture_sampler_png(const xtcore::sampler::Texture2D *tex, std::vector<unsigned char> &out)
{
    if (!tex) return false;
    if (tex->width() == 0 || tex->height() == 0) return false;

    nimg::Pixmap pixmap;
    if (pixmap.init(tex->width(), tex->height()) != 0) return false;

    for (size_t y = 0; y < tex->height(); ++y) {
        for (size_t x = 0; x < tex->width(); ++x) {
            pixmap.pixel(x, y) = tex->pixel_ro(x, y);
        }
    }

    return nimg::io::save::png_memory(pixmap, out) == 0;
}

bool scene_runtime_texture_png(const std::string &scene_path,
                               const std::string &variant,
                               const std::string &material_name,
                               const std::string &sampler_name,
                               std::vector<unsigned char> &png,
                               std::string &error,
                               bool &loading,
                               unsigned long long &job_id)
{
    scene_cache_lookup_t lookup = scene_cache_t::handle().get_or_load(scene_path, variant);
    error = lookup.error;
    loading = lookup.loading;
    job_id = lookup.load_job_id;
    std::shared_ptr<scene_cache_entry_t> entry = lookup.entry;
    if (!entry || !entry->scene) {
        if (error.empty() && !loading) error = "scene not loaded";
        return false;
    }

    pooled_hash_guard_t material_id_guard;
    material_id_guard.reset(xtcore::pool::str::add(material_name.c_str()));
    const HASH_UINT64 material_id = material_id_guard.value;
    auto it = entry->scene->m_materials.find(material_id);
    if (it == entry->scene->m_materials.end() || !it->second) {
        error = "material not found";
        return false;
    }

    xtcore::asset::IMaterial *mat = it->second;
    xtcore::sampler::ISampler *sampler = 0;
    for (size_t i = 0; i < mat->get_sampler_count(); ++i) {
        std::string name;
        xtcore::sampler::ISampler *candidate = mat->get_sampler_by_index(i, &name);
        if (name == sampler_name) {
            sampler = candidate;
            break;
        }
    }

    xtcore::sampler::Texture2D *tex = dynamic_cast<xtcore::sampler::Texture2D *>(sampler);
    if (!tex) {
        error = "texture sampler not found";
        return false;
    }

    if (!encode_texture_sampler_png(tex, png)) {
        error = "failed to encode texture preview";
        return false;
    }

    return true;
}

void send_scene_loading(crow::response &res, unsigned long long load_job_id)
{
    std::ostringstream ss;
    ss << "{"
       << "\"state\":\"loading\","
       << "\"job_id\":" << load_job_id
       << "}";
    send_json(res, ss.str(), 202);
}

const char *job_state_name(job_state_t state)
{
    switch (state) {
        case JOB_QUEUED:  return "queued";
        case JOB_RUNNING: return "running";
        case JOB_DONE:    return "done";
        case JOB_ABORTED: return "aborted";
        case JOB_ERROR:   return "error";
    }
    return "unknown";
}

void serve_static_file(const std::string &path, const char *mime, crow::response &res)
{
    std::vector<char> content;
    if (!read_binary_file(path, content)) {
        res.code = 404;
        res.end();
        return;
    }
    res.body = std::string(content.data(), content.size());
    res.set_header("Content-Type", mime);
    res.set_header("Cache-Control", "no-store");
    res.end();
}

std::string job_snapshot_to_json(const job_snapshot_t &snap)
{
    std::ostringstream ss;
    ss << "{"
       << "\"id\":\"" << json_escape(snap.id) << "\","
       << "\"workspace_id\":\"" << json_escape(snap.workspace_id) << "\","
       << "\"scene\":\"" << json_escape(snap.scene) << "\","
       << "\"integrator\":\"" << json_escape(snap.integrator) << "\","
       << "\"render_mode\":\"" << json_escape(snap.render_mode) << "\","
       << "\"state\":\"" << job_state_name(snap.state) << "\","
       << "\"threads\":" << snap.threads << ","
       << "\"progress\":" << snap.progress << ","
       << "\"tiles_done\":" << snap.tiles_done << ","
       << "\"tiles_total\":" << snap.tiles_total << ","
       << "\"pass_current\":" << snap.pass_current << ","
       << "\"pass_total\":" << snap.pass_total << ","
       << "\"elapsed_ms\":" << snap.elapsed_ms << ","
       << "\"has_image\":" << (snap.has_image ? "true" : "false") << ","
       << "\"width\":" << snap.width << ","
       << "\"height\":" << snap.height << ","
       << "\"active_tiles\":[";
    for (size_t i = 0; i < snap.active_tiles.size(); ++i) {
        if (i) ss << ",";
        const auto &r = snap.active_tiles[i];
        ss << "[" << r.x0 << "," << r.y0 << "," << r.x1 << "," << r.y1 << "]";
    }
    ss << "],"
       << "\"error\":\"" << json_escape(snap.error) << "\""
       << "}";
    return ss.str();
}

} // namespace

static job_ws_hub_t g_job_ws_hub;
static log_ws_hub_t g_log_ws_hub;
static log_ws_hub_t g_job_events_ws_hub;

// Build and broadcast {"type":"jobs_changed","jobs":[...]} to all /ws/jobs
// subscribers.  Called after every mutation that changes the active job list.
static void broadcast_jobs_changed(job_manager_t &jobs)
{
    std::vector<job_snapshot_t> active;
    jobs.list_active(active);
    std::ostringstream ss;
    ss << "{\"type\":\"jobs_changed\",\"jobs\":[";
    for (size_t i = 0; i < active.size(); ++i) {
        if (i) ss << ",";
        const job_snapshot_t &snap = active[i];
        ss << "{"
           << "\"id\":\"" << json_escape(snap.id) << "\","
           << "\"workspace_id\":\"" << json_escape(snap.workspace_id) << "\","
           << "\"scene\":\"" << json_escape(snap.scene) << "\","
           << "\"integrator\":\"" << json_escape(snap.integrator) << "\","
           << "\"render_mode\":\"" << json_escape(snap.render_mode) << "\","
           << "\"state\":\"" << job_state_name(snap.state) << "\","
           << "\"threads\":" << snap.threads << ","
           << "\"progress\":" << snap.progress << ","
           << "\"tiles_done\":" << snap.tiles_done << ","
           << "\"tiles_total\":" << snap.tiles_total << ","
           << "\"pass_current\":" << snap.pass_current << ","
           << "\"pass_total\":" << snap.pass_total << ","
           << "\"elapsed_ms\":" << snap.elapsed_ms << ","
           << "\"queue_index\":" << snap.queue_index
           << "}";
    }
    ss << "]}";
    g_job_events_ws_hub.broadcast_text(ss.str());
}

void setup_routes(WebApp &app,
                  job_manager_t &jobs,
                  workspace_manager_t &workspaces,
                  gallery_manager_t *gallery,
                  const std::string &scene_dir,
                  const std::string &web_root,
                  const render_thread_policy_t &thread_policy)
{
    backend_log_t::handle().add("info", "web routes initialized");

    CROW_ROUTE(app, "/api/health")
    ([](const crow::request &, crow::response &res) {
        send_json(res, "{\"ok\":true}");
    });

    CROW_ROUTE(app, "/api/sampling/samples")
    ([](const crow::request &req, crow::response &res) {
        const params_view params(req);
        const std::string method = params.str("method", "uniform_sphere");

        size_t count = 5000;
        parse_u64_param(params, "count", 1, 100000, count);

        double param   = 0.3;
        double param_x = 0.3;
        double param_y = 0.3;
        parse_f64_param(params, "param",   0.0, 1024.0, param);
        parse_f64_param(params, "param_x", 0.0, 1024.0, param_x);
        parse_f64_param(params, "param_y", 0.0, 1024.0, param_y);

        const nmath::Vector3f up(0.0f, 1.0f, 0.0f);

        std::ostringstream ss;
        ss << "{\"method\":\"" << json_escape(method) << "\",\"samples\":[";

        for (size_t i = 0; i < count; ++i) {
            nmath::Vector3f s;
            nmath::scalar_t pdf = 0.0;

            if (method == "uniform_sphere") {
                s = xtcore::math::sampling::sample_uniform_sphere(pdf);
            } else if (method == "cosine_hemisphere") {
                s = xtcore::math::sampling::sample_cosine_hemisphere(up, pdf);
            } else if (method == "power_cosine_lobe") {
                const nmath::scalar_t exp = (nmath::scalar_t)(param > 0.0 ? param : 16.0);
                s = xtcore::math::sampling::sample_power_cosine_lobe(up, exp, pdf);
            } else if (method == "ggx_half_vector") {
                const nmath::scalar_t roughness = (nmath::scalar_t)(param > 0.0 ? param : 0.3);
                s = xtcore::math::sampling::sample_ggx_half_vector(up, roughness, pdf);
            } else if (method == "ggx_half_vector_anisotropic") {
                const nmath::Vector3f tangent(1.0f, 0.0f, 0.0f);
                const nmath::Vector3f bitangent(0.0f, 0.0f, 1.0f);
                const nmath::scalar_t ax = (nmath::scalar_t)(param_x > 0.0 ? param_x : 0.3);
                const nmath::scalar_t ay = (nmath::scalar_t)(param_y > 0.0 ? param_y : 0.3);
                s = xtcore::math::sampling::sample_ggx_half_vector_anisotropic(up, tangent, bitangent, ax, ay, pdf);
            } else if (method == "uniform_cone") {
                const nmath::scalar_t cos_max = (nmath::scalar_t)(param > 0.0 ? param : 0.866);
                s = xtcore::math::sampling::sample_uniform_cone(up, cos_max, pdf);
            } else if (method == "nmath_sphere") {
                s = nmath::sample::sphere();
            } else if (method == "nmath_hemisphere") {
                s = nmath::sample::hemisphere(up, up);
            } else if (method == "nmath_diffuse") {
                s = nmath::sample::diffuse(up);
            } else if (method == "nmath_lobe") {
                const nmath::scalar_t exp = (nmath::scalar_t)(param > 0.0 ? param : 16.0);
                s = nmath::sample::lobe(up, up, exp);
            } else {
                send_json(res, "{\"error\":\"unknown method\"}", 400);
                return;
            }

            if (i) ss << ',';
            ss << "[" << s.x << "," << s.y << "," << s.z << "]";
        }

        ss << "]}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/tests/furnace/<string>/<string>")
    ([](const crow::request &, crow::response &res, std::string group, std::string integrator) {
        send_json(res, run_furnace_group(group, integrator));
    });

    CROW_ROUTE(app, "/api/about")
    ([&jobs, &web_root, thread_policy](const crow::request &, crow::response &res) {
        std::time_t now = std::time(nullptr);
        std::tm *utc = std::gmtime(&now);
        int year = utc ? (utc->tm_year + 1900) : 2010;
        if (year < 2010) year = 2010;
        const size_t logical_cores = runtime_logical_cores();
        const size_t openmp_max_threads = runtime_omp_max_threads();
        const size_t reserve_threads = thread_policy.reserve_threads;
        const size_t auto_render_threads = (thread_policy.max_render_threads > 0)
            ? thread_policy.max_render_threads
            : compute_auto_render_threads(reserve_threads);
        const size_t max_concurrent_renders = jobs.get_max_concurrent_renders();
        const size_t active_renders = jobs.get_active_render_count();

        std::ostringstream ss;
        ss << "{"
           << "\"name\":\"XTRACER WEB\","
           << "\"version\":\"" << json_escape(xtcore::get_version()) << "\","
           << "\"author_name\":\"Nikolaos Papadopoulos\","
           << "\"author_email\":\"nikpapas@gmail.com\","
           << "\"homepage\":\"https://www.4rknova.com\","
           << "\"website\":\"https://github.com/4rknova/xtracer\","
           << "\"copyright\":\"Copyright 2010-" << year << " (c) Nikolaos Papadopoulos\","
           << "\"license\":\"" << json_escape(xtcore::get_license()) << "\","
           << "\"max_concurrent_renders\":" << max_concurrent_renders << ","
           << "\"active_renders\":" << active_renders << ","
           << "\"logical_cores\":" << logical_cores << ","
           << "\"openmp_max_threads\":" << openmp_max_threads << ","
           << "\"render_reserve_threads\":" << reserve_threads << ","
           << "\"render_auto_threads\":" << auto_render_threads << ","
           << "\"third_party_licenses\":";
        append_third_party_licenses_json(ss, web_root);
        ss << "}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/workspaces")
    ([&](const crow::request &req, crow::response &res) {
        const params_view params(req);
        const std::string client_id = read_client_id(params);
        std::vector<workspace_snapshot_t> list;
        std::string active_workspace;
        workspaces.list(client_id, list, active_workspace);
        const xtcore::spatial_index_stats_t spatial_stats = xtcore::get_last_spatial_index_stats();

        std::ostringstream ss;
        ss << "{"
           << "\"active_workspace\":\"" << json_escape(active_workspace) << "\","
           << "\"spatial_index\":{"
           << "\"total_objects\":" << spatial_stats.total_objects << ","
           << "\"finite_objects\":" << spatial_stats.finite_objects << ","
           << "\"infinite_objects\":" << spatial_stats.infinite_objects << ","
           << "\"tlas_nodes\":" << spatial_stats.tlas_nodes << ","
           << "\"tlas_leaves\":" << spatial_stats.tlas_leaves << ","
           << "\"build_ms\":" << spatial_stats.build_ms << ","
           << "\"build_count\":" << spatial_stats.build_count
           << "},"
           << "\"workspaces\":[";
        for (size_t i = 0; i < list.size(); ++i) {
            const workspace_snapshot_t &w = list[i];
            const bool is_active_for_client = !active_workspace.empty() && (w.id == active_workspace);
            if (i) ss << ",";
            ss << "{"
               << "\"id\":\"" << json_escape(w.id) << "\","
               << "\"name\":\"" << json_escape(w.name) << "\","
               << "\"is_owned_by_client\":" << (w.is_owned_by_client ? "true" : "false") << ","
               << "\"is_active_for_client\":" << (is_active_for_client ? "true" : "false") << ","
               << "\"active_scene\":\"" << json_escape(w.active_scene) << "\","
               << "\"active_job_id\":\"" << json_escape(w.active_job_id) << "\","
               << "\"last_job_id\":\"" << json_escape(w.last_job_id) << "\","
               << "\"draft_count\":" << w.draft_count << ","
               << "\"client_count\":" << w.client_count << ","
               << "\"quality_samples\":" << w.quality_samples << ","
               << "\"quality_aa\":" << w.quality_aa << ","
               << "\"quality_sample_distribution\":\"" << json_escape(w.quality_sample_distribution) << "\","
               << "\"quality_rdepth\":" << w.quality_rdepth << ","
               << "\"settings_json\":\"" << json_escape(w.settings_json) << "\","
               << "\"updated_ms\":" << w.updated_ms
               << "}";
        }
        ss << "]}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/workspaces").methods(crow::HTTPMethod::Post)
    ([&](const crow::request &req, crow::response &res) {
        const params_view params(req);
        const std::string client_id = read_client_id(params);
        const std::string name = params.str("name");
        const std::string workspace_id = workspaces.create(name, client_id);
        if (workspace_id.empty()) {
            backend_log_t::handle().add("warn", "workspace create rejected: retained workspace limit reached");
            send_json(res, "{\"error\":\"workspace limit reached\"}", 409);
            return;
        }
        if (!client_id.empty()) {
            workspaces.set_active(client_id, workspace_id);
        }

        std::ostringstream ss;
        ss << "{"
           << "\"id\":\"" << json_escape(workspace_id) << "\","
           << "\"name\":\"" << json_escape(name) << "\""
           << "}";
        send_json(res, ss.str(), 201);
    });

    CROW_ROUTE(app, "/api/workspaces/active").methods(crow::HTTPMethod::Post)
    ([&](const crow::request &req, crow::response &res) {
        const params_view params(req);
        const std::string client_id = read_client_id(params);
        if (client_id.empty()) {
            send_json(res, "{\"error\":\"client_id is required\"}", 400);
            return;
        }
        if (!params.has("workspace_id")) {
            send_json(res, "{\"error\":\"workspace_id is required\"}", 400);
            return;
        }
        const std::string workspace_id = params.str("workspace_id");
        if (!workspaces.set_active(client_id, workspace_id)) {
            send_json(res, "{\"error\":\"workspace not found\"}", 404);
            return;
        }
        send_json(res, "{\"ok\":true}");
    });

    CROW_ROUTE(app, "/api/workspaces/delete").methods(crow::HTTPMethod::Post)
    ([&](const crow::request &req, crow::response &res) {
        const params_view params(req);
        const std::string client_id = read_client_id(params);
        if (client_id.empty()) {
            send_json(res, "{\"error\":\"client_id is required\"}", 400);
            return;
        }
        if (!params.has("workspace_id")) {
            send_json(res, "{\"error\":\"workspace_id is required\"}", 400);
            return;
        }

        const std::string workspace_id = params.str("workspace_id");
        std::string replacement_workspace_id;
        const workspace_manager_t::remove_result_t rc = workspaces.remove(workspace_id, replacement_workspace_id);
        if (rc == workspace_manager_t::REMOVE_NOT_FOUND) {
            send_json(res, "{\"error\":\"workspace not found\"}", 404);
            return;
        }
        if (rc == workspace_manager_t::REMOVE_LAST_WORKSPACE) {
            send_json(res, "{\"error\":\"cannot delete last workspace\"}", 409);
            return;
        }

        std::string active_workspace;
        workspaces.get_active(client_id, active_workspace);
        std::ostringstream ss;
        ss << "{"
           << "\"ok\":true,"
           << "\"replacement_workspace\":\"" << json_escape(replacement_workspace_id) << "\","
           << "\"active_workspace\":\"" << json_escape(active_workspace) << "\""
           << "}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/workspaces/scene_draft").methods(crow::HTTPMethod::Post)
    ([&](const crow::request &req, crow::response &res) {
        const params_view params(req);
        const std::string client_id = read_client_id(params);
        if (client_id.empty()) {
            send_json(res, "{\"error\":\"client_id is required\"}", 400);
            return;
        }
        if (!params.has("scene")) {
            send_json(res, "{\"error\":\"scene is required\"}", 400);
            return;
        }
        if (!params.has("source")) {
            send_json(res, "{\"error\":\"source is required\"}", 400);
            return;
        }
        std::string workspace_id;
        workspaces.get_active(client_id, workspace_id);
        const std::string scene = params.str("scene");
        if (!is_scene_name_safe(scene)) {
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }
        const workspace_manager_t::store_result_t rc =
            workspaces.set_scene_draft(workspace_id, scene, params.str("source"));
        if (rc == workspace_manager_t::STORE_TOO_LARGE) {
            send_json(res, "{\"error\":\"scene draft too large\"}", 413);
            return;
        }
        if (rc != workspace_manager_t::STORE_OK) {
            send_json(res, "{\"error\":\"workspace not found\"}", 404);
            return;
        }
        send_json(res, "{\"ok\":true}");
    });

    CROW_ROUTE(app, "/api/workspaces/settings").methods(crow::HTTPMethod::Post)
    ([&](const crow::request &req, crow::response &res) {
        const params_view params(req);
        const std::string client_id = read_client_id(params);
        if (client_id.empty()) {
            send_json(res, "{\"error\":\"client_id is required\"}", 400);
            return;
        }
        if (!params.has("settings_json")) {
            send_json(res, "{\"error\":\"settings_json is required\"}", 400);
            return;
        }

        std::string workspace_id;
        workspaces.get_active(client_id, workspace_id);
        const std::string settings_json = params.str("settings_json");
        const workspace_manager_t::store_result_t rc =
            workspaces.set_settings_json(workspace_id, settings_json);
        if (rc == workspace_manager_t::STORE_TOO_LARGE) {
            send_json(res, "{\"error\":\"settings_json too large\"}", 413);
            return;
        }
        if (rc != workspace_manager_t::STORE_OK) {
            send_json(res, "{\"error\":\"workspace not found\"}", 404);
            return;
        }
        send_json(res, "{\"ok\":true}");
    });

    CROW_ROUTE(app, "/api/scenes")
    ([scene_dir](const crow::request &, crow::response &res) {
        std::vector<std::string> scenes = list_scenes(scene_dir);
        std::ostringstream ss;
        ss << "{\"scenes\":[";
        for (size_t i = 0; i < scenes.size(); ++i) {
            if (i) ss << ',';
            ss << '"' << json_escape(scenes[i]) << '"';
        }
        ss << "]}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/logs")
    ([](const crow::request &req, crow::response &res) {
        const params_view params(req);
        unsigned long long since = 0;
        if (params.has("since")) {
            std::istringstream ss(params.str("since"));
            ss >> since;
            if (ss.fail()) {
                send_json(res, "{\"error\":\"invalid since\"}", 400);
                return;
            }
        }

        std::vector<backend_log_entry_t> list = backend_log_t::handle().since(since);
        send_json(res, backend_logs_to_json(list));
    });

    CROW_ROUTE(app, "/api/scenes/load_jobs/<string>")
    ([](const crow::request &, crow::response &res, std::string id_str) {
        unsigned long long id = 0ULL;
        std::istringstream ss(id_str);
        ss >> id;
        if (ss.fail() || !id) {
            send_json(res, "{\"error\":\"invalid job id\"}", 400);
            return;
        }

        xtcore::io::scn::async_load_snapshot_t snap;
        if (!scene_cache_t::handle().load_job_snapshot(id, snap)) {
            send_json(res, "{\"error\":\"job not found\"}", 404);
            return;
        }

        std::ostringstream out;
        out << "{"
            << "\"job_id\":" << snap.id << ","
            << "\"state\":\"" << json_escape(snap.state_name) << "\","
            << "\"filename\":\"" << json_escape(snap.filename) << "\","
            << "\"error\":\"" << json_escape(snap.error) << "\""
            << "}";
        send_json(res, out.str());
    });

    CROW_ROUTE(app, "/api/scenes/<string>/cameras")
    ([scene_dir](const crow::request &req, crow::response &res, std::string scene) {
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "camera list rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }
        const params_view params(req);
        std::string variant;
        std::string variant_error;
        if (!read_variant_name(params, variant, variant_error)) {
            send_json(res, "{\"error\":\"invalid variant\"}", 400);
            return;
        }

        std::string error;
        bool loading = false;
        unsigned long long load_job_id = 0ULL;
        camera_list_info_t cameras = list_cameras(join_path(scene_dir, scene), variant, error, loading, load_job_id);
        if (loading) {
            send_scene_loading(res, load_job_id);
            return;
        }
        if (!error.empty()) {
            backend_log_t::handle().add("error", "camera list failed for scene=" + scene);
            send_json(res, "{\"error\":\"failed to load scene\"}", 400);
            return;
        }

        std::ostringstream ss;
        ss << "{\"cameras\":[";
        for (size_t i = 0; i < cameras.cameras.size(); ++i) {
            if (i) ss << ',';
            ss << '"' << json_escape(cameras.cameras[i]) << '"';
        }
        ss << "],\"camera_entries\":[";
        for (size_t i = 0; i < cameras.camera_entries.size(); ++i) {
            if (i) ss << ',';
            ss << "{"
               << "\"name\":\"" << json_escape(cameras.camera_entries[i].name) << "\","
               << "\"type\":\"" << json_escape(cameras.camera_entries[i].type) << "\""
               << "}";
        }
        ss << "],\"default_camera\":\"" << json_escape(cameras.default_camera) << "\"}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/scenes/<string>/source")
    ([scene_dir, &workspaces](const crow::request &req, crow::response &res, std::string scene) {
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "source read rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }
        const params_view params(req);
        std::string source;
        std::string source_origin = "disk";
        const std::string client_id = read_client_id(params);
        if (!client_id.empty()) {
            std::string workspace_id;
            workspaces.get_active(client_id, workspace_id);
            std::string draft;
            if (workspaces.get_scene_draft(workspace_id, scene, draft)) {
                source.swap(draft);
                source_origin = "workspace";
            }
        }
        if (source.empty()) {
            if (!read_text_file(join_path(scene_dir, scene), source)) {
                backend_log_t::handle().add("error", "source read failed scene=" + scene);
                send_json(res, "{\"error\":\"failed to read scene\"}", 404);
                return;
            }
        }

        std::ostringstream ss;
        ss << "{"
           << "\"scene\":\"" << json_escape(scene) << "\","
           << "\"source\":\"" << json_escape(source) << "\","
           << "\"source_origin\":\"" << json_escape(source_origin) << "\""
           << "}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/scenes/<string>/geometry")
    ([scene_dir](const crow::request &req, crow::response &res, std::string scene) {
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "geometry rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }
        const params_view params(req);
        std::string variant;
        std::string variant_error;
        if (!read_variant_name(params, variant, variant_error)) {
            send_json(res, "{\"error\":\"invalid variant\"}", 400);
            return;
        }

        std::string error;
        bool loading = false;
        unsigned long long load_job_id = 0ULL;
        std::string payload = scene_geometry_json(join_path(scene_dir, scene), variant, error, loading, load_job_id);
        if (loading) {
            send_scene_loading(res, load_job_id);
            return;
        }
        if (!error.empty()) {
            backend_log_t::handle().add("error", "geometry export failed for scene=" + scene);
            send_json(res, "{\"error\":\"failed to load scene\"}", 400);
            return;
        }

        send_json(res, payload);
    });

    CROW_ROUTE(app, "/api/scenes/<string>/runtime_graph")
    ([scene_dir](const crow::request &req, crow::response &res, std::string scene) {
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "runtime_graph rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }
        const params_view params(req);
        std::string variant;
        std::string variant_error;
        if (!read_variant_name(params, variant, variant_error)) {
            send_json(res, "{\"error\":\"invalid variant\"}", 400);
            return;
        }

        std::string error;
        bool loading = false;
        unsigned long long load_job_id = 0ULL;
        std::string payload = scene_runtime_graph_json(join_path(scene_dir, scene), variant, error, loading, load_job_id);
        if (loading) {
            send_scene_loading(res, load_job_id);
            return;
        }
        if (!error.empty()) {
            backend_log_t::handle().add("error", "runtime_graph failed for scene=" + scene);
            send_json(res, "{\"error\":\"failed to load scene\"}", 400);
            return;
        }

        send_json(res, payload);
    });

    CROW_ROUTE(app, "/api/scenes/<string>/runtime_texture")
    ([scene_dir](const crow::request &req, crow::response &res, std::string scene) {
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "runtime_texture rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }
        const params_view params(req);
        if (!params.has("material") || !params.has("sampler")) {
            send_json(res, "{\"error\":\"material and sampler are required\"}", 400);
            return;
        }

        std::string variant;
        std::string variant_error;
        if (!read_variant_name(params, variant, variant_error)) {
            send_json(res, "{\"error\":\"invalid variant\"}", 400);
            return;
        }

        const std::string material = params.str("material");
        const std::string sampler = params.str("sampler");
        std::vector<unsigned char> png;
        std::string error;
        bool loading = false;
        unsigned long long load_job_id = 0ULL;
        if (!scene_runtime_texture_png(join_path(scene_dir, scene), variant, material, sampler, png, error, loading, load_job_id)) {
            if (loading) {
                send_scene_loading(res, load_job_id);
                return;
            }
            send_json(res, "{\"error\":\"texture preview unavailable\"}", 404);
            return;
        }

        res.set_header("Cache-Control", "no-store");
        res.body = std::string(reinterpret_cast<const char *>(png.data()), png.size());
        res.set_header("Content-Type", "image/png");
        res.end();
    });

    CROW_ROUTE(app, "/api/scenes/<string>/camera_resolve")
    ([scene_dir](const crow::request &req, crow::response &res, std::string scene) {
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "camera_resolve rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }
        const params_view params(req);
        std::string requested_camera;
        if (params.has("camera")) requested_camera = params.str("camera");
        std::string variant;
        std::string variant_error;
        if (!read_variant_name(params, variant, variant_error)) {
            send_json(res, "{\"error\":\"invalid variant\"}", 400);
            return;
        }

        std::string error;
        bool loading = false;
        unsigned long long load_job_id = 0ULL;
        std::string payload = scene_resolved_camera_json(join_path(scene_dir, scene),
                                                         variant,
                                                         requested_camera,
                                                         error,
                                                         loading,
                                                         load_job_id);
        if (loading) {
            send_scene_loading(res, load_job_id);
            return;
        }
        if (!error.empty()) {
            backend_log_t::handle().add("error", "camera_resolve failed for scene=" + scene);
            send_json(res, "{\"error\":\"failed to resolve camera\"}", 400);
            return;
        }
        send_json(res, payload);
    });

    CROW_ROUTE(app, "/api/scenes/<string>/asset")
    ([scene_dir](const crow::request &req, crow::response &res, std::string scene) {
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "asset read rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }
        const params_view params(req);
        if (!params.has("path")) {
            send_json(res, "{\"error\":\"path is required\"}", 400);
            return;
        }

        std::string relpath = params.str("path");
        if (!is_asset_relpath_safe(relpath)) {
            backend_log_t::handle().add("warn", "asset read rejected: invalid path");
            send_json(res, "{\"error\":\"invalid asset path\"}", 400);
            return;
        }

        std::string root_dir = dirname_path(scene_dir);
        if (root_dir.empty()) root_dir = ".";
        std::string full_path = join_path(root_dir, relpath);

        std::vector<char> content;
        if (!read_binary_file(full_path, content)) {
            backend_log_t::handle().add("warn", "asset read failed scene=" + scene + " path=" + relpath);
            send_json(res, "{\"error\":\"asset not found\"}", 404);
            return;
        }

        res.body = std::string(content.data(), content.size());
        res.set_header("Content-Type", "text/plain; charset=utf-8");
        res.end();
    });

    CROW_ROUTE(app, "/api/scenes/template/empty")
    ([](const crow::request &, crow::response &res) {
        std::ostringstream ss;
        ss << "{\"source\":\"" << json_escape(xtcore::get_empty_scene_template()) << "\"}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/scenes/save").methods(crow::HTTPMethod::Post)
    ([scene_dir, &workspaces](const crow::request &req, crow::response &res) {
        const params_view params(req);
        if (!params.has("name")) {
            backend_log_t::handle().add("warn", "scene save rejected: name missing");
            send_json(res, "{\"error\":\"name is required\"}", 400);
            return;
        }
        if (!params.has("source")) {
            backend_log_t::handle().add("warn", "scene save rejected: source missing");
            send_json(res, "{\"error\":\"source is required\"}", 400);
            return;
        }

        std::string input_name = params.str("name");
        if (!is_scene_basename_safe(input_name)) {
            backend_log_t::handle().add("warn", "scene save rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene name\"}", 400);
            return;
        }

        std::string scene_name = normalize_scene_name(input_name);
        if (!is_scene_name_safe(scene_name)) {
            backend_log_t::handle().add("warn", "scene save rejected after normalize: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene name\"}", 400);
            return;
        }

        std::string scene_path = join_path(scene_dir, scene_name);
        bool overwrite = params.has("overwrite") && params.str("overwrite") == "1";
        if (!overwrite && file_exists(scene_path)) {
            backend_log_t::handle().add("warn", "scene save conflict scene=" + scene_name);
            send_json(res, "{\"error\":\"scene already exists\"}", 409);
            return;
        }

        std::string source = params.str("source");
        if (!write_text_file(scene_path, source)) {
            backend_log_t::handle().add("error", "scene save failed scene=" + scene_name);
            send_json(res, "{\"error\":\"failed to write scene\"}", 500);
            return;
        }
        scene_cache_t::handle().invalidate(scene_path);
        const std::string client_id = read_client_id(params);
        if (!client_id.empty()) {
            std::string workspace_id;
            workspaces.get_active(client_id, workspace_id);
            const workspace_manager_t::store_result_t draft_rc =
                workspaces.set_scene_draft(workspace_id, scene_name, source);
            if (draft_rc == workspace_manager_t::STORE_TOO_LARGE) {
                backend_log_t::handle().add("warn", "scene draft cache skipped: source too large scene=" + scene_name);
            }
            workspaces.set_active_scene(workspace_id, scene_name);
        }
        backend_log_t::handle().add("info", "scene saved scene=" + scene_name);

        std::ostringstream ss;
        ss << "{\"scene\":\"" << json_escape(scene_name) << "\"}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/scenes/delete").methods(crow::HTTPMethod::Post)
    ([scene_dir](const crow::request &req, crow::response &res) {
        const params_view params(req);
        if (!params.has("name")) {
            backend_log_t::handle().add("warn", "scene delete rejected: name missing");
            send_json(res, "{\"error\":\"name is required\"}", 400);
            return;
        }

        std::string scene_name = params.str("name");
        if (!is_scene_name_safe(scene_name)) {
            backend_log_t::handle().add("warn", "scene delete rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene name\"}", 400);
            return;
        }

        const std::string scene_path = join_path(scene_dir, scene_name);
        if (!file_exists(scene_path)) {
            backend_log_t::handle().add("warn", "scene delete failed: scene not found scene=" + scene_name);
            send_json(res, "{\"error\":\"scene not found\"}", 404);
            return;
        }

        if (unlink(scene_path.c_str()) != 0) {
            backend_log_t::handle().add("error", "scene delete failed scene=" + scene_name);
            send_json(res, "{\"error\":\"failed to delete scene\"}", 500);
            return;
        }

        scene_cache_t::handle().invalidate(scene_path);
        backend_log_t::handle().add("info", "scene deleted scene=" + scene_name);

        std::ostringstream ss;
        ss << "{\"scene\":\"" << json_escape(scene_name) << "\",\"ok\":true}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/integrators")
    ([](const crow::request &, crow::response &res) {
        std::vector<common::integrator_info_t> list = common::list_integrators();
        std::ostringstream ss;
        ss << "{\"integrators\":[";
        for (size_t i = 0; i < list.size(); ++i) {
            if (i) ss << ',';
            ss << "{\"id\":\"" << json_escape(list[i].metadata.id)
               << "\",\"label\":\"" << json_escape(list[i].metadata.name)
               << "\",\"name\":\"" << json_escape(list[i].metadata.name)
               << "\",\"status\":\"" << json_escape(integrator_status_name(list[i].metadata.status))
               << "\",\"description\":\"" << json_escape(list[i].metadata.description)
               << "\",\"replacement_id\":\"" << json_escape(list[i].metadata.replacement_id)
               << "\",";
            append_integrator_controls_json(ss, list[i]);
            ss << "}";
        }
        ss << "]}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/post_filters")
    ([](const crow::request &, crow::response &res) {
        std::vector<post_filter_info_t> list = list_post_filters();
        std::ostringstream ss;
        ss << "{\"post_filters\":[";
        for (size_t i = 0; i < list.size(); ++i) {
            if (i) ss << ',';
            ss << "{\"id\":\"" << json_escape(list[i].metadata.id)
               << "\",\"label\":\"" << json_escape(list[i].metadata.name)
               << "\",\"name\":\"" << json_escape(list[i].metadata.name)
               << "\",\"status\":\"" << json_escape(filter_status_name(list[i].metadata.status))
               << "\",\"description\":\"" << json_escape(list[i].metadata.description)
               << "\",\"replacement_id\":\"" << json_escape(list[i].metadata.replacement_id)
               << "\",\"allow_before_tm\":" << (list[i].allow_before_tm ? "true" : "false")
               << ",\"allow_after_tm\":" << (list[i].allow_after_tm ? "true" : "false")
               << ",";
            append_post_filter_params_json(ss, list[i]);
            ss << "}";
        }
        ss << "]}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/resolutions")
    ([](const crow::request &, crow::response &res) {
        size_t count = 0;
        const xtcore::render::resolution_preset_t *presets = xtcore::render::resolution_presets(count);
        std::ostringstream ss;
        ss << "{\"presets\":[";
        for (size_t i = 0; i < count; ++i) {
            if (i) ss << ',';
            ss << "{\"id\":" << i
               << ",\"description\":\"" << json_escape(presets[i].description) << "\""
               << ",\"width\":" << presets[i].width
               << ",\"height\":" << presets[i].height << "}";
        }
        ss << "]}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/render").methods(crow::HTTPMethod::Post)
    ([&](const crow::request &req, crow::response &res) {
        const params_view params(req);
        if (!params.has("scene")) {
            backend_log_t::handle().add("warn", "render rejected: missing scene");
            send_json(res, "{\"error\":\"scene is required\"}", 400);
            return;
        }

        std::string scene = params.str("scene");
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "render rejected: invalid scene");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }

        common::render_request_t rr;
        rr.scene_path = join_path(scene_dir, scene);
        std::string variant_error;
        if (!read_variant_name(params, rr.variant, variant_error)) {
            backend_log_t::handle().add("warn", "render rejected: invalid variant");
            send_json(res, "{\"error\":\"invalid variant\"}", 400);
            return;
        }

        const std::string requester_client_id = read_client_id(params);

        std::string workspace_id;
        if (params.has("workspace_id")) workspace_id = params.str("workspace_id");
        if (workspace_id.empty() && !requester_client_id.empty()) {
            workspaces.get_active(requester_client_id, workspace_id);
        }
        if (workspace_id.empty()) workspace_id = workspaces.ensure_client("");
        workspaces.set_active_scene(workspace_id, scene);

        std::string cleanup_scene_path;
        std::string draft_source;
        if (workspaces.get_scene_draft(workspace_id, scene, draft_source) && !draft_source.empty()) {
            std::string tmp_scene_path;
            if (write_workspace_temp_scene(workspace_id, scene, draft_source, tmp_scene_path)) {
                rr.scene_path = tmp_scene_path;
                cleanup_scene_path = tmp_scene_path;
            }
        }

        if (params.has("integrator")) rr.integrator = params.str("integrator");
        if (!common::is_integrator_supported(rr.integrator)) {
            backend_log_t::handle().add("warn", "render rejected: unsupported integrator=" + rr.integrator);
            send_json(res, "{\"error\":\"integrator not supported\"}", 400);
            return;
        }
        if (!parse_render_mode_param(params, "render_mode", rr.render_mode) && params.has("render_mode")) {
            backend_log_t::handle().add("warn", "render rejected: invalid render_mode");
            send_json(res, "{\"error\":\"invalid render_mode\"}", 400);
            return;
        }
        for (const auto &k : params.keys()) {
            if (!has_prefix(k, "iopt.")) continue;
            const std::string key = k.substr(5);
            if (key.empty()) continue;
            rr.integrator_options[key] = params.str(k.c_str());
        }
        std::string integrator_opt_error;
        if (!common::validate_integrator_options(rr.integrator, rr.integrator_options, integrator_opt_error)) {
            backend_log_t::handle().add("warn", "render rejected: " + integrator_opt_error);
            send_json(res, "{\"error\":\"" + json_escape(integrator_opt_error) + "\"}", 400);
            return;
        }

        if (params.has("camera")) rr.camera = params.str("camera");

        const bool has_cam_override =
            params.has("cam_px") || params.has("cam_py") || params.has("cam_pz")
            || params.has("cam_tx") || params.has("cam_ty") || params.has("cam_tz")
            || params.has("cam_upx") || params.has("cam_upy") || params.has("cam_upz")
            || params.has("cam_hfov");
        if (has_cam_override) {
            double cam_px = 0.0, cam_py = 0.0, cam_pz = 0.0;
            double cam_tx = 0.0, cam_ty = 0.0, cam_tz = 0.0;
            double cam_upx = 0.0, cam_upy = 1.0, cam_upz = 0.0;
            double cam_hfov = 60.0;
            const bool ok =
                parse_f64_param(params, "cam_px", -1e9, 1e9, cam_px)
                && parse_f64_param(params, "cam_py", -1e9, 1e9, cam_py)
                && parse_f64_param(params, "cam_pz", -1e9, 1e9, cam_pz)
                && parse_f64_param(params, "cam_tx", -1e9, 1e9, cam_tx)
                && parse_f64_param(params, "cam_ty", -1e9, 1e9, cam_ty)
                && parse_f64_param(params, "cam_tz", -1e9, 1e9, cam_tz)
                && parse_f64_param(params, "cam_upx", -1e6, 1e6, cam_upx)
                && parse_f64_param(params, "cam_upy", -1e6, 1e6, cam_upy)
                && parse_f64_param(params, "cam_upz", -1e6, 1e6, cam_upz)
                && parse_f64_param(params, "cam_hfov", 1.0, 179.0, cam_hfov);
            if (!ok) {
                backend_log_t::handle().add("warn", "render rejected: invalid camera override");
                send_json(res, "{\"error\":\"invalid camera override\"}", 400);
                return;
            }
            rr.camera_override.enabled = true;
            rr.camera_override.px = cam_px;
            rr.camera_override.py = cam_py;
            rr.camera_override.pz = cam_pz;
            rr.camera_override.tx = cam_tx;
            rr.camera_override.ty = cam_ty;
            rr.camera_override.tz = cam_tz;
            rr.camera_override.upx = cam_upx;
            rr.camera_override.upy = cam_upy;
            rr.camera_override.upz = cam_upz;
            rr.camera_override.hfov = cam_hfov;
        }

        size_t v = 0;
        if (parse_u64_param(params, "width", 8, 8192, v)) rr.width = v;
        else if (params.has("width")) {
            backend_log_t::handle().add("warn", "render rejected: invalid width");
            send_json(res, "{\"error\":\"invalid width\"}", 400);
            return;
        }
        if (parse_u64_param(params, "height", 8, 8192, v)) rr.height = v;
        else if (params.has("height")) {
            backend_log_t::handle().add("warn", "render rejected: invalid height");
            send_json(res, "{\"error\":\"invalid height\"}", 400);
            return;
        }
        if (parse_u64_param(params, "samples", 1, 1024, v)) rr.samples = v;
        else if (params.has("samples")) {
            backend_log_t::handle().add("warn", "render rejected: invalid samples");
            send_json(res, "{\"error\":\"invalid samples\"}", 400);
            return;
        }
        if (parse_u64_param(params, "aa", 1, 16, v)) rr.aa = v;
        else if (params.has("aa")) {
            backend_log_t::handle().add("warn", "render rejected: invalid aa");
            send_json(res, "{\"error\":\"invalid aa\"}", 400);
            return;
        }
        if (!parse_sample_distribution_param(params, "sample_distribution", rr.sample_distribution)
            && params.has("sample_distribution")) {
            backend_log_t::handle().add("warn", "render rejected: invalid sample_distribution");
            send_json(res, "{\"error\":\"invalid sample_distribution\"}", 400);
            return;
        }
        if (parse_u64_param(params, "rdepth", 1, 4096, v)) rr.rdepth = v;
        else if (params.has("rdepth")) {
            backend_log_t::handle().add("warn", "render rejected: invalid rdepth");
            send_json(res, "{\"error\":\"invalid rdepth\"}", 400);
            return;
        }
        if (parse_u64_param(params, "tile_size", 8, 1024, v)) rr.tile_size = v;
        else if (params.has("tile_size")) {
            backend_log_t::handle().add("warn", "render rejected: invalid tile_size");
            send_json(res, "{\"error\":\"invalid tile_size\"}", 400);
            return;
        }
        if (parse_u64_param(params, "threads", 0, 256, v)) rr.threads = v;
        else if (params.has("threads")) {
            backend_log_t::handle().add("warn", "render rejected: invalid threads");
            send_json(res, "{\"error\":\"invalid threads\"}", 400);
            return;
        }
        const size_t max_render_threads = (thread_policy.max_render_threads > 0)
            ? thread_policy.max_render_threads
            : compute_auto_render_threads(thread_policy.reserve_threads);
        const bool auto_threads_requested = (rr.threads == 0);
        const size_t requested_threads = rr.threads;
        if (!auto_threads_requested && rr.threads > max_render_threads) {
            rr.threads = max_render_threads;
        }
        if (!parse_tile_order_param(params, "tile_order", rr.tile_order) && params.has("tile_order")) {
            backend_log_t::handle().add("warn", "render rejected: invalid tile_order");
            send_json(res, "{\"error\":\"invalid tile_order\"}", 400);
            return;
        }

        {
            std::ostringstream policy_log;
            policy_log << "render thread policy workspace=" << workspace_id
                       << " mode=" << (auto_threads_requested ? "auto" : "manual")
                       << " requested_threads=" << requested_threads
                       << " effective_threads=" << (auto_threads_requested ? 0 : rr.threads)
                       << " reserve_threads=" << thread_policy.reserve_threads;
            backend_log_t::handle().add("debug", policy_log.str());
        }

        xtcore::tonemapping::settings_t initial_tm;
        std::string tm_error_json;
        parse_tonemapping_settings(params, initial_tm, tm_error_json); // best-effort; ignore errors
        std::string job_id = jobs.create(rr, scene, workspace_id, requester_client_id, cleanup_scene_path, initial_tm);
        if (job_id.empty()) {
            if (!cleanup_scene_path.empty()) unlink(cleanup_scene_path.c_str());
            backend_log_t::handle().add("warn", "render rejected: queue full workspace=" + workspace_id);
            send_json(res, "{\"error\":\"render queue is full\"}", 503);
            return;
        }
        workspaces.mark_job_started(workspace_id, job_id);
        broadcast_jobs_changed(jobs);
        std::ostringstream ss;
        ss << "{"
           << "\"job_id\":\"" << json_escape(job_id) << "\","
           << "\"workspace_id\":\"" << json_escape(workspace_id) << "\""
           << "}";
        send_json(res, ss.str(), 202);
    });

    CROW_ROUTE(app, "/api/jobs/<string>/live-tm").methods(crow::HTTPMethod::Put)
    ([&](const crow::request &req, crow::response &res, std::string id) {
        const params_view params(req);
        xtcore::tonemapping::settings_t tm_settings;
        std::string tm_error_json;
        if (!parse_tonemapping_settings(params, tm_settings, tm_error_json)) {
            send_json(res, tm_error_json, 400);
            return;
        }
        if (!jobs.set_live_tm_settings(id, tm_settings)) {
            send_json(res, "{\"error\":\"not found\"}", 404);
            return;
        }
        send_json(res, "{\"ok\":true}", 200);
    });

    CROW_ROUTE(app, "/api/jobs/<string>/image")
    ([&](const crow::request &req, crow::response &res, std::string id) {
        const params_view params(req);
        bool final_only = params.has("final") && params.str("final") == "1";
        xtcore::tonemapping::settings_t tm_settings;
        std::string tm_error_json;
        if (!parse_tonemapping_settings(params, tm_settings, tm_error_json)) {
            send_json(res, tm_error_json, 400);
            return;
        }
        bool post_filters_enabled = false;
        std::string post_filters;
        std::string post_filter_error_json;
        if (!parse_post_filter_settings(params, post_filters_enabled, post_filters, post_filter_error_json)) {
            send_json(res, post_filter_error_json, 400);
            return;
        }
        std::vector<unsigned char> image;
        if (!jobs.image(id, image, !final_only, tm_settings, post_filters_enabled, post_filters)) {
            backend_log_t::handle().add("warn", "job image missing id=" + id);
            send_json(res, "{\"error\":\"image not available\"}", 404);
            return;
        }
        res.body = std::string(reinterpret_cast<const char *>(image.data()), image.size());
        res.set_header("Content-Type", "image/png");
        res.end();
    });

    CROW_ROUTE(app, "/api/jobs/<string>/export")
    ([&](const crow::request &req, crow::response &res, std::string id) {
        const params_view params(req);
        std::string format = "png";
        if (params.has("format")) format = lower_ascii(params.str("format"));
        if (format != "png" && format != "exr" && format != "hdr"
            && format != "jpg" && format != "bmp" && format != "tga") {
            send_json(res, "{\"error\":\"unsupported format\"}", 400);
            return;
        }

        std::vector<unsigned char> image;
        std::string mime_type;
        std::string extension;
        bool post_filters_enabled = false;
        std::string post_filters;
        std::string post_filter_error_json;
        if (!parse_post_filter_settings(params, post_filters_enabled, post_filters, post_filter_error_json)) {
            send_json(res, post_filter_error_json, 400);
            return;
        }
        if (!jobs.image_export(id, format, image, mime_type, extension, post_filters_enabled, post_filters)) {
            backend_log_t::handle().add("warn", "job export unavailable id=" + id + " format=" + format);
            send_json(res, "{\"error\":\"export not available\"}", 404);
            return;
        }

        job_snapshot_t snap;
        std::string scene_token = "scene";
        if (jobs.snapshot(id, snap)) {
            scene_token = sanitize_filename_token(scene_basename_for_filename(snap.scene), "scene");
        }
        const std::string requester_client_id = read_client_id(params);
        const std::string client_token = sanitize_filename_token(requester_client_id, "client");
        const std::string filename = "xtracer_"
            + scene_token + "_"
            + client_token + "_"
            + utc_timestamp_for_filename()
            + "." + extension;
        res.set_header("Cache-Control", "no-store");
        res.set_header("Content-Disposition", ("attachment; filename=\"" + filename + "\"").c_str());
        res.body = std::string(reinterpret_cast<const char *>(image.data()), image.size());
        res.set_header("Content-Type", mime_type.c_str());
        res.end();
    });

    CROW_ROUTE(app, "/api/jobs/<string>/photons")
    ([&](const crow::request &req, crow::response &res, std::string id) {
        const params_view params(req);
        size_t limit = 100000;
        size_t v = 0;
        if (parse_u64_param(params, "limit", 1, 500000, v)) limit = v;
        else if (params.has("limit")) {
            send_json(res, "{\"error\":\"invalid limit\"}", 400);
            return;
        }

        std::vector<common::render_result_t::point3_t> diffuse;
        std::vector<common::render_result_t::point3_t> caustic;
        if (!jobs.photons(id, diffuse, caustic, limit)) {
            send_json(res, "{\"error\":\"photon data not available\"}", 404);
            return;
        }

        std::ostringstream ss;
        ss << "{\"diffuse\":[";
        for (size_t i = 0; i < diffuse.size(); ++i) {
            if (i) ss << ',';
            ss << "[" << diffuse[i].x << "," << diffuse[i].y << "," << diffuse[i].z << "]";
        }
        ss << "],\"caustic\":[";
        for (size_t i = 0; i < caustic.size(); ++i) {
            if (i) ss << ',';
            ss << "[" << caustic[i].x << "," << caustic[i].y << "," << caustic[i].z << "]";
        }
        ss << "]}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/jobs/active")
    ([&](const crow::request &, crow::response &res) {
        std::vector<job_snapshot_t> active;
        jobs.list_active(active);
        std::ostringstream ss;
        ss << "{\"jobs\":[";
        for (size_t i = 0; i < active.size(); ++i) {
            if (i) ss << ",";
            const job_snapshot_t &snap = active[i];
            ss << "{"
               << "\"id\":\"" << json_escape(snap.id) << "\","
               << "\"workspace_id\":\"" << json_escape(snap.workspace_id) << "\","
               << "\"scene\":\"" << json_escape(snap.scene) << "\","
               << "\"integrator\":\"" << json_escape(snap.integrator) << "\","
               << "\"render_mode\":\"" << json_escape(snap.render_mode) << "\","
               << "\"state\":\"" << job_state_name(snap.state) << "\","
               << "\"threads\":" << snap.threads << ","
               << "\"progress\":" << snap.progress << ","
               << "\"tiles_done\":" << snap.tiles_done << ","
               << "\"tiles_total\":" << snap.tiles_total << ","
               << "\"pass_current\":" << snap.pass_current << ","
               << "\"pass_total\":" << snap.pass_total << ","
               << "\"elapsed_ms\":" << snap.elapsed_ms << ","
               << "\"queue_index\":" << snap.queue_index
               << "}";
        }
        ss << "]}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/jobs/abort/<string>").methods(crow::HTTPMethod::Post)
    ([&](const crow::request &req, crow::response &res, std::string id) {
        job_snapshot_t snap;
        if (!jobs.snapshot(id, snap)) {
            send_json(res, "{\"error\":\"job not found\"}", 404);
            return;
        }

        const params_view params(req);
        const std::string requester_client_id = read_client_id(params);
        if (requester_client_id.empty()) {
            send_json(res, "{\"error\":\"client_id is required\"}", 400);
            return;
        }
        if (!jobs.belongs_to_client(id, requester_client_id)) {
            backend_log_t::handle().add(
                "warn",
                "job abort rejected id=" + id
                + " requester_client=" + requester_client_id
            );
            send_json(res, "{\"error\":\"job does not belong to requesting client\"}", 403);
            return;
        }

        std::string requester_workspace_id;
        if (params.has("workspace_id")) {
            requester_workspace_id = params.str("workspace_id");
        }
        if (requester_workspace_id.empty() && !requester_client_id.empty()) {
            workspaces.get_active(requester_client_id, requester_workspace_id);
        }
        if (requester_workspace_id.empty()) {
            send_json(res, "{\"error\":\"workspace context required\"}", 400);
            return;
        }
        if (snap.workspace_id != requester_workspace_id) {
            backend_log_t::handle().add(
                "warn",
                "job abort rejected id=" + id
                + " requester_workspace=" + requester_workspace_id
                + " job_workspace=" + snap.workspace_id
            );
            send_json(res, "{\"error\":\"job does not belong to active workspace\"}", 403);
            return;
        }

        if (!jobs.abort(id)) {
            send_json(res, "{\"error\":\"job not found\"}", 404);
            return;
        }
        backend_log_t::handle().add(
            "info",
            "job abort requested id=" + id + " workspace=" + requester_workspace_id
        );
        broadcast_jobs_changed(jobs);
        send_json(res, "{\"ok\":true}");
    });

    CROW_ROUTE(app, "/api/jobs/queue/up/<string>").methods(crow::HTTPMethod::Post)
    ([&](const crow::request &, crow::response &res, std::string id) {
        if (!jobs.move_queue_up(id)) {
            send_json(res, "{\"error\":\"job not found or not queued\"}", 404);
            return;
        }
        backend_log_t::handle().add("info", "job queue move up id=" + id);
        broadcast_jobs_changed(jobs);
        send_json(res, "{\"ok\":true}");
    });

    CROW_ROUTE(app, "/api/jobs/queue/down/<string>").methods(crow::HTTPMethod::Post)
    ([&](const crow::request &, crow::response &res, std::string id) {
        if (!jobs.move_queue_down(id)) {
            send_json(res, "{\"error\":\"job not found or not queued\"}", 404);
            return;
        }
        backend_log_t::handle().add("info", "job queue move down id=" + id);
        broadcast_jobs_changed(jobs);
        send_json(res, "{\"ok\":true}");
    });

    CROW_ROUTE(app, "/api/jobs/<string>")
    ([&](const crow::request &, crow::response &res, std::string id) {
        job_snapshot_t snap;
        if (!jobs.snapshot(id, snap)) {
            backend_log_t::handle().add("warn", "job lookup failed id=" + id);
            send_json(res, "{\"error\":\"job not found\"}", 404);
            return;
        }
        if (snap.state == JOB_DONE || snap.state == JOB_ABORTED || snap.state == JOB_ERROR) {
            workspaces.mark_job_finished(snap.workspace_id, snap.id);
        }
        send_json(res, job_snapshot_to_json(snap));
    });

    // Gallery API
    CROW_ROUTE(app, "/api/gallery")
    ([gallery](const crow::request &, crow::response &res) {
        if (!gallery || !gallery->is_initialized()) {
            send_json(res, "{\"entries\":[]}");
            return;
        }
        const std::vector<std::string> ids = gallery->list_entry_ids();
        std::ostringstream ss;
        ss << "{\"entries\":[";
        bool first = true;
        for (const auto &id : ids) {
            std::string meta_json;
            if (!gallery->get_meta_json(id, meta_json)) continue;
            if (!first) ss << ",";
            ss << meta_json;
            first = false;
        }
        ss << "]}";
        send_json(res, ss.str());
    });

    CROW_ROUTE(app, "/api/gallery/<string>/image")
    ([gallery](const crow::request &req, crow::response &res, std::string id) {
        if (!gallery || !gallery->is_initialized()) {
            res.code = 404;
            res.end();
            return;
        }
        std::vector<unsigned char> exr;
        if (!gallery->get_image_exr(id, exr) || exr.empty()) {
            res.code = 404;
            res.end();
            return;
        }
        nimg::Pixmap fb;
        if (nimg::io::load::exr_memory(exr.data(), exr.size(), fb) != 0) {
            res.code = 500;
            res.end();
            return;
        }
        // Build default TM settings from meta, allow query-param overrides
        xtcore::tonemapping::settings_t tm_settings;
        gallery_entry_meta_t meta;
        if (gallery->get_meta_struct(id, meta)) {
            parse_tonemapping_operator(meta.tm_op, tm_settings.op);
            tm_settings.exposure           = meta.tm_exposure;
            tm_settings.white_point        = meta.tm_white_point;
            tm_settings.mantiuk_contrast   = meta.tm_mantiuk_contrast;
            tm_settings.mantiuk_saturation = meta.tm_mantiuk_saturation;
            tm_settings.mantiuk_detail     = meta.tm_mantiuk_detail;
        }
        const params_view params(req);
        std::string tm_error_json;
        if (!parse_tonemapping_settings(params, tm_settings, tm_error_json)) {
            send_json(res, tm_error_json, 400);
            return;
        }
        xtcore::tonemapping::apply(fb, tm_settings);
        std::vector<unsigned char> png;
        if (nimg::io::save::png_memory(fb, png) != 0) {
            res.code = 500;
            res.end();
            return;
        }
        res.body = std::string(reinterpret_cast<const char *>(png.data()), png.size());
        res.set_header("Content-Type", "image/png");
        res.end();
    });

    CROW_ROUTE(app, "/api/gallery/<string>/pass/<uint>/image")
    ([gallery](const crow::request &req, crow::response &res, std::string id, unsigned int pass_index) {
        if (!gallery || !gallery->is_initialized()) {
            res.code = 404;
            res.end();
            return;
        }
        std::vector<unsigned char> exr;
        if (!gallery->get_pass_image_exr(id, static_cast<size_t>(pass_index), exr) || exr.empty()) {
            res.code = 404;
            res.end();
            return;
        }
        nimg::Pixmap fb;
        if (nimg::io::load::exr_memory(exr.data(), exr.size(), fb) != 0) {
            res.code = 500;
            res.end();
            return;
        }
        xtcore::tonemapping::settings_t tm_settings;
        gallery_entry_meta_t meta;
        if (gallery->get_meta_struct(id, meta)) {
            parse_tonemapping_operator(meta.tm_op, tm_settings.op);
            tm_settings.exposure           = meta.tm_exposure;
            tm_settings.white_point        = meta.tm_white_point;
            tm_settings.mantiuk_contrast   = meta.tm_mantiuk_contrast;
            tm_settings.mantiuk_saturation = meta.tm_mantiuk_saturation;
            tm_settings.mantiuk_detail     = meta.tm_mantiuk_detail;
        }
        const params_view params(req);
        std::string tm_error_json;
        if (!parse_tonemapping_settings(params, tm_settings, tm_error_json)) {
            send_json(res, tm_error_json, 400);
            return;
        }
        xtcore::tonemapping::apply(fb, tm_settings);
        std::vector<unsigned char> png;
        if (nimg::io::save::png_memory(fb, png) != 0) {
            res.code = 500;
            res.end();
            return;
        }
        res.body = std::string(reinterpret_cast<const char *>(png.data()), png.size());
        res.set_header("Content-Type", "image/png");
        res.end();
    });

    CROW_ROUTE(app, "/api/gallery/<string>/export")
    ([gallery](const crow::request &req, crow::response &res, std::string id) {
        if (!gallery || !gallery->is_initialized()) {
            res.code = 404;
            res.end();
            return;
        }
        const params_view params(req);
        std::string format = "png";
        if (params.has("format")) format = lower_ascii(params.str("format"));
        if (format != "png" && format != "jpg" && format != "bmp" && format != "tga"
            && format != "exr" && format != "hdr") {
            send_json(res, "{\"error\":\"unsupported format\"}", 400);
            return;
        }
        std::vector<unsigned char> exr;
        if (!gallery->get_image_exr(id, exr) || exr.empty()) {
            res.code = 404;
            res.end();
            return;
        }
        nimg::Pixmap fb;
        if (nimg::io::load::exr_memory(exr.data(), exr.size(), fb) != 0) {
            res.code = 500;
            res.end();
            return;
        }
        gallery_entry_meta_t meta;
        const bool has_meta = gallery->get_meta_struct(id, meta);
        const bool is_hdr_format = (format == "exr" || format == "hdr");
        if (!is_hdr_format) {
            xtcore::tonemapping::settings_t tm_settings;
            if (has_meta) {
                parse_tonemapping_operator(meta.tm_op, tm_settings.op);
                tm_settings.exposure           = meta.tm_exposure;
                tm_settings.white_point        = meta.tm_white_point;
                tm_settings.mantiuk_contrast   = meta.tm_mantiuk_contrast;
                tm_settings.mantiuk_saturation = meta.tm_mantiuk_saturation;
                tm_settings.mantiuk_detail     = meta.tm_mantiuk_detail;
            }
            std::string tm_error_json;
            if (!parse_tonemapping_settings(params, tm_settings, tm_error_json)) {
                send_json(res, tm_error_json, 400);
                return;
            }
            xtcore::tonemapping::apply(fb, tm_settings);
        }
        std::vector<unsigned char> image;
        std::string mime_type;
        int encode_result = 1;
        if (format == "png") {
            encode_result = nimg::io::save::png_memory(fb, image);
            mime_type = "image/png";
        } else if (format == "jpg") {
            encode_result = nimg::io::save::jpg_memory(fb, image);
            mime_type = "image/jpeg";
        } else if (format == "bmp") {
            encode_result = nimg::io::save::bmp_memory(fb, image);
            mime_type = "image/bmp";
        } else if (format == "tga") {
            encode_result = nimg::io::save::tga_memory(fb, image);
            mime_type = "image/x-tga";
        } else if (format == "exr") {
            encode_result = nimg::io::save::exr_memory(fb, image);
            mime_type = "image/x-exr";
        } else if (format == "hdr") {
            encode_result = nimg::io::save::hdr_memory(fb, image);
            mime_type = "image/vnd.radiance";
        }
        if (encode_result != 0 || image.empty()) {
            res.code = 500;
            res.end();
            return;
        }
        const std::string scene_token = has_meta
            ? sanitize_filename_token(scene_basename_for_filename(meta.scene), "gallery")
            : std::string("gallery");
        const std::string client_token = sanitize_filename_token(read_client_id(params), "client");
        const std::string filename = "xtracer_"
            + scene_token + "_"
            + client_token + "_"
            + utc_timestamp_for_filename()
            + "." + format;
        res.set_header("Cache-Control", "no-store");
        res.set_header("Content-Disposition", ("attachment; filename=\"" + filename + "\"").c_str());
        res.body = std::string(reinterpret_cast<const char *>(image.data()), image.size());
        res.set_header("Content-Type", mime_type.c_str());
        res.end();
    });

    CROW_ROUTE(app, "/api/gallery/<string>/pass/<uint>/export")
    ([gallery](const crow::request &req, crow::response &res, std::string id, unsigned int pass_index) {
        if (!gallery || !gallery->is_initialized()) {
            res.code = 404;
            res.end();
            return;
        }
        const params_view params(req);
        std::string format = "png";
        if (params.has("format")) format = lower_ascii(params.str("format"));
        if (format != "png" && format != "jpg" && format != "bmp" && format != "tga"
            && format != "exr" && format != "hdr") {
            send_json(res, "{\"error\":\"unsupported format\"}", 400);
            return;
        }
        std::vector<unsigned char> exr;
        if (!gallery->get_pass_image_exr(id, static_cast<size_t>(pass_index), exr) || exr.empty()) {
            res.code = 404;
            res.end();
            return;
        }
        nimg::Pixmap fb;
        if (nimg::io::load::exr_memory(exr.data(), exr.size(), fb) != 0) {
            res.code = 500;
            res.end();
            return;
        }
        gallery_entry_meta_t meta;
        const bool has_meta = gallery->get_meta_struct(id, meta);
        const bool is_hdr_format = (format == "exr" || format == "hdr");
        if (!is_hdr_format) {
            xtcore::tonemapping::settings_t tm_settings;
            if (has_meta) {
                parse_tonemapping_operator(meta.tm_op, tm_settings.op);
                tm_settings.exposure           = meta.tm_exposure;
                tm_settings.white_point        = meta.tm_white_point;
                tm_settings.mantiuk_contrast   = meta.tm_mantiuk_contrast;
                tm_settings.mantiuk_saturation = meta.tm_mantiuk_saturation;
                tm_settings.mantiuk_detail     = meta.tm_mantiuk_detail;
            }
            std::string tm_error_json;
            if (!parse_tonemapping_settings(params, tm_settings, tm_error_json)) {
                send_json(res, tm_error_json, 400);
                return;
            }
            xtcore::tonemapping::apply(fb, tm_settings);
        }
        std::vector<unsigned char> image;
        std::string mime_type;
        int encode_result = 1;
        if (format == "png") {
            encode_result = nimg::io::save::png_memory(fb, image);
            mime_type = "image/png";
        } else if (format == "jpg") {
            encode_result = nimg::io::save::jpg_memory(fb, image);
            mime_type = "image/jpeg";
        } else if (format == "bmp") {
            encode_result = nimg::io::save::bmp_memory(fb, image);
            mime_type = "image/bmp";
        } else if (format == "tga") {
            encode_result = nimg::io::save::tga_memory(fb, image);
            mime_type = "image/x-tga";
        } else if (format == "exr") {
            encode_result = nimg::io::save::exr_memory(fb, image);
            mime_type = "image/x-exr";
        } else if (format == "hdr") {
            encode_result = nimg::io::save::hdr_memory(fb, image);
            mime_type = "image/vnd.radiance";
        }
        if (encode_result != 0 || image.empty()) {
            res.code = 500;
            res.end();
            return;
        }
        const std::string scene_token = has_meta
            ? sanitize_filename_token(scene_basename_for_filename(meta.scene), "gallery")
            : std::string("gallery");
        const std::string client_token = sanitize_filename_token(read_client_id(params), "client");
        const std::string filename = "xtracer_"
            + scene_token + "_"
            + client_token + "_"
            + utc_timestamp_for_filename()
            + "." + format;
        res.set_header("Cache-Control", "no-store");
        res.set_header("Content-Disposition", ("attachment; filename=\"" + filename + "\"").c_str());
        res.body = std::string(reinterpret_cast<const char *>(image.data()), image.size());
        res.set_header("Content-Type", mime_type.c_str());
        res.end();
    });

    CROW_ROUTE(app, "/api/gallery/<string>").methods(crow::HTTPMethod::Delete)
    ([gallery](const crow::request &, crow::response &res, std::string id) {
        if (!gallery || !gallery->is_initialized()) {
            send_json(res, "{\"ok\":false}", 503);
            return;
        }
        if (gallery->delete_entry(id)) {
            send_json(res, "{\"ok\":true}");
        } else {
            send_json(res, "{\"ok\":false}", 404);
        }
    });

    // Static files
    CROW_ROUTE(app, "/")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "index.html"), "text/html", res);
    });

    CROW_ROUTE(app, "/showcase.html")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "showcase.html"), "text/html", res);
    });

    CROW_ROUTE(app, "/sampling.html")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "sampling.html"), "text/html", res);
    });

    CROW_ROUTE(app, "/sampling.js")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "sampling.js"), "application/javascript", res);
    });

    CROW_ROUTE(app, "/furnace.html")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "furnace.html"), "text/html", res);
    });

    CROW_ROUTE(app, "/furnace.js")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "furnace.js"), "application/javascript", res);
    });

    CROW_ROUTE(app, "/app.js")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "app.js"), "application/javascript", res);
    });

    CROW_ROUTE(app, "/showcase.js")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "showcase.js"), "application/javascript", res);
    });

    CROW_ROUTE(app, "/app/<string>")
    ([web_root](const crow::request &, crow::response &res, std::string name) {
        serve_static_file(join_path(web_root, "app/" + name), "application/javascript", res);
    });

    CROW_ROUTE(app, "/app/widgets/<string>")
    ([web_root](const crow::request &, crow::response &res, std::string name) {
        serve_static_file(join_path(web_root, "app/widgets/" + name), "application/javascript", res);
    });

    CROW_ROUTE(app, "/visual_editor.js")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "visual_editor.js"), "application/javascript", res);
    });

    CROW_ROUTE(app, "/vendor/three.min.js")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "vendor/three.min.js"), "application/javascript", res);
    });

    CROW_ROUTE(app, "/wasm_adapter.js")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "wasm_adapter.js"), "application/javascript", res);
    });

    CROW_ROUTE(app, "/wasm_worker.js")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "wasm_worker.js"), "application/javascript", res);
    });

    CROW_ROUTE(app, "/xtracer_wasm.js")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "xtracer_wasm.js"), "application/javascript", res);
    });

    CROW_ROUTE(app, "/xtracer_wasm.wasm")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "xtracer_wasm.wasm"), "application/wasm", res);
    });

    CROW_ROUTE(app, "/app/data/<string>")
    ([web_root](const crow::request &, crow::response &res, std::string name) {
        serve_static_file(join_path(web_root, "app/data/" + name), "application/json", res);
    });

    CROW_ROUTE(app, "/scenes/<string>")
    ([web_root](const crow::request &, crow::response &res, std::string scene) {
        serve_static_file(join_path(web_root, "scenes/" + scene), "text/plain", res);
    });

    CROW_ROUTE(app, "/styles.css")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "styles.css"), "text/css", res);
    });

    CROW_ROUTE(app, "/styles/<string>")
    ([web_root](const crow::request &, crow::response &res, std::string name) {
        serve_static_file(join_path(web_root, "styles/" + name), "text/css", res);
    });

    CROW_ROUTE(app, "/preview.jpg")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "preview.jpg"), "image/jpeg", res);
    });

    CROW_ROUTE(app, "/logo.png")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "logo.png"), "image/png", res);
    });

    CROW_ROUTE(app, "/logo.svg")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "logo.svg"), "image/svg+xml", res);
    });

    CROW_ROUTE(app, "/res/<string>")
    ([web_root](const crow::request &, crow::response &res, std::string name) {
        const std::string direct_path = join_path("res", name);
        if (file_exists(direct_path)) {
            serve_static_file(direct_path, "image/png", res);
            return;
        }
        const std::string web_res_path = join_path(join_path(web_root, "res"), name);
        if (file_exists(web_res_path)) {
            serve_static_file(web_res_path, "image/png", res);
            return;
        }
        const std::string repo_root = join_path(join_path(join_path(web_root, ".."), ".."), "..");
        const std::string repo_res_path = join_path(join_path(repo_root, "res"), name);
        serve_static_file(repo_res_path, "image/png", res);
    });

    CROW_ROUTE(app, "/license.txt")
    ([web_root](const crow::request &, crow::response &res) {
        serve_static_file(join_path(web_root, "license.txt"), "text/plain; charset=utf-8", res);
    });

    // WebSocket: job status + tile streaming
    // WebSocket: job-list event notifications (no job ID required)
    // Broadcasts {"type":"jobs_changed","jobs":[...]} whenever any job is
    // created, reaches a terminal state, is aborted, or the queue order
    // changes.  Also sends the current job list immediately on connect so
    // clients never need to call /api/jobs/active via REST.
    CROW_WEBSOCKET_ROUTE(app, "/ws/jobs")
        .onopen([&](crow::websocket::connection &conn) {
            g_job_events_ws_hub.subscribe(&conn);
            // Send the current active job list to this new subscriber so it
            // has an up-to-date snapshot without waiting for the next mutation.
            std::vector<job_snapshot_t> active;
            jobs.list_active(active);
            std::ostringstream ss;
            ss << "{\"type\":\"jobs_changed\",\"jobs\":[";
            for (size_t i = 0; i < active.size(); ++i) {
                if (i) ss << ",";
                const job_snapshot_t &snap = active[i];
                ss << "{"
                   << "\"id\":\"" << json_escape(snap.id) << "\","
                   << "\"workspace_id\":\"" << json_escape(snap.workspace_id) << "\","
                   << "\"scene\":\"" << json_escape(snap.scene) << "\","
                   << "\"integrator\":\"" << json_escape(snap.integrator) << "\","
                   << "\"render_mode\":\"" << json_escape(snap.render_mode) << "\","
                   << "\"state\":\"" << job_state_name(snap.state) << "\","
                   << "\"threads\":" << snap.threads << ","
                   << "\"progress\":" << snap.progress << ","
                   << "\"tiles_done\":" << snap.tiles_done << ","
                   << "\"tiles_total\":" << snap.tiles_total << ","
                   << "\"pass_current\":" << snap.pass_current << ","
                   << "\"pass_total\":" << snap.pass_total << ","
                   << "\"elapsed_ms\":" << snap.elapsed_ms << ","
                   << "\"queue_index\":" << snap.queue_index
                   << "}";
            }
            ss << "]}";
            try { conn.send_text(ss.str()); } catch (...) {}
        })
        .onclose([&](crow::websocket::connection &conn, const std::string &) {
            g_job_events_ws_hub.unsubscribe(&conn);
        });

    // onaccept extracts job_id and client_tag from the request and stores them
    // in a heap-allocated struct; onopen/onmessage/onclose retrieve it via userdata().
    struct ws_job_conn_data_t {
        std::string job_id;
        std::string client_tag;
    };

    CROW_WEBSOCKET_ROUTE(app, "/ws/jobs/<string>")
        .onaccept([](const crow::request &req, void **userdata) -> bool {
            const std::string &url = req.url;
            const size_t pos = url.rfind('/');
            const std::string job_id = (pos != std::string::npos) ? url.substr(pos + 1) : "";
            if (job_id.empty()) return false;
            auto *d = new ws_job_conn_data_t;
            d->job_id     = job_id;
            d->client_tag = rlm_request_client_tag(req);
            *userdata = d;
            return true;
        })
        .onopen([&](crow::websocket::connection &conn) {
            auto *d = static_cast<ws_job_conn_data_t *>(conn.userdata());
            if (!d) { conn.close("bad state"); return; }
            const std::string &job_id = d->job_id;
            job_snapshot_t snap;
            if (!jobs.snapshot(job_id, snap)) {
                conn.close("job not found");
                return;
            }
            // Subscribe first so no tiles are lost between the initial send and live pushes.
            g_job_ws_hub.subscribe(job_id, &conn);
            // Send a full-frame XTDR catchup (raw RGBA) so the client can use
            // putImageData — no PNG encode/decode at all.
            bool has_catchup = false;
            if (snap.tiles_done > 0 && snap.width > 0 && snap.height > 0) {
                std::vector<unsigned char> rgba;
                size_t cw = 0, ch = 0, cdone = 0, ctotal = 0;
                if (jobs.image_rgba(job_id, rgba, cw, ch, cdone, ctotal) && !rgba.empty()) {
                    // Build an XTDR packet: header (32 bytes) + one tile record + 0 active tiles.
                    auto push_u32le = [](std::vector<unsigned char> &buf, uint32_t v) {
                        buf.push_back((unsigned char)(v & 0xFF));
                        buf.push_back((unsigned char)((v >> 8) & 0xFF));
                        buf.push_back((unsigned char)((v >> 16) & 0xFF));
                        buf.push_back((unsigned char)((v >> 24) & 0xFF));
                    };
                    std::vector<unsigned char> pkt;
                    pkt.reserve(32 + 24 + rgba.size() + 4);
                    pkt.push_back('X'); pkt.push_back('T');
                    pkt.push_back('D'); pkt.push_back('R');
                    push_u32le(pkt, (uint32_t)cw);
                    push_u32le(pkt, (uint32_t)ch);
                    push_u32le(pkt, (uint32_t)cdone);
                    push_u32le(pkt, (uint32_t)ctotal);
                    push_u32le(pkt, (uint32_t)JOB_RUNNING);
                    push_u32le(pkt, 1u); // tile_count
                    push_u32le(pkt, (uint32_t)snap.elapsed_ms);
                    // Tile record: full frame as single tile.
                    push_u32le(pkt, 0u);              // x0
                    push_u32le(pkt, 0u);              // y0
                    push_u32le(pkt, (uint32_t)cw);    // x1
                    push_u32le(pkt, (uint32_t)ch);    // y1
                    push_u32le(pkt, (uint32_t)cdone); // done_index
                    push_u32le(pkt, (uint32_t)rgba.size());
                    pkt.insert(pkt.end(), rgba.begin(), rgba.end());
                    // Active tiles section: none (catchup frame, not a live event).
                    push_u32le(pkt, 0u);
                    conn.send_binary(std::string(reinterpret_cast<const char *>(pkt.data()), pkt.size()));
                    has_catchup = true;
                }
            }
            // Refresh snapshot after image fetch so progress/pass info is consistent
            // with the catchup packet (the earlier snap may predate a few tile completions).
            jobs.snapshot(job_id, snap);
            // Send current status snapshot.
            conn.send_text(job_snapshot_to_json(snap));
            // Log after subscribe + initial send so the annotation is accurate.
            std::string detail = "job=" + job_id;
            if (has_catchup) detail += "  full-frame catchup";
            rlm_ws_log(d->client_tag, "OPEN", "/ws/jobs/" + job_id, detail);
        })
        .onmessage([&](crow::websocket::connection &conn, const std::string &data, bool is_binary) {
            if (!is_binary && data.find("\"abort\"") != std::string::npos) {
                auto *d = static_cast<ws_job_conn_data_t *>(conn.userdata());
                if (d) {
                    jobs.abort(d->job_id);
                    rlm_ws_log(d->client_tag, "ABORT", "/ws/jobs/" + d->job_id);
                }
            }
        })
        .onclose([&](crow::websocket::connection &conn, const std::string &reason) {
            auto *d = static_cast<ws_job_conn_data_t *>(conn.userdata());
            if (d) {
                rlm_ws_log(d->client_tag, "CLOSE", "/ws/jobs/" + d->job_id,
                           reason.empty() ? "" : "reason=" + reason);
                g_job_ws_hub.unsubscribe(&conn);
                delete d;
                conn.userdata(nullptr);
            } else {
                g_job_ws_hub.unsubscribe(&conn);
            }
        });

    // WebSocket: backend log streaming
    // onaccept extracts "since" query param and client_tag; stores both in userdata.
    struct ws_logs_conn_data_t {
        unsigned long long since_id;
        std::string client_tag;
    };

    CROW_WEBSOCKET_ROUTE(app, "/ws/logs")
        .onaccept([](const crow::request &req, void **userdata) -> bool {
            const char *since_str = req.url_params.get("since");
            unsigned long long since_id = 0;
            if (since_str) {
                try { since_id = std::stoull(since_str); } catch (...) {}
            }
            auto *d = new ws_logs_conn_data_t;
            d->since_id   = since_id;
            d->client_tag = rlm_request_client_tag(req);
            *userdata = d;
            return true;
        })
        .onopen([&](crow::websocket::connection &conn) {
            auto *d = static_cast<ws_logs_conn_data_t *>(conn.userdata());
            const unsigned long long since_id = d ? d->since_id : 0;
            auto entries = backend_log_t::handle().since(since_id);
            if (!entries.empty()) {
                std::ostringstream ss;
                ss << "{\"entries\":[";
                for (size_t i = 0; i < entries.size(); ++i) {
                    if (i) ss << ",";
                    ss << "{"
                       << "\"id\":" << entries[i].id << ","
                       << "\"ts\":\"" << json_escape(entries[i].timestamp) << "\","
                       << "\"level\":\"" << json_escape(entries[i].level) << "\","
                       << "\"message\":\"" << json_escape(entries[i].message) << "\""
                       << "}";
                }
                ss << "]}";
                conn.send_text(ss.str());
            }
            g_log_ws_hub.subscribe(&conn);
            const std::string path = since_id > 0
                ? "/ws/logs?since=" + std::to_string(since_id)
                : "/ws/logs";
            std::string detail;
            if (!entries.empty()) detail = std::to_string(entries.size()) + " entries catchup";
            rlm_ws_log(d ? d->client_tag : "", "OPEN", path, detail);
        })
        .onclose([&](crow::websocket::connection &conn, const std::string &reason) {
            auto *d = static_cast<ws_logs_conn_data_t *>(conn.userdata());
            rlm_ws_log(d ? d->client_tag : "", "CLOSE", "/ws/logs",
                       reason.empty() ? "" : "reason=" + reason);
            g_log_ws_hub.unsubscribe(&conn);
            if (d) { delete d; conn.userdata(nullptr); }
        });

    // Wire push callbacks so renders and log entries are pushed to WS subscribers
    jobs.set_push_callback([&](const std::string &job_id,
                                const job_snapshot_t &snap,
                                const std::vector<unsigned char> &tile_bytes) {
        if (tile_bytes.empty()) {
            // Terminal state (done/aborted/error): send text snapshot so clients
            // know to fetch the final image or handle completion, then immediately
            // broadcast the updated (now-empty) job list to /ws/jobs subscribers.
            g_job_ws_hub.broadcast_text(job_id, job_snapshot_to_json(snap));
            if (snap.state == JOB_DONE || snap.state == JOB_ABORTED || snap.state == JOB_ERROR) {
                broadcast_jobs_changed(jobs);
            }
        } else {
            // Per-tile update: binary only to per-job render subscribers.
            // The /ws/jobs heartbeat thread keeps the jobs card current; no
            // extra broadcast needed here.
            g_job_ws_hub.broadcast_binary(
                job_id,
                std::string(reinterpret_cast<const char *>(tile_bytes.data()), tile_bytes.size()));
        }
    });

    backend_log_t::handle().set_push_callback([&](const backend_log_entry_t &e) {
        std::ostringstream ss;
        ss << "{\"entries\":[{"
           << "\"id\":" << e.id << ","
           << "\"ts\":\"" << json_escape(e.timestamp) << "\","
           << "\"level\":\"" << json_escape(e.level) << "\","
           << "\"message\":\"" << json_escape(e.message) << "\""
           << "}]}";
        g_log_ws_hub.broadcast_text(ss.str());
    });

    // Heartbeat thread: push the full active job list to all /ws/jobs
    // subscribers every second.  Guarantees the jobs card stays current during
    // renders (queued→running transition, live elapsed_ms / progress) without
    // relying on per-tile mutation broadcasts.  Detached — runs for server lifetime.
    std::thread([&jobs]() {
        for (;;) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            broadcast_jobs_changed(jobs);
        }
    }).detach();
}

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */
