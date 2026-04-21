#include "packager.h"
#include "converter.h"

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
    char tmpl[] = "/tmp/mitsuba2xtracer_XXXXXX";
    const char* p = mkdtemp(tmpl);
    return p ? p : "";
}

static bool extract_zip(const std::string& zip_path, const std::string& dest,
                        bool verbose, std::ostream& err) {
    if (!make_dirs(dest, err)) return false;

    // Shell out to unzip. Quote paths to handle spaces.
    std::string cmd = "unzip -o";
    if (!verbose) cmd += " -q";
    cmd += " \"" + zip_path + "\" -d \"" + dest + "\" 2>&1";

    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) {
        err << "Error: failed to run unzip\n";
        return false;
    }
    char buf[256];
    while (fgets(buf, sizeof(buf), fp))
        if (verbose) err << buf;
    int rc = pclose(fp);
    if (rc != 0) {
        err << "Error: unzip failed (exit " << rc
            << "). Is 'unzip' installed?\n";
        return false;
    }
    return true;
}

// ─── scene XML discovery ──────────────────────────────────────────────────────

// Returns the "best" XML path inside dir: prefers an XML with <scene> root.
static std::string find_scene_xml(const std::string& dir, std::ostream& err) {
    std::vector<std::string> xmls;
    find_files(dir, ".xml", xmls);

    if (xmls.empty()) {
        err << "Error: no .xml file found inside archive\n";
        return "";
    }
    if (xmls.size() == 1) return xmls[0];

    // Multiple XMLs — prefer one whose root element is <scene>
    for (auto& p : xmls) {
        std::ifstream f(p);
        std::string line;
        int checked = 0;
        while (std::getline(f, line) && checked < 20) {
            if (line.find("<scene") != std::string::npos) return p;
            ++checked;
        }
    }
    // Fallback: shortest path (usually the root-level one)
    std::string best = xmls[0];
    for (auto& p : xmls)
        if (p.size() < best.size()) best = p;
    err << "Warning: multiple XMLs found, using '" << best << "'\n";
    return best;
}

// ─── asset copying ────────────────────────────────────────────────────────────

static void copy_assets(const std::vector<ResourceEntry>& resources,
                        const std::string& dest_dir,
                        bool verbose, std::ostream& err) {
    for (auto& r : resources) {
        std::string sub = (r.type == "texture") ? "textures" : "geometry";
        std::string dst_dir = pb_join(pb_join(dest_dir, "resources"), sub);
        make_dirs(dst_dir, err);

        std::string dst = pb_join(dst_dir, r.filename);

        // Skip if source doesn't exist (referenced but not in zip — warn)
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
                std::string mtl_dst = pb_join(dst_dir, pb_stem(r.filename) + ".mtl");
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

    std::string tmp_dir;   // non-empty when we extracted a zip
    std::string xml_path;

    // ── 1. Resolve input XML ──────────────────────────────────────────────────
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
        xml_path = find_scene_xml(tmp_dir, err);
        if (xml_path.empty()) {
            std::string cmd = "rm -rf \"" + tmp_dir + "\"";
            system(cmd.c_str());
            return false;
        }
        if (popts.verbose) err << "Found scene XML: " << xml_path << "\n";
    } else {
        xml_path = input;
    }

    // ── 2. Determine scene name ───────────────────────────────────────────────
    std::string name = popts.scene_name.empty()
                     ? pb_stem(ext == ".zip" ? input : xml_path)
                     : popts.scene_name;

    // ── 3. Create output directory ────────────────────────────────────────────
    if (!make_dirs(dest, err)) return false;

    // ── 4. Convert ────────────────────────────────────────────────────────────
    ConvertOptions copts;
    copts.input_path  = xml_path;
    copts.xml_dir     = pb_dirname(xml_path);
    copts.dest_dir    = dest;
    copts.scene_name  = name;
    copts.verbose     = popts.verbose;

    std::vector<ResourceEntry> resources;
    std::ostringstream scene_buf;

    if (popts.verbose) err << "Converting scene...\n";
    bool ok = convert_mitsuba(copts, scene_buf, err,
                              popts.copy_assets ? &resources : nullptr);
    if (!ok) {
        if (!tmp_dir.empty()) {
            std::string cmd = "rm -rf \"" + tmp_dir + "\"";
            system(cmd.c_str());
        }
        return false;
    }

    // ── 5. Write .scn ─────────────────────────────────────────────────────────
    std::string scn_path = pb_join(dest, "mitsuba-" + name + ".scn");
    std::ofstream scn(scn_path);
    if (!scn) {
        err << "Error: cannot write '" << scn_path << "': " << strerror(errno) << "\n";
        return false;
    }
    scn << scene_buf.str();
    scn.close();
    err << "Wrote " << scn_path << "\n";

    // ── 6. Copy assets ────────────────────────────────────────────────────────
    if (popts.copy_assets && !resources.empty()) {
        if (popts.verbose) err << "Copying " << resources.size() << " asset(s)...\n";
        copy_assets(resources, dest, popts.verbose, err);
    }

    // ── 7. Cleanup temp dir ───────────────────────────────────────────────────
    if (!tmp_dir.empty()) {
        std::string cmd = "rm -rf \"" + tmp_dir + "\"";
        system(cmd.c_str());
    }

    return true;
}
