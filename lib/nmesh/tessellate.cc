#include <vector>
#include "tessellate.h"

namespace nmesh {
    namespace generator {

void tessellate(object_t *obj, size_t iterations)
{
    while (--iterations != 0) tessellate(obj);
}

void tessellate(object_t *obj)
{
    std::vector<index_t> indices;
    std::vector<float>   v;
    std::vector<float>   n;
    std::vector<float>   uv;

}

    } /* namespace generator */
} /* namespace nmesh */
