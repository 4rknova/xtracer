#pragma once
#include <iosfwd>
#include <string>
#include <vector>

struct ResourceEntry {
    std::string src_path;   // absolute path to source file
    std::string type;       // "texture" or "geometry"
    std::string filename;   // destination basename (under resources/<type>/)
};

struct ConvertOptions {
    std::string input_path;  // path to .xml file
    std::string xml_dir;     // directory containing the xml (for resolving relative paths)
    std::string dest_dir;    // if set, rewrite asset paths into resources/ layout
    std::string scene_name;  // override scene name (defaults to stem of input_path)
    bool verbose = false;
};

// Converts a Mitsuba 3 XML scene to xtracer NCF format.
// Writes NCF to 'out', diagnostic messages to 'err'.
// If resources is non-null and opts.dest_dir is set, populates it with
// all referenced asset files (for the caller to copy).
// Returns true on success.
bool convert_mitsuba(const ConvertOptions& opts, std::ostream& out, std::ostream& err,
                     std::vector<ResourceEntry>* resources = nullptr);
