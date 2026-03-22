#ifndef XTCORE_MATH_KDTREE_H_INCLUDED
#define XTCORE_MATH_KDTREE_H_INCLUDED

#include <vector>

#include <nmath/precision.h>
#include <nmath/vector.h>

namespace xtcore {
namespace math {

template <typename T, typename Accessor>
class KDTree3
{
    public:
    struct Node
    {
        T data;
        int left;
        int right;
        int axis;
    };

    KDTree3();

    void clear();
    bool empty() const;

    void build(std::vector<T> items);
    void build(std::vector<T> items, const Accessor &accessor);

    template <typename Visitor>
    void radius_search(const nmath::Vector3f &point, nmath::scalar_t radius, Visitor visitor) const;

    private:
    int build_recursive(size_t begin, size_t end, int axis);

    std::vector<T> m_items;
    std::vector<Node> m_nodes;
    int m_root;
    Accessor m_accessor;
};

} // namespace math
} // namespace xtcore

#include "kdtree.tml"

#endif /* XTCORE_MATH_KDTREE_H_INCLUDED */
