#include <iostream>
#include <fstream>
#include "raygraph.h"

namespace xtcore {
    namespace raygraph {

int write(const char *filepath, const raygraph_t &raygraph)
{
    if (!filepath) return 1;

    size_t count_vertices = 0;
    size_t count_edges    = 0;

    for (const bundle_t *bundle : raygraph.bundles) {
        if (!bundle) continue;
        for (const path_t &path : bundle->paths) {
            size_t count_path_vertices = path.samples.size();
            count_vertices += count_path_vertices;
            count_edges    += count_path_vertices - 1;
        }
    }

    std::ofstream file;
    file.open(filepath);

    // Write header
    // http://paulbourke.net/dataformats/ply/
    file << "ply"                               << std::endl;
    file << "format ascii 1.0"                  << std::endl;
    file << "comment object: A single line"     << std::endl;
    file << "element vertex " << count_vertices << std::endl;
    file << "property float x"                  << std::endl;
    file << "property float y"                  << std::endl;
    file << "property float z"                  << std::endl;
    file << "property float red"                << std::endl;
    file << "property float green"              << std::endl;
    file << "property float blue"               << std::endl;
    file << "element edge " << count_edges      << std::endl;
    file << "property int vertex1"              << std::endl;
    file << "property int vertex2"              << std::endl;
    file << "end_header"                        << std::endl;

    // Write vertex data
    for (const bundle_t *bundle : raygraph.bundles) {
        if (!bundle) continue;
        for (const path_t &path : bundle->paths) {
            for (const sample_t &sample : path.samples) {
                Vector3f   p = sample.position;
                ColorRGBAf c = sample.color;
                file << p.x   << " " << p.y   << " " << p.z   << " "
                     << c.r() << " " << c.g() << " " << c.b() << std::endl;
            }
        }
    }

    // Write edge data
    size_t offset = 0;
    for (const bundle_t *bundle : raygraph.bundles) {
        if (!bundle) continue;
        for (const path_t &path : bundle->paths) {
            size_t path_size = path.samples.size();
            for (size_t i = 1; i < path_size; ++i) {
                 size_t idx0 = offset + i - 1;
                 size_t idx1 = offset + i;
                 file << idx0 << " " << idx1 << std::endl;
            }
            offset += path_size;
        }
    }

    file.close();

    return 0;
}

    } /* namespace raygraph */
} /* namespace xtcore */
