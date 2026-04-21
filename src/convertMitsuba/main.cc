#include "converter.h"
#include "packager.h"
#include <cstring>
#include <fstream>
#include <iostream>

static bool ends_with_ci(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    std::string tail = s.substr(s.size() - suffix.size());
    for (char& c : tail) c = (char)tolower((unsigned char)c);
    return tail == suffix;
}

static void usage(const char* prog) {
    std::cerr
        << "Usage:\n"
        << "  " << prog << " [options] input.xml           # convert, write to stdout\n"
        << "  " << prog << " [options] input.xml -o out.scn\n"
        << "  " << prog << " [options] input.xml -d ./out  # convert + organise assets\n"
        << "  " << prog << " [options] input.zip -d ./out  # extract, convert, organise\n"
        << "Options:\n"
        << "  -o, --output FILE    Write .scn to FILE (stdout if omitted; XML input only)\n"
        << "  -d, --dest DIR       Output directory for .scn + resources/ (default: ./scene)\n"
        << "  -n, --name NAME      Override output scene name\n"
        << "  -c, --comment TEXT   Write TEXT as a # comment line in the output\n"
        << "      --no-assets      Skip copying referenced textures / meshes\n"
        << "  -v, --verbose        Print conversion warnings and progress\n"
        << "  -h, --help           Show this help\n";
}

int main(int argc, char* argv[]) {
    std::string input_path;
    std::string output_path;
    std::string dest_dir;
    std::string scene_name;
    std::string comment;
    bool verbose    = false;
    bool no_assets  = false;

    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "-h") || !std::strcmp(argv[i], "--help")) {
            usage(argv[0]);
            return 0;
        } else if (!std::strcmp(argv[i], "-v") || !std::strcmp(argv[i], "--verbose")) {
            verbose = true;
        } else if (!std::strcmp(argv[i], "--no-assets")) {
            no_assets = true;
        } else if ((!std::strcmp(argv[i], "-o") || !std::strcmp(argv[i], "--output")) && i + 1 < argc) {
            output_path = argv[++i];
        } else if ((!std::strcmp(argv[i], "-d") || !std::strcmp(argv[i], "--dest")) && i + 1 < argc) {
            dest_dir = argv[++i];
        } else if ((!std::strcmp(argv[i], "-n") || !std::strcmp(argv[i], "--name")) && i + 1 < argc) {
            scene_name = argv[++i];
        } else if ((!std::strcmp(argv[i], "-c") || !std::strcmp(argv[i], "--comment")) && i + 1 < argc) {
            comment = argv[++i];
        } else if (argv[i][0] != '-') {
            if (input_path.empty()) {
                input_path = argv[i];
            } else {
                std::cerr << "Unexpected argument: " << argv[i] << "\n";
                return 1;
            }
        } else {
            std::cerr << "Unknown option: " << argv[i] << "\n";
            usage(argv[0]);
            return 1;
        }
    }

    if (input_path.empty()) {
        std::cerr << "Error: no input file specified\n";
        usage(argv[0]);
        return 1;
    }

    // Use packager when: input is a zip, or a dest dir was requested
    bool use_packager = ends_with_ci(input_path, ".zip") || !dest_dir.empty();

    if (use_packager) {
        if (!output_path.empty()) {
            std::cerr << "Error: -o and -d/-zip are mutually exclusive; use -d to set the destination directory\n";
            return 1;
        }
        PackageOptions popts;
        popts.input_path   = input_path;
        popts.dest_dir     = dest_dir.empty() ? "./scene" : dest_dir;
        popts.scene_name   = scene_name;
        popts.comment      = comment;
        popts.copy_assets  = !no_assets;
        popts.verbose      = verbose;
        return run_packager(popts, std::cerr) ? 0 : 1;
    }

    // Plain XML → stdout or single file
    ConvertOptions copts;
    copts.input_path  = input_path;
    copts.xml_dir     = "";  // no asset rewriting in plain mode
    copts.verbose     = verbose;
    copts.scene_name  = scene_name;
    copts.comment     = comment;

    if (output_path.empty()) {
        return convert_mitsuba(copts, std::cout, std::cerr) ? 0 : 1;
    }

    std::ofstream ofs(output_path);
    if (!ofs) {
        std::cerr << "Error: cannot open output file: " << output_path << "\n";
        return 1;
    }
    return convert_mitsuba(copts, ofs, std::cerr) ? 0 : 1;
}
