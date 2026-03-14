#include "routes.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <iterator>
#include <ctime>
#include <vector>
#include <thread>
#include <unistd.h>
#if defined(_OPENMP)
#include <omp.h>
#endif

#include <cpp-httplib/httplib.h>
#include <xtcore/camera.h>
#include <xtcore/math/triangle.h>
#include <xtcore/mesh.h>
#include <xtcore/parseutil.h>
#include <xtcore/resolution_preset.h>
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
    std::vector<std::string> cameras;
    std::string default_camera;
};

HASH_ID find_camera_id_by_name(const xtcore::Scene &scene, const std::string &name)
{
    if (name.empty()) return HASH_ID_INVALID;
    for (auto it = scene.m_cameras.begin(); it != scene.m_cameras.end(); ++it) {
        const char *camera_name = xtcore::pool::str::get((*it).first);
        if (camera_name && name == camera_name) return (*it).first;
    }
    return HASH_ID_INVALID;
}

camera_list_info_t list_cameras(const std::string &scene_path, std::string &error)
{
    camera_list_info_t out;
    xtcore::Scene scene;
    int load_err = xtcore::io::scn::load(&scene, scene_path.c_str(), nullptr);
    if (load_err) {
        error = "failed to load scene";
        return out;
    }

    for (auto it = scene.m_cameras.begin(); it != scene.m_cameras.end(); ++it) {
        const char *name = xtcore::pool::str::get((*it).first);
        if (name && *name) out.cameras.push_back(name);
    }

    std::sort(out.cameras.begin(), out.cameras.end());
    out.default_camera = scene.m_default_camera;
    return out;
}

std::string scene_geometry_json(const std::string &scene_path, std::string &error)
{
    xtcore::Scene scene;
    int load_err = xtcore::io::scn::load(&scene, scene_path.c_str(), nullptr);
    if (load_err) {
        error = "failed to load scene";
        return std::string();
    }

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
        ss << "]}";
    }

    ss << "}}";
    return ss.str();
}

std::string scene_resolved_camera_json(const std::string &scene_path,
                                       const std::string &requested_camera,
                                       std::string &error)
{
    xtcore::Scene scene;
    int load_err = xtcore::io::scn::load(&scene, scene_path.c_str(), nullptr);
    if (load_err) {
        error = "failed to load scene";
        return std::string();
    }

    HASH_ID resolved_id = HASH_ID_INVALID;
    HASH_ID requested_id = HASH_ID_INVALID;
    bool release_requested_id = false;

    if (!requested_camera.empty()) {
        requested_id = xtcore::pool::str::add(requested_camera.c_str());
        release_requested_id = true;
        if (scene.get_camera(requested_id)) resolved_id = requested_id;
    }
    if (resolved_id == HASH_ID_INVALID) {
        resolved_id = find_camera_id_by_name(scene, scene.m_default_camera);
        if (resolved_id == HASH_ID_INVALID) {
            auto first_cam = scene.m_cameras.begin();
            if (first_cam != scene.m_cameras.end()) resolved_id = (*first_cam).first;
        }
    }

    if (resolved_id == HASH_ID_INVALID || !scene.get_camera(resolved_id)) {
        if (release_requested_id) xtcore::pool::str::del(requested_id);
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

    if (release_requested_id) xtcore::pool::str::del(requested_id);
    return ss.str();
}

const char *job_state_name(job_state_t state)
{
    switch (state) {
        case JOB_QUEUED:  return "queued";
        case JOB_RUNNING: return "running";
        case JOB_DONE:    return "done";
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
                  const std::string &web_root)
{
    backend_log_t::handle().add("info", "web routes initialized");

    server.Get("/api/health", [](const httplib::Request &, httplib::Response &res) {
        send_json(res, "{\"ok\":true}");
    });

    server.Get("/api/about", [&jobs](const httplib::Request &, httplib::Response &res) {
        std::time_t now = std::time(nullptr);
        std::tm *utc = std::gmtime(&now);
        int year = utc ? (utc->tm_year + 1900) : 2010;
        if (year < 2010) year = 2010;
        const unsigned int logical_cores_raw = std::thread::hardware_concurrency();
        const size_t logical_cores = (logical_cores_raw == 0) ? 1 : static_cast<size_t>(logical_cores_raw);
        const size_t openmp_max_threads = runtime_omp_max_threads();
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

        std::ostringstream ss;
        ss << "{"
           << "\"active_workspace\":\"" << json_escape(active_workspace) << "\","
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

    server.Get(R"(/api/scenes/([A-Za-z0-9_.-]+)/cameras)", [scene_dir](const httplib::Request &req, httplib::Response &res) {
        std::string scene = req.matches[1];
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "camera list rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }

        std::string error;
        camera_list_info_t cameras = list_cameras(join_path(scene_dir, scene), error);
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

        std::string error;
        std::string payload = scene_geometry_json(join_path(scene_dir, scene), error);
        if (!error.empty()) {
            backend_log_t::handle().add("error", "geometry export failed for scene=" + scene);
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

        std::string error;
        std::string payload = scene_resolved_camera_json(join_path(scene_dir, scene), requested_camera, error);
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

        size_t v = 0;
        if (parse_u64_param(req, "width", 32, 8192, v)) rr.width = v;
        else if (req.has_param("width")) {
            backend_log_t::handle().add("warn", "render rejected: invalid width");
            send_json(res, "{\"error\":\"invalid width\"}", 400);
            return;
        }
        if (parse_u64_param(req, "height", 32, 8192, v)) rr.height = v;
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
        if (!parse_tile_order_param(req, "tile_order", rr.tile_order) && req.has_param("tile_order")) {
            backend_log_t::handle().add("warn", "render rejected: invalid tile_order");
            send_json(res, "{\"error\":\"invalid tile_order\"}", 400);
            return;
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
        if (req.has_param("tm")) {
            const std::string tm = lower_ascii(req.get_param_value("tm"));
            if (!parse_tonemapping_operator(tm, tm_settings.op)) {
                send_json(res, "{\"error\":\"invalid tone mapping operator\"}", 400);
                return;
            }
        }
        if (req.has_param("tm_exposure")) {
            std::istringstream es(req.get_param_value("tm_exposure"));
            float exposure = 1.0f;
            es >> exposure;
            if (es.fail() || exposure <= 0.0f) {
                send_json(res, "{\"error\":\"invalid tone mapping exposure\"}", 400);
                return;
            }
            tm_settings.exposure = exposure;
        }
        if (req.has_param("tm_white_point")) {
            std::istringstream ws(req.get_param_value("tm_white_point"));
            float white_point = 1.0f;
            ws >> white_point;
            if (ws.fail() || white_point <= 0.0f) {
                send_json(res, "{\"error\":\"invalid tone mapping white point\"}", 400);
                return;
            }
            tm_settings.white_point = white_point;
        }
        if (req.has_param("tm_mantiuk_contrast")) {
            std::istringstream cs(req.get_param_value("tm_mantiuk_contrast"));
            float v = 0.1f;
            cs >> v;
            if (cs.fail() || v < 0.0f || v > 1.0f) {
                send_json(res, "{\"error\":\"invalid mantiuk contrast\"}", 400);
                return;
            }
            tm_settings.mantiuk_contrast = v;
        }
        if (req.has_param("tm_mantiuk_saturation")) {
            std::istringstream ss(req.get_param_value("tm_mantiuk_saturation"));
            float v = 0.8f;
            ss >> v;
            if (ss.fail() || v < 0.0f || v > 2.0f) {
                send_json(res, "{\"error\":\"invalid mantiuk saturation\"}", 400);
                return;
            }
            tm_settings.mantiuk_saturation = v;
        }
        if (req.has_param("tm_mantiuk_detail")) {
            std::istringstream ds(req.get_param_value("tm_mantiuk_detail"));
            float v = 1.0f;
            ds >> v;
            if (ds.fail() || v < 1.0f || v > 99.0f) {
                send_json(res, "{\"error\":\"invalid mantiuk detail\"}", 400);
                return;
            }
            tm_settings.mantiuk_detail = v;
        }
        std::vector<unsigned char> image;
        if (!jobs.image(id, image, !final_only, tm_settings)) {
            backend_log_t::handle().add("warn", "job image missing id=" + id);
            send_json(res, "{\"error\":\"image not available\"}", 404);
            return;
        }
        res.set_content((const char *)image.data(), image.size(), "image/png");
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

    server.Get(R"(/api/jobs/([A-Za-z0-9_]+))", [&](const httplib::Request &req, httplib::Response &res) {
        std::string id = req.matches[1];
        job_snapshot_t snap;
        if (!jobs.snapshot(id, snap)) {
            backend_log_t::handle().add("warn", "job lookup failed id=" + id);
            send_json(res, "{\"error\":\"job not found\"}", 404);
            return;
        }
        if (snap.state == JOB_DONE || snap.state == JOB_ERROR) {
            workspaces.mark_job_finished(snap.workspace_id, snap.id);
        }

        std::ostringstream ss;
        ss << "{"
           << "\"id\":\"" << json_escape(snap.id) << "\","
           << "\"workspace_id\":\"" << json_escape(snap.workspace_id) << "\","
           << "\"scene\":\"" << json_escape(snap.scene) << "\","
           << "\"integrator\":\"" << json_escape(snap.integrator) << "\","
           << "\"state\":\"" << job_state_name(snap.state) << "\","
           << "\"progress\":" << snap.progress << ","
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

    server.Get("/integrators.json", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "integrators.json"), "application/json", res);
    });

    server.Get("/sidebar_cards.json", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "sidebar_cards.json"), "application/json", res);
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

    server.Get("/license.txt", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "license.txt"), "text/plain; charset=utf-8", res);
    });
}

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */
