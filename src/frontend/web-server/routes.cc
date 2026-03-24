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

#include <cpp-httplib/httplib.h>
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
#include <xtcore/sampler/sampler_graphpaper.h>
#include <xtcore/sampler/sampler_tex.h>
#include <xtcore/sampler/sampler_weave.h>
#include <xtcore/scene.h>
#include <xtcore/strpool.h>
#include <xtcore/tonemapping/tonemapping.h>
#include <xtcore/xtcore.h>

#include "backend_log.h"
#include "job_manager.h"
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

void send_json(httplib::Response &res, const std::string &json, int status = 200)
{
    res.status = status;
    res.set_header("Cache-Control", "no-store");
    res.set_content(json, "application/json");
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

bool finite_aabb3(const xtcore::AABB3 &box)
{
    return std::isfinite((double)box.min.x) && std::isfinite((double)box.min.y) && std::isfinite((double)box.min.z)
        && std::isfinite((double)box.max.x) && std::isfinite((double)box.max.y) && std::isfinite((double)box.max.z);
}

bool parse_u64_param(const httplib::Request &req, const char *key, size_t min_v, size_t max_v, size_t &out)
{
    if (!req.has_param(key)) return false;
    std::string s = req.get_param_value(key);
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

bool parse_f64_param(const httplib::Request &req, const char *key, double min_v, double max_v, double &out)
{
    if (!req.has_param(key)) return false;
    const std::string s = req.get_param_value(key);
    if (s.empty()) return false;
    std::istringstream ss(s);
    double v = 0.0;
    ss >> v;
    if (ss.fail() || !std::isfinite(v)) return false;
    if (v < min_v || v > max_v) return false;
    out = v;
    return true;
}

bool parse_tile_order_param(const httplib::Request &req, const char *key, xtcore::render::TILE_ORDER &out)
{
    if (!req.has_param(key)) return false;
    std::string s = req.get_param_value(key);
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

bool parse_render_mode_param(const httplib::Request &req,
                             const char *key,
                             common::render_request_t::render_mode_t &out)
{
    if (!req.has_param(key)) return false;
    std::string s = req.get_param_value(key);
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });
    if (s == "normal") {
        out = common::render_request_t::RENDER_MODE_NORMAL;
        return true;
    }
    if (s == "progressive") {
        out = common::render_request_t::RENDER_MODE_PROGRESSIVE;
        return true;
    }
    if (s == "interactive") {
        out = common::render_request_t::RENDER_MODE_INTERACTIVE;
        return true;
    }
    return false;
}

bool parse_sample_distribution_param(const httplib::Request &req,
                                     const char *key,
                                     xtcore::antialiasing::SAMPLE_DISTRIBUTION &out)
{
    if (!req.has_param(key)) return false;
    std::string s = req.get_param_value(key);
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });
    if (s == "grid" || s == "grid_aligned") {
        out = xtcore::antialiasing::SAMPLE_DISTRIBUTION_GRID;
        return true;
    }
    if (s == "random" || s == "monte_carlo") {
        out = xtcore::antialiasing::SAMPLE_DISTRIBUTION_RANDOM;
        return true;
    }
    return false;
}

bool parse_tonemapping_operator(const std::string &s, xtcore::tonemapping::operator_t &out)
{
    if (s == "aces") {
        out = xtcore::tonemapping::OP_ACES_FITTED;
        return true;
    }
    if (s == "reinhard") {
        out = xtcore::tonemapping::OP_REINHARD;
        return true;
    }
    if (s == "reinhard_luma") {
        out = xtcore::tonemapping::OP_REINHARD_LUMINANCE;
        return true;
    }
    if (s == "mantiuk_2006") {
        out = xtcore::tonemapping::OP_MANTIUK_2006;
        return true;
    }
    if (s == "none") {
        out = xtcore::tonemapping::OP_NONE;
        return true;
    }
    return false;
}

bool parse_tonemapping_settings(const httplib::Request &req, xtcore::tonemapping::settings_t &tm_settings, std::string &error_json)
{
    if (req.has_param("tm")) {
        std::string tm = req.get_param_value("tm");
        std::transform(tm.begin(), tm.end(), tm.begin(),
            [](unsigned char c) { return (char)std::tolower(c); });
        if (!parse_tonemapping_operator(tm, tm_settings.op)) {
            error_json = "{\"error\":\"invalid tone mapping operator\"}";
            return false;
        }
    }
    if (req.has_param("tm_exposure")) {
        std::istringstream es(req.get_param_value("tm_exposure"));
        float exposure = 1.0f;
        es >> exposure;
        if (es.fail() || exposure <= 0.0f) {
            error_json = "{\"error\":\"invalid tone mapping exposure\"}";
            return false;
        }
        tm_settings.exposure = exposure;
    }
    if (req.has_param("tm_white_point")) {
        std::istringstream ws(req.get_param_value("tm_white_point"));
        float white_point = 1.0f;
        ws >> white_point;
        if (ws.fail() || white_point <= 0.0f) {
            error_json = "{\"error\":\"invalid tone mapping white point\"}";
            return false;
        }
        tm_settings.white_point = white_point;
    }
    if (req.has_param("tm_mantiuk_contrast")) {
        std::istringstream cs(req.get_param_value("tm_mantiuk_contrast"));
        float v = 0.1f;
        cs >> v;
        if (cs.fail() || v < 0.0f || v > 1.0f) {
            error_json = "{\"error\":\"invalid mantiuk contrast\"}";
            return false;
        }
        tm_settings.mantiuk_contrast = v;
    }
    if (req.has_param("tm_mantiuk_saturation")) {
        std::istringstream ss(req.get_param_value("tm_mantiuk_saturation"));
        float v = 0.8f;
        ss >> v;
        if (ss.fail() || v < 0.0f || v > 2.0f) {
            error_json = "{\"error\":\"invalid mantiuk saturation\"}";
            return false;
        }
        tm_settings.mantiuk_saturation = v;
    }
    if (req.has_param("tm_mantiuk_detail")) {
        std::istringstream ds(req.get_param_value("tm_mantiuk_detail"));
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

void append_u32le(std::vector<unsigned char> &out, uint32_t v)
{
    out.push_back((unsigned char)(v & 0xFFu));
    out.push_back((unsigned char)((v >> 8) & 0xFFu));
    out.push_back((unsigned char)((v >> 16) & 0xFFu));
    out.push_back((unsigned char)((v >> 24) & 0xFFu));
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

struct third_party_dep_t {
    const char *name;
    const char *license;
    const char *url;
};

void append_third_party_licenses_json(std::ostringstream &ss)
{
    static const third_party_dep_t deps[] = {
        { "TinyObjLoader", "MIT", "https://github.com/syoyo/tinyobjloader" },
        { "STB", "Public Domain / MIT", "https://github.com/nothings/stb" },
        { "TinyEXR", "BSD-3-Clause", "https://github.com/syoyo/tinyexr" },
        { "strpool", "Public Domain", "https://github.com/mattiasgustavsson/libs" },
        { "cpp-httplib", "MIT", "https://github.com/yhirose/cpp-httplib" },
        { "RtMidi", "MIT-style", "https://github.com/thestk/rtmidi" },
        { "Three.js", "MIT", "https://github.com/mrdoob/three.js" },
    };

    ss << '[';
    for (size_t i = 0; i < (sizeof(deps) / sizeof(deps[0])); ++i) {
        if (i) ss << ',';
        ss << "{"
           << "\"name\":\"" << json_escape(deps[i].name ? deps[i].name : "") << "\","
           << "\"license\":\"" << json_escape(deps[i].license ? deps[i].license : "") << "\","
           << "\"url\":\"" << json_escape(deps[i].url ? deps[i].url : "") << "\""
           << "}";
    }
    ss << ']';
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

std::string read_client_id(const httplib::Request &req)
{
    if (!req.has_param("client_id")) return "";
    const std::string client_id = req.get_param_value("client_id");
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

bool read_variant_name(const httplib::Request &req, std::string &variant, std::string &error)
{
    variant.clear();
    error.clear();
    if (!req.has_param("variant")) return true;
    variant = req.get_param_value("variant");
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

std::string scene_geometry_json_from_scene(const xtcore::Scene &scene)
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

    ss << "}}";
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
    return "material";
}

const char *sampler_type_name(const xtcore::sampler::ISampler *sampler)
{
    if (!sampler) return "sampler";
    if (dynamic_cast<const xtcore::sampler::Texture2D *>(sampler)) return "texture";
    if (dynamic_cast<const xtcore::sampler::Cubemap *>(sampler)) return "cubemap";
    if (dynamic_cast<const xtcore::sampler::ERP *>(sampler)) return "erp";
    if (dynamic_cast<const xtcore::sampler::Gradient *>(sampler)) return "gradient";
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
        const xtcore::camera::Perspective *pcam = dynamic_cast<const xtcore::camera::Perspective *>(cam);
        const xtcore::camera::ERP *ecam = dynamic_cast<const xtcore::camera::ERP *>(cam);
        const xtcore::camera::ODS *ocam = dynamic_cast<const xtcore::camera::ODS *>(cam);
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

        std::uint64_t mtime = 0ULL;
        if (!file_mtime(scene_path, mtime)) {
            out.error = "scene file not found";
            return out;
        }

        unsigned long long pending_job_id = 0ULL;
        {
            std::lock_guard<std::mutex> lock(mut);
            auto it = entries.find(key);
            if (it != entries.end() && it->second && it->second->mtime == mtime) {
                out.cache_hit = true;
                backend_log_t::handle().add("debug",
                                            "scene cache hit path=" + scene_path + " variant=" + variant);
                out.entry = it->second;
                return out;
            }

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
        for (auto eit = entries.begin(); eit != entries.end(); ) {
            if (scene_cache_key_matches_path(eit->first, scene_path)) {
                eit = entries.erase(eit);
            } else {
                ++eit;
            }
        }
    }

    bool load_job_snapshot(unsigned long long id, xtcore::io::scn::async_load_snapshot_t &snapshot)
    {
        return xtcore::io::scn::load_async_snapshot(id, &snapshot);
    }

    private:
    scene_cache_t() = default;
    scene_cache_t(const scene_cache_t &) = delete;
    scene_cache_t &operator=(const scene_cache_t &) = delete;

    std::mutex mut;
    std::unordered_map<std::string, std::shared_ptr<scene_cache_entry_t> > entries;
    std::unordered_map<std::string, scene_pending_load_t> pending;
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

void send_scene_loading(httplib::Response &res, unsigned long long load_job_id)
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

void serve_static_file(const std::string &path, const char *mime, httplib::Response &res)
{
    std::vector<char> content;
    if (!read_binary_file(path, content)) {
        res.status = 404;
        return;
    }
    res.set_content(content.data(), content.size(), mime);
}

} // namespace

void setup_routes(httplib::Server &server,
                  job_manager_t &jobs,
                  workspace_manager_t &workspaces,
                  const std::string &scene_dir,
                  const std::string &web_root,
                  const render_thread_policy_t &thread_policy)
{
    backend_log_t::handle().add("info", "web routes initialized");

    server.Get("/api/health", [](const httplib::Request &, httplib::Response &res) {
        send_json(res, "{\"ok\":true}");
    });

    server.Get("/api/about", [&jobs, thread_policy](const httplib::Request &, httplib::Response &res) {
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
        append_third_party_licenses_json(ss);
        ss
           << "}";
        send_json(res, ss.str());
    });

    server.Get("/api/workspaces", [&](const httplib::Request &req, httplib::Response &res) {
        const std::string client_id = read_client_id(req);
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
            if (i) ss << ",";
            ss << "{"
               << "\"id\":\"" << json_escape(w.id) << "\","
               << "\"name\":\"" << json_escape(w.name) << "\","
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

    server.Post("/api/workspaces", [&](const httplib::Request &req, httplib::Response &res) {
        const std::string client_id = read_client_id(req);
        const std::string name = req.has_param("name") ? req.get_param_value("name") : "";
        const std::string workspace_id = workspaces.create(name);
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

    server.Post("/api/workspaces/active", [&](const httplib::Request &req, httplib::Response &res) {
        const std::string client_id = read_client_id(req);
        if (client_id.empty()) {
            send_json(res, "{\"error\":\"client_id is required\"}", 400);
            return;
        }
        if (!req.has_param("workspace_id")) {
            send_json(res, "{\"error\":\"workspace_id is required\"}", 400);
            return;
        }
        const std::string workspace_id = req.get_param_value("workspace_id");
        if (!workspaces.set_active(client_id, workspace_id)) {
            send_json(res, "{\"error\":\"workspace not found\"}", 404);
            return;
        }
        send_json(res, "{\"ok\":true}");
    });

    server.Post("/api/workspaces/delete", [&](const httplib::Request &req, httplib::Response &res) {
        const std::string client_id = read_client_id(req);
        if (client_id.empty()) {
            send_json(res, "{\"error\":\"client_id is required\"}", 400);
            return;
        }
        if (!req.has_param("workspace_id")) {
            send_json(res, "{\"error\":\"workspace_id is required\"}", 400);
            return;
        }

        const std::string workspace_id = req.get_param_value("workspace_id");
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

    server.Post("/api/workspaces/scene_draft", [&](const httplib::Request &req, httplib::Response &res) {
        const std::string client_id = read_client_id(req);
        if (client_id.empty()) {
            send_json(res, "{\"error\":\"client_id is required\"}", 400);
            return;
        }
        if (!req.has_param("scene")) {
            send_json(res, "{\"error\":\"scene is required\"}", 400);
            return;
        }
        if (!req.has_param("source")) {
            send_json(res, "{\"error\":\"source is required\"}", 400);
            return;
        }
        std::string workspace_id;
        workspaces.get_active(client_id, workspace_id);
        const std::string scene = req.get_param_value("scene");
        if (!is_scene_name_safe(scene)) {
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }
        if (!workspaces.set_scene_draft(workspace_id, scene, req.get_param_value("source"))) {
            send_json(res, "{\"error\":\"workspace not found\"}", 404);
            return;
        }
        send_json(res, "{\"ok\":true}");
    });

    server.Post("/api/workspaces/settings", [&](const httplib::Request &req, httplib::Response &res) {
        const std::string client_id = read_client_id(req);
        if (client_id.empty()) {
            send_json(res, "{\"error\":\"client_id is required\"}", 400);
            return;
        }
        if (!req.has_param("settings_json")) {
            send_json(res, "{\"error\":\"settings_json is required\"}", 400);
            return;
        }

        std::string workspace_id;
        workspaces.get_active(client_id, workspace_id);
        const std::string settings_json = req.get_param_value("settings_json");
        if (settings_json.size() > 65536) {
            send_json(res, "{\"error\":\"settings_json too large\"}", 400);
            return;
        }
        if (!workspaces.set_settings_json(workspace_id, settings_json)) {
            send_json(res, "{\"error\":\"workspace not found\"}", 404);
            return;
        }
        send_json(res, "{\"ok\":true}");
    });

    server.Get("/api/scenes", [scene_dir](const httplib::Request &, httplib::Response &res) {
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

    server.Get("/api/logs", [](const httplib::Request &req, httplib::Response &res) {
        unsigned long long since = 0;
        if (req.has_param("since")) {
            std::istringstream ss(req.get_param_value("since"));
            ss >> since;
            if (ss.fail()) {
                send_json(res, "{\"error\":\"invalid since\"}", 400);
                return;
            }
        }

        std::vector<backend_log_entry_t> list = backend_log_t::handle().since(since);
        send_json(res, backend_logs_to_json(list));
    });

    server.Get("/api/logs/wait", [](const httplib::Request &req, httplib::Response &res) {
        unsigned long long since = 0;
        if (req.has_param("since")) {
            std::istringstream ss(req.get_param_value("since"));
            ss >> since;
            if (ss.fail()) {
                send_json(res, "{\"error\":\"invalid since\"}", 400);
                return;
            }
        }

        unsigned long timeout_ms = 15000;
        if (req.has_param("timeout_ms")) {
            std::istringstream ss(req.get_param_value("timeout_ms"));
            unsigned long parsed = 0;
            ss >> parsed;
            if (ss.fail()) {
                send_json(res, "{\"error\":\"invalid timeout_ms\"}", 400);
                return;
            }
            if (parsed < 1000) parsed = 1000;
            if (parsed > 60000) parsed = 60000;
            timeout_ms = parsed;
        }

        std::vector<backend_log_entry_t> list = backend_log_t::handle().wait_since(since, timeout_ms);
        send_json(res, backend_logs_to_json(list));
    });

    server.Get(R"(/api/scenes/load_jobs/([0-9]+))", [](const httplib::Request &req, httplib::Response &res) {
        unsigned long long id = 0ULL;
        std::istringstream ss(req.matches[1].str());
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

    server.Get(R"(/api/scenes/([A-Za-z0-9_.-]+)/cameras)", [scene_dir](const httplib::Request &req, httplib::Response &res) {
        std::string scene = req.matches[1];
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "camera list rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }
        std::string variant;
        std::string variant_error;
        if (!read_variant_name(req, variant, variant_error)) {
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

    server.Get(R"(/api/scenes/([A-Za-z0-9_.-]+)/source)", [scene_dir, &workspaces](const httplib::Request &req, httplib::Response &res) {
        std::string scene = req.matches[1];
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "source read rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }

        std::string source;
        std::string source_origin = "disk";
        const std::string client_id = read_client_id(req);
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

    server.Get(R"(/api/scenes/([A-Za-z0-9_.-]+)/geometry)", [scene_dir](const httplib::Request &req, httplib::Response &res) {
        std::string scene = req.matches[1];
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "geometry rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }
        std::string variant;
        std::string variant_error;
        if (!read_variant_name(req, variant, variant_error)) {
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

    server.Get(R"(/api/scenes/([A-Za-z0-9_.-]+)/runtime_graph)", [scene_dir](const httplib::Request &req, httplib::Response &res) {
        std::string scene = req.matches[1];
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "runtime_graph rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }
        std::string variant;
        std::string variant_error;
        if (!read_variant_name(req, variant, variant_error)) {
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

    server.Get(R"(/api/scenes/([A-Za-z0-9_.-]+)/camera_resolve)", [scene_dir](const httplib::Request &req, httplib::Response &res) {
        std::string scene = req.matches[1];
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "camera_resolve rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }

        std::string requested_camera;
        if (req.has_param("camera")) requested_camera = req.get_param_value("camera");
        std::string variant;
        std::string variant_error;
        if (!read_variant_name(req, variant, variant_error)) {
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

    server.Get(R"(/api/scenes/([A-Za-z0-9_.-]+)/asset)", [scene_dir](const httplib::Request &req, httplib::Response &res) {
        std::string scene = req.matches[1];
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "asset read rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }
        if (!req.has_param("path")) {
            send_json(res, "{\"error\":\"path is required\"}", 400);
            return;
        }

        std::string relpath = req.get_param_value("path");
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

        res.set_content(content.data(), content.size(), "text/plain; charset=utf-8");
    });

    server.Get("/api/scenes/template/empty", [](const httplib::Request &, httplib::Response &res) {
        std::ostringstream ss;
        ss << "{\"source\":\"" << json_escape(xtcore::get_empty_scene_template()) << "\"}";
        send_json(res, ss.str());
    });

    server.Post("/api/scenes/save", [scene_dir, &workspaces](const httplib::Request &req, httplib::Response &res) {
        if (!req.has_param("name")) {
            backend_log_t::handle().add("warn", "scene save rejected: name missing");
            send_json(res, "{\"error\":\"name is required\"}", 400);
            return;
        }
        if (!req.has_param("source")) {
            backend_log_t::handle().add("warn", "scene save rejected: source missing");
            send_json(res, "{\"error\":\"source is required\"}", 400);
            return;
        }

        std::string input_name = req.get_param_value("name");
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
        bool overwrite = req.has_param("overwrite") && req.get_param_value("overwrite") == "1";
        if (!overwrite && file_exists(scene_path)) {
            backend_log_t::handle().add("warn", "scene save conflict scene=" + scene_name);
            send_json(res, "{\"error\":\"scene already exists\"}", 409);
            return;
        }

        std::string source = req.get_param_value("source");
        if (!write_text_file(scene_path, source)) {
            backend_log_t::handle().add("error", "scene save failed scene=" + scene_name);
            send_json(res, "{\"error\":\"failed to write scene\"}", 500);
            return;
        }
        scene_cache_t::handle().invalidate(scene_path);
        const std::string client_id = read_client_id(req);
        if (!client_id.empty()) {
            std::string workspace_id;
            workspaces.get_active(client_id, workspace_id);
            workspaces.set_scene_draft(workspace_id, scene_name, source);
            workspaces.set_active_scene(workspace_id, scene_name);
        }
        backend_log_t::handle().add("info", "scene saved scene=" + scene_name);

        std::ostringstream ss;
        ss << "{\"scene\":\"" << json_escape(scene_name) << "\"}";
        send_json(res, ss.str());
    });

    server.Post("/api/scenes/delete", [scene_dir](const httplib::Request &req, httplib::Response &res) {
        if (!req.has_param("name")) {
            backend_log_t::handle().add("warn", "scene delete rejected: name missing");
            send_json(res, "{\"error\":\"name is required\"}", 400);
            return;
        }

        std::string scene_name = req.get_param_value("name");
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

    server.Get("/api/integrators", [](const httplib::Request &, httplib::Response &res) {
        std::vector<common::integrator_info_t> list = common::list_integrators();
        std::ostringstream ss;
        ss << "{\"integrators\":[";
        for (size_t i = 0; i < list.size(); ++i) {
            if (i) ss << ',';
            ss << "{\"id\":\"" << json_escape(list[i].id)
               << "\",\"label\":\"" << json_escape(list[i].label) << "\",";
            append_integrator_controls_json(ss, list[i]);
            ss << "}";
        }
        ss << "]}";
        send_json(res, ss.str());
    });

    server.Get("/api/resolutions", [](const httplib::Request &, httplib::Response &res) {
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

    server.Post("/api/render", [&](const httplib::Request &req, httplib::Response &res) {
        if (!req.has_param("scene")) {
            backend_log_t::handle().add("warn", "render rejected: missing scene");
            send_json(res, "{\"error\":\"scene is required\"}", 400);
            return;
        }

        std::string scene = req.get_param_value("scene");
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "render rejected: invalid scene");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }

        common::render_request_t rr;
        rr.scene_path = join_path(scene_dir, scene);
        std::string variant_error;
        if (!read_variant_name(req, rr.variant, variant_error)) {
            backend_log_t::handle().add("warn", "render rejected: invalid variant");
            send_json(res, "{\"error\":\"invalid variant\"}", 400);
            return;
        }

        std::string workspace_id;
        if (req.has_param("workspace_id")) {
            workspace_id = req.get_param_value("workspace_id");
        }
        if (workspace_id.empty()) {
            const std::string client_id = read_client_id(req);
            if (!client_id.empty()) {
                workspaces.get_active(client_id, workspace_id);
            }
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

        if (req.has_param("integrator")) rr.integrator = req.get_param_value("integrator");
        if (!common::is_integrator_supported(rr.integrator)) {
            backend_log_t::handle().add("warn", "render rejected: unsupported integrator=" + rr.integrator);
            send_json(res, "{\"error\":\"integrator not supported\"}", 400);
            return;
        }
        if (!parse_render_mode_param(req, "render_mode", rr.render_mode) && req.has_param("render_mode")) {
            backend_log_t::handle().add("warn", "render rejected: invalid render_mode");
            send_json(res, "{\"error\":\"invalid render_mode\"}", 400);
            return;
        }
        for (auto it = req.params.begin(); it != req.params.end(); ++it) {
            if (!has_prefix((*it).first, "iopt.")) continue;
            const std::string key = (*it).first.substr(5);
            if (key.empty()) continue;
            rr.integrator_options[key] = (*it).second;
        }
        std::string integrator_opt_error;
        if (!common::validate_integrator_options(rr.integrator, rr.integrator_options, integrator_opt_error)) {
            backend_log_t::handle().add("warn", "render rejected: " + integrator_opt_error);
            send_json(res, "{\"error\":\"" + json_escape(integrator_opt_error) + "\"}", 400);
            return;
        }

        if (req.has_param("camera")) rr.camera = req.get_param_value("camera");

        const bool has_cam_override =
            req.has_param("cam_px") || req.has_param("cam_py") || req.has_param("cam_pz")
            || req.has_param("cam_tx") || req.has_param("cam_ty") || req.has_param("cam_tz")
            || req.has_param("cam_upx") || req.has_param("cam_upy") || req.has_param("cam_upz")
            || req.has_param("cam_hfov");
        if (has_cam_override) {
            double cam_px = 0.0, cam_py = 0.0, cam_pz = 0.0;
            double cam_tx = 0.0, cam_ty = 0.0, cam_tz = 0.0;
            double cam_upx = 0.0, cam_upy = 1.0, cam_upz = 0.0;
            double cam_hfov = 60.0;
            const bool ok =
                parse_f64_param(req, "cam_px", -1e9, 1e9, cam_px)
                && parse_f64_param(req, "cam_py", -1e9, 1e9, cam_py)
                && parse_f64_param(req, "cam_pz", -1e9, 1e9, cam_pz)
                && parse_f64_param(req, "cam_tx", -1e9, 1e9, cam_tx)
                && parse_f64_param(req, "cam_ty", -1e9, 1e9, cam_ty)
                && parse_f64_param(req, "cam_tz", -1e9, 1e9, cam_tz)
                && parse_f64_param(req, "cam_upx", -1e6, 1e6, cam_upx)
                && parse_f64_param(req, "cam_upy", -1e6, 1e6, cam_upy)
                && parse_f64_param(req, "cam_upz", -1e6, 1e6, cam_upz)
                && parse_f64_param(req, "cam_hfov", 1.0, 179.0, cam_hfov);
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
        if (parse_u64_param(req, "width", 8, 8192, v)) rr.width = v;
        else if (req.has_param("width")) {
            backend_log_t::handle().add("warn", "render rejected: invalid width");
            send_json(res, "{\"error\":\"invalid width\"}", 400);
            return;
        }
        if (parse_u64_param(req, "height", 8, 8192, v)) rr.height = v;
        else if (req.has_param("height")) {
            backend_log_t::handle().add("warn", "render rejected: invalid height");
            send_json(res, "{\"error\":\"invalid height\"}", 400);
            return;
        }
        if (parse_u64_param(req, "samples", 1, 1024, v)) rr.samples = v;
        else if (req.has_param("samples")) {
            backend_log_t::handle().add("warn", "render rejected: invalid samples");
            send_json(res, "{\"error\":\"invalid samples\"}", 400);
            return;
        }
        if (parse_u64_param(req, "aa", 1, 16, v)) rr.aa = v;
        else if (req.has_param("aa")) {
            backend_log_t::handle().add("warn", "render rejected: invalid aa");
            send_json(res, "{\"error\":\"invalid aa\"}", 400);
            return;
        }
        if (!parse_sample_distribution_param(req, "sample_distribution", rr.sample_distribution)
            && req.has_param("sample_distribution")) {
            backend_log_t::handle().add("warn", "render rejected: invalid sample_distribution");
            send_json(res, "{\"error\":\"invalid sample_distribution\"}", 400);
            return;
        }
        if (parse_u64_param(req, "rdepth", 1, 4096, v)) rr.rdepth = v;
        else if (req.has_param("rdepth")) {
            backend_log_t::handle().add("warn", "render rejected: invalid rdepth");
            send_json(res, "{\"error\":\"invalid rdepth\"}", 400);
            return;
        }
        if (parse_u64_param(req, "tile_size", 8, 1024, v)) rr.tile_size = v;
        else if (req.has_param("tile_size")) {
            backend_log_t::handle().add("warn", "render rejected: invalid tile_size");
            send_json(res, "{\"error\":\"invalid tile_size\"}", 400);
            return;
        }
        if (parse_u64_param(req, "threads", 0, 256, v)) rr.threads = v;
        else if (req.has_param("threads")) {
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
        if (!parse_tile_order_param(req, "tile_order", rr.tile_order) && req.has_param("tile_order")) {
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

        std::string job_id = jobs.create(rr, scene, workspace_id, cleanup_scene_path);
        workspaces.mark_job_started(workspace_id, job_id);
        std::ostringstream ss;
        ss << "{"
           << "\"job_id\":\"" << json_escape(job_id) << "\","
           << "\"workspace_id\":\"" << json_escape(workspace_id) << "\""
           << "}";
        send_json(res, ss.str(), 202);
    });

    server.Get(R"(/api/jobs/([A-Za-z0-9_]+)/image)", [&](const httplib::Request &req, httplib::Response &res) {
        std::string id = req.matches[1];
        bool final_only = req.has_param("final") && req.get_param_value("final") == "1";
        xtcore::tonemapping::settings_t tm_settings;
        std::string tm_error_json;
        if (!parse_tonemapping_settings(req, tm_settings, tm_error_json)) {
            send_json(res, tm_error_json, 400);
            return;
        }
        std::vector<unsigned char> image;
        if (!jobs.image(id, image, !final_only, tm_settings)) {
            backend_log_t::handle().add("warn", "job image missing id=" + id);
            send_json(res, "{\"error\":\"image not available\"}", 404);
            return;
        }
        res.set_content((const char *)image.data(), image.size(), "image/png");
    });

    server.Get(R"(/api/jobs/([A-Za-z0-9_]+)/image_delta)", [&](const httplib::Request &req, httplib::Response &res) {
        std::string id = req.matches[1];
        size_t since_done = 0;
        size_t parsed = 0;
        if (parse_u64_param(req, "since", 0, 1000000000, parsed)) {
            since_done = parsed;
        } else if (req.has_param("since")) {
            send_json(res, "{\"error\":\"invalid since\"}", 400);
            return;
        }

        size_t max_tiles = 16;
        if (parse_u64_param(req, "limit", 1, 256, parsed)) {
            max_tiles = parsed;
        } else if (req.has_param("limit")) {
            send_json(res, "{\"error\":\"invalid limit\"}", 400);
            return;
        }

        xtcore::tonemapping::settings_t tm_settings;
        std::string tm_error_json;
        if (!parse_tonemapping_settings(req, tm_settings, tm_error_json)) {
            send_json(res, tm_error_json, 400);
            return;
        }

        job_image_delta_t delta;
        if (!jobs.image_delta(id, since_done, max_tiles, tm_settings, delta)) {
            send_json(res, "{\"error\":\"image delta not available\"}", 404);
            return;
        }

        std::vector<unsigned char> payload;
        payload.reserve(64);
        payload.push_back('X');
        payload.push_back('T');
        payload.push_back('D');
        payload.push_back('1');
        append_u32le(payload, (uint32_t)delta.width);
        append_u32le(payload, (uint32_t)delta.height);
        append_u32le(payload, (uint32_t)delta.tiles_done);
        append_u32le(payload, (uint32_t)delta.tiles_total);
        append_u32le(payload, (uint32_t)delta.state);
        append_u32le(payload, (uint32_t)delta.tiles.size());
        for (size_t i = 0; i < delta.tiles.size(); ++i) {
            const job_image_delta_t::tile_t &t = delta.tiles[i];
            append_u32le(payload, (uint32_t)t.x0);
            append_u32le(payload, (uint32_t)t.y0);
            append_u32le(payload, (uint32_t)t.x1);
            append_u32le(payload, (uint32_t)t.y1);
            append_u32le(payload, (uint32_t)t.done_index);
            append_u32le(payload, (uint32_t)t.png.size());
            payload.insert(payload.end(), t.png.begin(), t.png.end());
        }
        res.set_header("Cache-Control", "no-store");
        res.set_content((const char *)payload.data(), payload.size(), "application/octet-stream");
    });

    server.Get(R"(/api/jobs/([A-Za-z0-9_]+)/export)", [&](const httplib::Request &req, httplib::Response &res) {
        std::string id = req.matches[1];
        std::string format = "png";
        if (req.has_param("format")) format = lower_ascii(req.get_param_value("format"));
        if (format != "png" && format != "exr" && format != "hdr"
            && format != "jpg" && format != "bmp" && format != "tga"
            && format != "ply") {
            send_json(res, "{\"error\":\"unsupported format\"}", 400);
            return;
        }

        std::vector<unsigned char> image;
        std::string mime_type;
        std::string extension;
        if (!jobs.image_export(id, format, image, mime_type, extension)) {
            backend_log_t::handle().add("warn", "job export unavailable id=" + id + " format=" + format);
            send_json(res, "{\"error\":\"export not available\"}", 404);
            return;
        }

        const std::string filename = "xtracer_" + id + "_" + utc_timestamp_for_filename() + "." + extension;
        const std::string content_disposition = "attachment; filename=\"" + filename + "\"";
        res.set_header("Cache-Control", "no-store");
        res.set_header("Content-Disposition", content_disposition.c_str());
        res.set_content((const char *)image.data(), image.size(), mime_type.c_str());
    });

    server.Get(R"(/api/jobs/([A-Za-z0-9_]+)/photons)", [&](const httplib::Request &req, httplib::Response &res) {
        std::string id = req.matches[1];
        size_t limit = 100000;
        size_t v = 0;
        if (parse_u64_param(req, "limit", 1, 500000, v)) limit = v;
        else if (req.has_param("limit")) {
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

    server.Get("/api/jobs/active", [&](const httplib::Request &, httplib::Response &res) {
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

    server.Post(R"(/api/jobs/abort/([A-Za-z0-9_]+))", [&](const httplib::Request &req, httplib::Response &res) {
        std::string id = req.matches[1];
        if (!jobs.abort(id)) {
            send_json(res, "{\"error\":\"job not found\"}", 404);
            return;
        }
        backend_log_t::handle().add("info", "job abort requested id=" + id);
        send_json(res, "{\"ok\":true}");
    });

    server.Post(R"(/api/jobs/queue/up/([A-Za-z0-9_]+))", [&](const httplib::Request &req, httplib::Response &res) {
        std::string id = req.matches[1];
        if (!jobs.move_queue_up(id)) {
            send_json(res, "{\"error\":\"job not found or not queued\"}", 404);
            return;
        }
        backend_log_t::handle().add("info", "job queue move up id=" + id);
        send_json(res, "{\"ok\":true}");
    });

    server.Post(R"(/api/jobs/queue/down/([A-Za-z0-9_]+))", [&](const httplib::Request &req, httplib::Response &res) {
        std::string id = req.matches[1];
        if (!jobs.move_queue_down(id)) {
            send_json(res, "{\"error\":\"job not found or not queued\"}", 404);
            return;
        }
        backend_log_t::handle().add("info", "job queue move down id=" + id);
        send_json(res, "{\"ok\":true}");
    });

    server.Get(R"(/api/jobs/([A-Za-z0-9_]+))", [&](const httplib::Request &req, httplib::Response &res) {
        std::string id = req.matches[1];
        job_snapshot_t snap;
        if (!jobs.snapshot(id, snap)) {
            backend_log_t::handle().add("warn", "job lookup failed id=" + id);
            send_json(res, "{\"error\":\"job not found\"}", 404);
            return;
        }
        if (snap.state == JOB_DONE || snap.state == JOB_ABORTED || snap.state == JOB_ERROR) {
            workspaces.mark_job_finished(snap.workspace_id, snap.id);
        }

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
        send_json(res, ss.str());
    });

    server.Get("/", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "index.html"), "text/html", res);
    });

    server.Get("/app.js", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "app.js"), "application/javascript", res);
    });

    server.Get(R"(/app/([A-Za-z0-9_.-]+\.js))", [web_root](const httplib::Request &req, httplib::Response &res) {
        const std::string name = req.matches[1];
        serve_static_file(join_path(web_root, "app/" + name), "application/javascript", res);
    });

    server.Get("/visual_editor.js", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "visual_editor.js"), "application/javascript", res);
    });

    server.Get("/vendor/three.min.js", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "vendor/three.min.js"), "application/javascript", res);
    });

    server.Get("/wasm_adapter.js", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "wasm_adapter.js"), "application/javascript", res);
    });

    server.Get("/wasm_worker.js", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "wasm_worker.js"), "application/javascript", res);
    });

    server.Get("/xtracer_wasm.js", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "xtracer_wasm.js"), "application/javascript", res);
    });

    server.Get("/xtracer_wasm.wasm", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "xtracer_wasm.wasm"), "application/wasm", res);
    });

    server.Get(R"(/app/data/([A-Za-z0-9_.-]+\.json))", [web_root](const httplib::Request &req, httplib::Response &res) {
        const std::string name = req.matches[1];
        serve_static_file(join_path(web_root, "app/data/" + name), "application/json", res);
    });

    server.Get(R"(/scenes/([A-Za-z0-9_.-]+\.scn))", [web_root](const httplib::Request &req, httplib::Response &res) {
        std::string scene = req.matches[1];
        serve_static_file(join_path(web_root, "scenes/" + scene), "text/plain", res);
    });

    server.Get("/styles.css", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "styles.css"), "text/css", res);
    });

    server.Get("/preview.jpg", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "preview.jpg"), "image/jpeg", res);
    });

    server.Get("/logo.png", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "logo.png"), "image/png", res);
    });

    server.Get("/logo.svg", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "logo.svg"), "image/svg+xml", res);
    });

    server.Get(R"(/res/([A-Za-z0-9_.-]+\.png))", [web_root](const httplib::Request &req, httplib::Response &res) {
        const std::string name = req.matches[1];
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

    server.Get("/license.txt", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "license.txt"), "text/plain; charset=utf-8", res);
    });
}

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */
