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
    ss << "pass_" << std::setw(4) << std::setfill('0') << idx << ".png";
    return ss.str();
}

std::string meta_to_json(const gallery_entry_meta_t &m)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
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
       << "\"pass_count\":"  << m.pass_count
       << "}";
    return ss.str();
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
                                     const std::vector<unsigned char> &png)
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
    if (!png.empty()) {
        write_file(dir + "/render.png", png.data(), png.size());
    }
    return true;
}

bool gallery_manager_t::update_render(const std::string &id,
                                      const std::vector<unsigned char> &png,
                                      double elapsed_ms)
{
    if (!initialized_ || png.empty()) return false;
    std::lock_guard<std::mutex> lock(mut_);
    auto it = entries_.find(id);
    if (it != entries_.end()) {
        it->second.elapsed_ms = elapsed_ms;
        write_meta_locked(id);
    }
    return write_file(entry_dir(id) + "/render.png", png.data(), png.size());
}

bool gallery_manager_t::save_pass(const std::string &id,
                                  size_t pass_index,
                                  const std::vector<unsigned char> &png,
                                  double elapsed_ms)
{
    if (!initialized_ || png.empty()) return false;
    std::lock_guard<std::mutex> lock(mut_);
    const std::string dir = entry_dir(id);
    write_file(dir + "/" + pass_filename(pass_index), png.data(), png.size());
    auto it = entries_.find(id);
    if (it != entries_.end()) {
        it->second.pass_count = pass_index + 1;
        it->second.elapsed_ms = elapsed_ms;
        write_meta_locked(id);
    }
    // Always update render.png with the latest pass
    write_file(dir + "/render.png", png.data(), png.size());
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

bool gallery_manager_t::get_image(const std::string &id, std::vector<unsigned char> &out) const
{
    if (!initialized_) return false;
    return read_file(entry_dir(id) + "/render.png", out);
}

bool gallery_manager_t::get_pass_image(const std::string &id, size_t pass_index,
                                       std::vector<unsigned char> &out) const
{
    if (!initialized_) return false;
    return read_file(entry_dir(id) + "/" + pass_filename(pass_index), out);
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
