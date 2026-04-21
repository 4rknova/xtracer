#pragma once
#include <iosfwd>
#include <string>

struct PackageOptions {
    std::string input_path;     // .zip or .xml
    std::string dest_dir;       // output directory (default: "./scene")
    std::string scene_name;     // override scene stem name
    std::string comment;        // optional comment written as # line in output
    bool copy_assets = true;    // copy referenced textures / meshes
    bool verbose     = false;
};

// Extracts (if zip), converts, organises assets, writes .scn.
// Returns true on success.
bool run_packager(const PackageOptions& opts, std::ostream& err);
