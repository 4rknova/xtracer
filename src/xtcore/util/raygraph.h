#ifndef XTCORE_RAYGRAPH_H_INCLUDED
#define XTCORE_RAYGRAPH_H_INCLUDED

#include <vector>
#include <nimg/color.h>
#include <nmath/vector.h>

using nmath::Vector3f;
using nimg::ColorRGBAf;

namespace xtcore {
    namespace raygraph {

struct sample_t
{
    Vector3f   position;
    ColorRGBAf color;
};

struct path_t
{
    std::vector<sample_t> samples;
};

struct bundle_t
{
    std::vector<path_t> paths;
};

struct raygraph_t
{
    std::vector<const bundle_t*> bundles;
};

int write(const char *filepath, const raygraph_t &raygraph);

    } /* namespace raygraph */
} /* namespace xtcore */

#endif /* XTCORE_RAYGRAPH_H_INCLUDED */
