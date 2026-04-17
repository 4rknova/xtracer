#include "gallery_manager.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <unistd.h>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>

namespace xtracer {
namespace frontend {
namespace web {

namespace {

bool make_dir(const std::string &path)
{
    if (mkdir(path.c_str(), 0755) == 0) return true;
    return errno == EEXIST;
}

bool remove_dir_recursive(const std::string &path)
{
    DIR *d = opendir(path.c_str());
    if (!d) return false;
    struct dirent *entry;
    while ((entry = readdir(d)) != nullptr) {
        const std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        const std::string child = path + "/" + name;
        struct stat st;
        if (stat(child.c_str(), &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) remove_dir_recursive(child);
        else unlink(child.c_str());
    }
    closedir(d);
    rmdir(path.c_str());
    return true;
}

std::string basename_of(const std::string &path)
{
    size_t pos = path.rfind('/');
    if (pos == std::string::npos) pos = path.rfind('\\');
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

std::string json_str(const std::string &s)
{
    std::string out;
    out.reserve(s.size() + 2);
    out += '"';
    for (unsigned char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20) {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\u%04x", (unsigned)c);
            out += buf;
        }
        else out += (char)c;
    }
    out += '"';
    return out;
}

std::string pass_filename(size_t idx)
{
    std::ostringstream ss;
    ss << "pass_" << std::setw(4) << std::setfill('0') << idx << ".exr";
    return ss.str();
}

std::string meta_to_json(const gallery_entry_meta_t &m)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "{"
       << "\"id\":"          << json_str(m.id) << ","
       << "\"scene\":"       << json_str(m.scene) << ","
       << "\"workspace_id\":" << json_str(m.workspace_id) << ","
       << "\"integrator\":"  << json_str(m.integrator) << ","
       << "\"render_mode\":" << json_str(m.render_mode) << ","
       << "\"width\":"       << m.width << ","
       << "\"height\":"      << m.height << ","
       << "\"samples\":"     << m.samples << ","
       << "\"aa\":"          << m.aa << ","
       << "\"rdepth\":"      << m.rdepth << ","
       << "\"threads\":"     << m.threads << ","
       << "\"tile_size\":"   << m.tile_size << ","
       << "\"elapsed_ms\":"  << m.elapsed_ms << ","
       << "\"created_at_ms\":" << m.created_at_ms << ","
       << "\"pass_count\":"  << m.pass_count << ","
       << "\"tm_op\":"       << json_str(m.tm_op) << ","
       << "\"tm_exposure\":"           << m.tm_exposure << ","
       << "\"tm_white_point\":"        << m.tm_white_point << ","
       << "\"tm_mantiuk_contrast\":"   << m.tm_mantiuk_contrast << ","
       << "\"tm_mantiuk_saturation\":" << m.tm_mantiuk_saturation << ","
       << "\"tm_mantiuk_detail\":"     << m.tm_mantiuk_detail
       << "}";
    return ss.str();
}

gallery_entry_meta_t parse_meta_from_json(const std::string &json)
{
    gallery_entry_meta_t m;

    auto extract_str = [&](const std::string &key) -> std::string {
        const std::string search = "\"" + key + "\":\"";
        const size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        const size_t start = pos + search.size();
        const size_t end = json.find('"', start);
        return (end != std::string::npos) ? json.substr(start, end - start) : "";
    };

    auto extract_num = [&](const std::string &key) -> std::string {
        const std::string search = "\"" + key + "\":";
        const size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        const size_t start = pos + search.size();
        const size_t end = json.find_first_of(",}", start);
        return (end != std::string::npos) ? json.substr(start, end - start) : "";
    };

    m.id          = extract_str("id");
    m.scene       = extract_str("scene");
    m.workspace_id = extract_str("workspace_id");
    m.integrator  = extract_str("integrator");
    m.render_mode = extract_str("render_mode");

    try { const std::string s = extract_num("width");       if (!s.empty()) m.width       = static_cast<size_t>(std::stoul(s)); } catch (...) {}
    try { const std::string s = extract_num("height");      if (!s.empty()) m.height      = static_cast<size_t>(std::stoul(s)); } catch (...) {}
    try { const std::string s = extract_num("samples");     if (!s.empty()) m.samples     = static_cast<size_t>(std::stoul(s)); } catch (...) {}
    try { const std::string s = extract_num("aa");          if (!s.empty()) m.aa          = static_cast<size_t>(std::stoul(s)); } catch (...) {}
    try { const std::string s = extract_num("rdepth");      if (!s.empty()) m.rdepth      = static_cast<size_t>(std::stoul(s)); } catch (...) {}
    try { const std::string s = extract_num("threads");     if (!s.empty()) m.threads     = static_cast<size_t>(std::stoul(s)); } catch (...) {}
    try { const std::string s = extract_num("tile_size");   if (!s.empty()) m.tile_size   = static_cast<size_t>(std::stoul(s)); } catch (...) {}
    try { const std::string s = extract_num("elapsed_ms");  if (!s.empty()) m.elapsed_ms  = std::stod(s); } catch (...) {}
    try { const std::string s = extract_num("created_at_ms"); if (!s.empty()) m.created_at_ms = std::stoll(s); } catch (...) {}
    try { const std::string s = extract_num("pass_count");  if (!s.empty()) m.pass_count  = static_cast<size_t>(std::stoul(s)); } catch (...) {}

    const std::string tm_op_s = extract_str("tm_op");
    if (!tm_op_s.empty()) m.tm_op = tm_op_s;
    try { const std::string s = extract_num("tm_exposure");           if (!s.empty()) m.tm_exposure           = std::stof(s); } catch (...) {}
    try { const std::string s = extract_num("tm_white_point");        if (!s.empty()) m.tm_white_point        = std::stof(s); } catch (...) {}
    try { const std::string s = extract_num("tm_mantiuk_contrast");   if (!s.empty()) m.tm_mantiuk_contrast   = std::stof(s); } catch (...) {}
    try { const std::string s = extract_num("tm_mantiuk_saturation"); if (!s.empty()) m.tm_mantiuk_saturation = std::stof(s); } catch (...) {}
    try { const std::string s = extract_num("tm_mantiuk_detail");     if (!s.empty()) m.tm_mantiuk_detail     = std::stof(s); } catch (...) {}

    return m;
}

} // namespace

gallery_manager_t::gallery_manager_t()
    : initialized_(false)
{}

bool gallery_manager_t::init(const std::string &gallery_dir)
{
    if (gallery_dir.empty()) return false;
    if (!make_dir(gallery_dir)) return false;
    gallery_dir_ = gallery_dir;
    if (!gallery_dir_.empty() && gallery_dir_.back() != '/' && gallery_dir_.back() != '\\') {
        gallery_dir_ += '/';
    }
    initialized_ = true;
    return true;
}

bool gallery_manager_t::is_initialized() const
{
    return initialized_;
}

std::string gallery_manager_t::entry_dir(const std::string &id) const
{
    return gallery_dir_ + id;
}

bool gallery_manager_t::write_file(const std::string &path, const void *data, size_t size)
{
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f.is_open()) return false;
    f.write(static_cast<const char *>(data), static_cast<std::streamsize>(size));
    return f.good();
}

bool gallery_manager_t::read_file(const std::string &path, std::vector<unsigned char> &out) const
{
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

bool gallery_manager_t::write_meta_locked(const std::string &id)
{
    auto it = entries_.find(id);
    if (it == entries_.end()) return false;
    const std::string json = meta_to_json(it->second);
    return write_file(entry_dir(id) + "/meta.json", json.data(), json.size());
}

bool gallery_manager_t::create_entry(const gallery_entry_meta_t &meta,
                                     const std::vector<unsigned char> &exr)
{
    if (!initialized_) return false;
    gallery_entry_meta_t m = meta;
    m.scene = basename_of(meta.scene);
    m.pass_count = 0;

    std::lock_guard<std::mutex> lock(mut_);
    const std::string dir = entry_dir(meta.id);
    if (!make_dir(dir)) return false;
    entries_[meta.id] = m;
    if (!write_meta_locked(meta.id)) return false;
    if (!exr.empty()) {
        write_file(dir + "/render.exr", exr.data(), exr.size());
    }
    return true;
}

bool gallery_manager_t::update_render(const std::string &id,
                                      const std::vector<unsigned char> &exr,
                                      double elapsed_ms)
{
    if (!initialized_ || exr.empty()) return false;
    std::lock_guard<std::mutex> lock(mut_);
    auto it = entries_.find(id);
    if (it != entries_.end()) {
        it->second.elapsed_ms = elapsed_ms;
        write_meta_locked(id);
    }
    return write_file(entry_dir(id) + "/render.exr", exr.data(), exr.size());
}

bool gallery_manager_t::save_pass(const std::string &id,
                                  size_t pass_index,
                                  const std::vector<unsigned char> &exr,
                                  double elapsed_ms)
{
    if (!initialized_ || exr.empty()) return false;
    std::lock_guard<std::mutex> lock(mut_);
    const std::string dir = entry_dir(id);
    write_file(dir + "/" + pass_filename(pass_index), exr.data(), exr.size());
    auto it = entries_.find(id);
    if (it != entries_.end()) {
        it->second.pass_count = pass_index + 1;
        it->second.elapsed_ms = elapsed_ms;
        write_meta_locked(id);
    }
    // Always update render.exr with the latest pass
    write_file(dir + "/render.exr", exr.data(), exr.size());
    return true;
}

std::vector<std::string> gallery_manager_t::list_entry_ids() const
{
    if (!initialized_) return {};
    std::vector<std::pair<long long, std::string>> entries;

    DIR *d = opendir(gallery_dir_.c_str());
    if (!d) return {};
    struct dirent *entry;
    while ((entry = readdir(d)) != nullptr) {
        const std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        const std::string dir = gallery_dir_ + name;
        struct stat st;
        if (stat(dir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) continue;

        // Try to get created_at_ms from meta.json
        long long ts = 0;
        {
            std::lock_guard<std::mutex> lock(mut_);
            auto it = entries_.find(name);
            if (it != entries_.end()) {
                ts = it->second.created_at_ms;
            }
        }
        if (ts == 0) {
            // Read from meta.json on disk
            std::ifstream f(dir + "/meta.json");
            if (f.is_open()) {
                std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                // Simple extraction of created_at_ms value
                const std::string key = "\"created_at_ms\":";
                size_t pos = json.find(key);
                if (pos != std::string::npos) {
                    pos += key.size();
                    ts = std::stoll(json.substr(pos));
                }
            }
        }
        entries.push_back({ts, name});
    }
    closedir(d);

    // Sort newest first
    std::sort(entries.begin(), entries.end(),
        [](const std::pair<long long, std::string> &a, const std::pair<long long, std::string> &b) {
            return a.first > b.first;
        });

    std::vector<std::string> ids;
    ids.reserve(entries.size());
    for (auto &p : entries) ids.push_back(p.second);
    return ids;
}

bool gallery_manager_t::get_meta_json(const std::string &id, std::string &out) const
{
    if (!initialized_) return false;
    {
        std::lock_guard<std::mutex> lock(mut_);
        auto it = entries_.find(id);
        if (it != entries_.end()) {
            out = meta_to_json(it->second);
            return true;
        }
    }
    // Fall back to reading from disk
    std::ifstream f(entry_dir(id) + "/meta.json");
    if (!f.is_open()) return false;
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return !out.empty();
}

bool gallery_manager_t::get_image_exr(const std::string &id, std::vector<unsigned char> &out) const
{
    if (!initialized_) return false;
    return read_file(entry_dir(id) + "/render.exr", out);
}

bool gallery_manager_t::get_pass_image_exr(const std::string &id, size_t pass_index,
                                            std::vector<unsigned char> &out) const
{
    if (!initialized_) return false;
    return read_file(entry_dir(id) + "/" + pass_filename(pass_index), out);
}

bool gallery_manager_t::get_meta_struct(const std::string &id, gallery_entry_meta_t &out) const
{
    if (!initialized_) return false;
    {
        std::lock_guard<std::mutex> lock(mut_);
        auto it = entries_.find(id);
        if (it != entries_.end()) {
            out = it->second;
            return true;
        }
    }
    std::ifstream f(entry_dir(id) + "/meta.json");
    if (!f.is_open()) return false;
    const std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (json.empty()) return false;
    out = parse_meta_from_json(json);
    return !out.id.empty();
}

bool gallery_manager_t::delete_entry(const std::string &id)
{
    if (!initialized_) return false;
    std::lock_guard<std::mutex> lock(mut_);
    entries_.erase(id);
    return remove_dir_recursive(entry_dir(id));
}

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */
