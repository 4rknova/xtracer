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

#include <cpp-httplib/httplib.h>
#include <xtcore/parseutil.h>
#include <xtcore/resolution_preset.h>
#include <xtcore/strpool.h>
#include <xtcore/xtcore.h>

#include "backend_log.h"
#include "job_manager.h"

namespace xtracer {
namespace frontend {
namespace web {

namespace {

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

std::vector<std::string> list_cameras(const std::string &scene_path, std::string &error)
{
    std::vector<std::string> out;
    xtcore::Scene scene;
    int load_err = xtcore::io::scn::load(&scene, scene_path.c_str(), nullptr);
    if (load_err) {
        error = "failed to load scene";
        return out;
    }

    for (auto it = scene.m_cameras.begin(); it != scene.m_cameras.end(); ++it) {
        const char *name = xtcore::pool::str::get((*it).first);
        if (name && *name) out.push_back(name);
    }

    std::sort(out.begin(), out.end());
    return out;
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
                  const std::string &scene_dir,
                  const std::string &web_root)
{
    backend_log_t::handle().add("info", "web routes initialized");

    server.Get("/api/health", [](const httplib::Request &, httplib::Response &res) {
        send_json(res, "{\"ok\":true}");
    });

    server.Get("/api/about", [](const httplib::Request &, httplib::Response &res) {
        std::time_t now = std::time(nullptr);
        std::tm *utc = std::gmtime(&now);
        int year = utc ? (utc->tm_year + 1900) : 2010;
        if (year < 2010) year = 2010;

        std::ostringstream ss;
        ss << "{"
           << "\"name\":\"XTRACER WEB\","
           << "\"version\":\"" << json_escape(xtcore::get_version()) << "\","
           << "\"author_name\":\"Nikos Papadopoulos\","
           << "\"author_email\":\"nikpapas@gmail.com\","
           << "\"homepage\":\"https://www.4rknova.com\","
           << "\"website\":\"https://github.com/4rknova/xtracer\","
           << "\"copyright\":\"Copyright 2010-" << year << " (c) Nikos Papadopoulos\","
           << "\"license\":\"" << json_escape(xtcore::get_license()) << "\""
           << "}";
        send_json(res, ss.str());
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
        send_json(res, out.str());
    });

    server.Get(R"(/api/scenes/([A-Za-z0-9_.-]+)/cameras)", [scene_dir](const httplib::Request &req, httplib::Response &res) {
        std::string scene = req.matches[1];
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "camera list rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }

        std::string error;
        std::vector<std::string> cameras = list_cameras(join_path(scene_dir, scene), error);
        if (!error.empty()) {
            backend_log_t::handle().add("error", "camera list failed for scene=" + scene);
            send_json(res, "{\"error\":\"failed to load scene\"}", 400);
            return;
        }

        std::ostringstream ss;
        ss << "{\"cameras\":[";
        for (size_t i = 0; i < cameras.size(); ++i) {
            if (i) ss << ',';
            ss << '"' << json_escape(cameras[i]) << '"';
        }
        ss << "]}";
        send_json(res, ss.str());
    });

    server.Get(R"(/api/scenes/([A-Za-z0-9_.-]+)/source)", [scene_dir](const httplib::Request &req, httplib::Response &res) {
        std::string scene = req.matches[1];
        if (!is_scene_name_safe(scene)) {
            backend_log_t::handle().add("warn", "source read rejected: invalid scene name");
            send_json(res, "{\"error\":\"invalid scene\"}", 400);
            return;
        }

        std::string source;
        if (!read_text_file(join_path(scene_dir, scene), source)) {
            backend_log_t::handle().add("error", "source read failed scene=" + scene);
            send_json(res, "{\"error\":\"failed to read scene\"}", 404);
            return;
        }

        std::ostringstream ss;
        ss << "{"
           << "\"scene\":\"" << json_escape(scene) << "\","
           << "\"source\":\"" << json_escape(source) << "\""
           << "}";
        send_json(res, ss.str());
    });

    server.Get("/api/scenes/template/empty", [](const httplib::Request &, httplib::Response &res) {
        std::ostringstream ss;
        ss << "{\"source\":\"" << json_escape(xtcore::get_empty_scene_template()) << "\"}";
        send_json(res, ss.str());
    });

    server.Post("/api/scenes/save", [scene_dir](const httplib::Request &req, httplib::Response &res) {
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

        std::string job_id = jobs.create(rr, scene);
        std::ostringstream ss;
        ss << "{\"job_id\":\"" << json_escape(job_id) << "\"}";
        send_json(res, ss.str(), 202);
    });

    server.Get(R"(/api/jobs/([A-Za-z0-9_]+)/image)", [&](const httplib::Request &req, httplib::Response &res) {
        std::string id = req.matches[1];
        bool final_only = req.has_param("final") && req.get_param_value("final") == "1";
        std::vector<unsigned char> image;
        if (!jobs.image(id, image, !final_only)) {
            backend_log_t::handle().add("warn", "job image missing id=" + id);
            send_json(res, "{\"error\":\"image not available\"}", 404);
            return;
        }
        res.set_content((const char *)image.data(), image.size(), "image/png");
    });

    server.Get(R"(/api/jobs/([A-Za-z0-9_]+))", [&](const httplib::Request &req, httplib::Response &res) {
        std::string id = req.matches[1];
        job_snapshot_t snap;
        if (!jobs.snapshot(id, snap)) {
            backend_log_t::handle().add("warn", "job lookup failed id=" + id);
            send_json(res, "{\"error\":\"job not found\"}", 404);
            return;
        }

        std::ostringstream ss;
        ss << "{"
           << "\"id\":\"" << json_escape(snap.id) << "\","
           << "\"scene\":\"" << json_escape(snap.scene) << "\","
           << "\"integrator\":\"" << json_escape(snap.integrator) << "\","
           << "\"state\":\"" << job_state_name(snap.state) << "\","
           << "\"progress\":" << snap.progress << ","
           << "\"elapsed_ms\":" << snap.elapsed_ms << ","
           << "\"has_image\":" << (snap.has_image ? "true" : "false") << ","
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

    server.Get("/resolutions.json", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "resolutions.json"), "application/json", res);
    });

    server.Get("/scenes/index.json", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "scenes/index.json"), "application/json", res);
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

    server.Get("/license.txt", [web_root](const httplib::Request &, httplib::Response &res) {
        serve_static_file(join_path(web_root, "license.txt"), "text/plain; charset=utf-8", res);
    });
}

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */
