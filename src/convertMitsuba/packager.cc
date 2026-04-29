#include "packager.h"
#include "converter.h"

#define MINIZ_IMPLEMENTATION
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "../../ext/miniz/miniz.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <vector>

// ─── path helpers ─────────────────────────────────────────────────────────────

static std::string pb_basename(const std::string& p) {
    size_t s = p.find_last_of("/\\");
    return (s != std::string::npos) ? p.substr(s + 1) : p;
}

static std::string pb_dirname(const std::string& p) {
    size_t s = p.find_last_of("/\\");
    return (s != std::string::npos) ? p.substr(0, s) : ".";
}

static std::string pb_stem(const std::string& p) {
    std::string n = pb_basename(p);
    size_t d = n.find_last_of('.');
    return (d != std::string::npos) ? n.substr(0, d) : n;
}

static std::string pb_ext(const std::string& p) {
    std::string n = pb_basename(p);
    size_t d = n.find_last_of('.');
    return (d != std::string::npos) ? n.substr(d) : "";
}

static std::string pb_join(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    if (b[0] == '/') return b;
    return a + "/" + b;
}

// ─── filesystem helpers ───────────────────────────────────────────────────────

static bool make_dirs(const std::string& path, std::ostream& err) {
    if (path.empty()) return true;
    // Walk the path and mkdir each component
    std::string cur;
    for (size_t i = 0; i <= path.size(); ++i) {
        if (i == path.size() || path[i] == '/') {
            if (!cur.empty()) {
                struct stat st;
                if (stat(cur.c_str(), &st) != 0) {
                    if (mkdir(cur.c_str(), 0755) != 0 && errno != EEXIST) {
                        err << "Error: mkdir '" << cur << "': " << strerror(errno) << "\n";
                        return false;
                    }
                }
            }
        }
        if (i < path.size()) cur += path[i];
    }
    return true;
}

static bool copy_file(const std::string& src, const std::string& dst, std::ostream& err) {
    std::ifstream in(src, std::ios::binary);
    if (!in) {
        err << "Warning: cannot open '" << src << "' for copying: " << strerror(errno) << "\n";
        return false;
    }
    std::ofstream out(dst, std::ios::binary);
    if (!out) {
        err << "Warning: cannot create '" << dst << "': " << strerror(errno) << "\n";
        return false;
    }
    out << in.rdbuf();
    return true;
}

// Recursively collect all files under dir matching extension (e.g. ".xml")
static void find_files(const std::string& dir, const std::string& ext,
                       std::vector<std::string>& out) {
    DIR* d = opendir(dir.c_str());
    if (!d) return;
    struct dirent* e;
    while ((e = readdir(d)) != nullptr) {
        std::string name = e->d_name;
        if (name == "." || name == "..") continue;
        std::string full = pb_join(dir, name);
        if (e->d_type == DT_DIR) {
            find_files(full, ext, out);
        } else {
            // Case-insensitive extension check
            std::string n = name;
            for (char& c : n) c = (char)tolower((unsigned char)c);
            std::string ex = ext;
            for (char& c : ex) c = (char)tolower((unsigned char)c);
            if (n.size() >= ex.size() && n.substr(n.size() - ex.size()) == ex)
                out.push_back(full);
        }
    }
    closedir(d);
}

// ─── zip extraction ───────────────────────────────────────────────────────────

static std::string make_temp_dir() {
    char tmpl[] = "/tmp/convertMitsuba_XXXXXX";
    const char* p = mkdtemp(tmpl);
    return p ? p : "";
}

static bool extract_zip(const std::string& zip_path, const std::string& dest,
                        bool verbose, std::ostream& err) {
    if (!make_dirs(dest, err)) return false;

    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);

    if (!mz_zip_reader_init_file(&zip, zip_path.c_str(), 0)) {
        err << "Error: cannot open zip '" << zip_path << "'\n";
        return false;
    }

    mz_uint num = mz_zip_reader_get_num_files(&zip);
    bool ok = true;

    for (mz_uint i = 0; i < num; ++i) {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&zip, i, &st)) {
            err << "Warning: cannot stat zip entry " << i << ", skipping\n";
            continue;
        }
        if (st.m_is_directory) continue;

        std::string dst = pb_join(dest, st.m_filename);
        std::string parent = pb_dirname(dst);
        if (!make_dirs(parent, err)) { ok = false; continue; }

        if (verbose) err << "  extracting " << st.m_filename << "\n";

        if (!mz_zip_reader_extract_to_file(&zip, i, dst.c_str(), 0)) {
            err << "Warning: failed to extract '" << st.m_filename << "'\n";
            ok = false;
        }
    }

    mz_zip_reader_end(&zip);
    return ok;
}

// ─── scene XML discovery ──────────────────────────────────────────────────────

// Returns all Mitsuba scene XML paths inside dir (those with a <scene> root).
static std::vector<std::string> find_scene_xmls(const std::string& dir, std::ostream& err) {
    std::vector<std::string> xmls;
    find_files(dir, ".xml", xmls);
    std::sort(xmls.begin(), xmls.end());

    std::vector<std::string> result;
    for (auto& p : xmls) {
        std::ifstream f(p);
        std::string line;
        int checked = 0;
        while (std::getline(f, line) && checked < 20) {
            if (line.find("<scene") != std::string::npos) { result.push_back(p); break; }
            ++checked;
        }
    }

    if (result.empty())
        err << "Error: no Mitsuba scene XML found inside archive\n";
    return result;
}

// Derive a short version tag from an XML filename for use as a .scn suffix.
// scene_v3.xml -> "v3", scene_v0.6.xml -> "v0.6", foo.xml -> "foo"
static std::string xml_version_tag(const std::string& xml_path) {
    std::string stem = pb_stem(pb_basename(xml_path));
    // strip leading "scene_" or "scene" prefix
    if (stem.size() > 6 && stem.substr(0, 6) == "scene_") return stem.substr(6);
    if (stem == "scene") return "";
    return stem;
}

// ─── asset copying ────────────────────────────────────────────────────────────

static void copy_assets(const std::vector<ResourceEntry>& resources,
                        const std::string& dest_dir,
                        const std::string& scene_name,
                        bool verbose, std::ostream& err) {
    std::string res_dir = pb_join(pb_join(dest_dir, "resources"), scene_name);
    make_dirs(res_dir, err);

    for (auto& r : resources) {
        std::string dst = pb_join(res_dir, r.filename);

        struct stat st;
        if (stat(r.src_path.c_str(), &st) != 0) {
            err << "Warning: asset not found, skipping: " << r.src_path << "\n";
            continue;
        }

        if (verbose) err << "  copying " << r.type << ": " << r.filename << "\n";
        copy_file(r.src_path, dst, err);

        // For OBJ meshes, also look for a sibling .mtl file
        if (r.type == "geometry" && pb_ext(r.filename) == ".obj") {
            std::string mtl_src = pb_join(pb_dirname(r.src_path),
                                          pb_stem(r.filename) + ".mtl");
            if (stat(mtl_src.c_str(), &st) == 0) {
                std::string mtl_dst = pb_join(res_dir, pb_stem(r.filename) + ".mtl");
                if (verbose) err << "  copying geometry: " << pb_stem(r.filename) << ".mtl\n";
                copy_file(mtl_src, mtl_dst, err);
            }
        }
    }
}

// ─── public API ───────────────────────────────────────────────────────────────

bool run_packager(const PackageOptions& popts, std::ostream& err) {
    std::string input  = popts.input_path;
    std::string dest   = popts.dest_dir.empty() ? "./scene" : popts.dest_dir;

    std::string tmp_dir;
    std::vector<std::string> xml_paths;

    // ── 1. Resolve input XMLs ─────────────────────────────────────────────────
    std::string ext = pb_ext(input);
    for (char& c : ext) c = (char)tolower((unsigned char)c);

    if (ext == ".zip") {
        tmp_dir = make_temp_dir();
        if (tmp_dir.empty()) {
            err << "Error: failed to create temp directory\n";
            return false;
        }
        if (popts.verbose) err << "Extracting '" << input << "'...\n";
        if (!extract_zip(input, tmp_dir, popts.verbose, err)) {
            rmdir(tmp_dir.c_str());
            return false;
        }
        xml_paths = find_scene_xmls(tmp_dir, err);
        if (xml_paths.empty()) {
            std::string cmd = "rm -rf \"" + tmp_dir + "\"";
            system(cmd.c_str());
            return false;
        }
        if (popts.verbose) {
            for (auto& p : xml_paths) err << "Found scene XML: " << p << "\n";
        }
    } else {
        xml_paths.push_back(input);
    }

    // ── 2. Determine base scene name (used for resource directory) ────────────
    std::string base_name = popts.scene_name.empty()
                          ? pb_stem(ext == ".zip" ? input : xml_paths[0])
                          : popts.scene_name;

    // ── 3. Create output directory ────────────────────────────────────────────
    if (!make_dirs(dest, err)) return false;

    // ── 4. Convert each XML ───────────────────────────────────────────────────
    std::vector<ResourceEntry> all_resources;
    bool any_ok = false;

    for (const auto& xml_path : xml_paths) {
        // Derive per-file suffix when multiple XMLs are present
        std::string tag = (xml_paths.size() > 1) ? xml_version_tag(xml_path) : "";
        std::string out_name = tag.empty() ? base_name : (base_name + "-" + tag);

        ConvertOptions copts;
        copts.input_path  = xml_path;
        copts.xml_dir     = pb_dirname(xml_path);
        copts.dest_dir    = dest;
        copts.scene_name  = base_name;   // resource dir always uses base name
        copts.source_path = popts.input_path;
        copts.comment     = popts.comment;
        copts.verbose     = popts.verbose;

        std::vector<ResourceEntry> resources;
        std::ostringstream scene_buf;

        if (popts.verbose) err << "Converting " << pb_basename(xml_path) << "...\n";
        bool ok = convert_mitsuba(copts, scene_buf, err,
                                  popts.copy_assets ? &resources : nullptr);
        if (!ok) {
            err << "Warning: conversion failed for " << pb_basename(xml_path) << ", skipping\n";
            continue;
        }

        // ── 5. Write .scn ─────────────────────────────────────────────────────
        std::string scn_path = pb_join(dest, "mitsuba-" + out_name + ".scn");
        std::ofstream scn(scn_path);
        if (!scn) {
            err << "Error: cannot write '" << scn_path << "': " << strerror(errno) << "\n";
            continue;
        }
        scn << scene_buf.str();
        scn.close();
        err << "Wrote " << scn_path << "\n";
        any_ok = true;

        // Accumulate resources, deduplicating by src_path
        for (auto& r : resources) {
            bool dup = false;
            for (auto& ar : all_resources)
                if (ar.src_path == r.src_path && ar.type == r.type) { dup = true; break; }
            if (!dup) all_resources.push_back(r);
        }
    }

    // ── 6. Copy assets once for all conversions ───────────────────────────────
    if (popts.copy_assets && !all_resources.empty()) {
        if (popts.verbose) err << "Copying " << all_resources.size() << " asset(s)...\n";
        copy_assets(all_resources, dest, base_name, popts.verbose, err);
    }

    // ── 7. Cleanup temp dir ───────────────────────────────────────────────────
    if (!tmp_dir.empty()) {
        std::string cmd = "rm -rf \"" + tmp_dir + "\"";
        system(cmd.c_str());
    }

    return any_ok;
}
