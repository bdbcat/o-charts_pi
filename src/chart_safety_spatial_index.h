/******************************************************************************
 * Static spatial index used by the optional semantic chart-safety query.
 *
 * This contains no chart or wxWidgets types so its geometry can be tested in
 * isolation.  The owning chart supplies immutable feature bounding boxes and
 * retains ownership of the corresponding decoded objects.
 ******************************************************************************/

#ifndef O_CHARTS_CHART_SAFETY_SPATIAL_INDEX_H
#define O_CHARTS_CHART_SAFETY_SPATIAL_INDEX_H

#include <algorithm>
#include <cstddef>
#include <limits>
#include <vector>

namespace ocharts {
namespace chart_safety {

struct Bounds {
    double min_x;
    double min_y;
    double max_x;
    double max_y;

    Bounds()
        : min_x(std::numeric_limits<double>::infinity()),
          min_y(std::numeric_limits<double>::infinity()),
          max_x(-std::numeric_limits<double>::infinity()),
          max_y(-std::numeric_limits<double>::infinity()) {}

    Bounds(double x0, double y0, double x1, double y1)
        : min_x(std::min(x0, x1)), min_y(std::min(y0, y1)),
          max_x(std::max(x0, x1)), max_y(std::max(y0, y1)) {}

    bool IsValid() const {
        return min_x <= max_x && min_y <= max_y;
    }

    bool Intersects(const Bounds &other) const {
        return IsValid() && other.IsValid() &&
               min_x <= other.max_x && max_x >= other.min_x &&
               min_y <= other.max_y && max_y >= other.min_y;
    }

    void Expand(const Bounds &other) {
        if( !other.IsValid() ) return;
        min_x = std::min(min_x, other.min_x);
        min_y = std::min(min_y, other.min_y);
        max_x = std::max(max_x, other.max_x);
        max_y = std::max(max_y, other.max_y);
    }
};

class PackedBoundsIndex {
public:
    PackedBoundsIndex() : m_root(kNoNode) {}

    void Build(const std::vector<Bounds> &bounds) {
        m_bounds = bounds;
        m_order.resize(bounds.size());
        for( std::size_t i = 0; i < bounds.size(); ++i ) m_order[i] = i;
        m_nodes.clear();
        m_nodes.reserve(bounds.empty() ? 0 : bounds.size() * 2);
        m_root = bounds.empty() ? kNoNode : BuildNode(0, bounds.size());
    }

    void Query(const Bounds &bounds, std::vector<std::size_t> *matches) const {
        if( !matches || m_root == kNoNode || !bounds.IsValid() ) return;
        QueryNode(m_root, bounds, matches);
    }

    std::size_t Size() const { return m_bounds.size(); }

private:
    struct Node {
        Bounds bounds;
        std::size_t begin;
        std::size_t count;
        std::size_t left;
        std::size_t right;

        Node() : begin(0), count(0), left(kNoNode), right(kNoNode) {}
        bool IsLeaf() const { return left == kNoNode; }
    };

    static const std::size_t kNoNode = static_cast<std::size_t>(-1);
    static const std::size_t kLeafFeatures = 8;

    std::size_t BuildNode(std::size_t begin, std::size_t end) {
        Node node;
        node.begin = begin;
        node.count = end - begin;
        for( std::size_t i = begin; i < end; ++i )
            node.bounds.Expand(m_bounds[m_order[i]]);

        const std::size_t node_index = m_nodes.size();
        m_nodes.push_back(node);
        if( node.count <= kLeafFeatures ) return node_index;

        const bool split_x =
            (node.bounds.max_x - node.bounds.min_x) >=
            (node.bounds.max_y - node.bounds.min_y);
        const std::size_t middle = begin + node.count / 2;
        std::nth_element(
            m_order.begin() + begin, m_order.begin() + middle,
            m_order.begin() + end,
            [this, split_x](std::size_t lhs, std::size_t rhs) {
                const Bounds &a = m_bounds[lhs];
                const Bounds &b = m_bounds[rhs];
                const double ac = split_x ? a.min_x + a.max_x
                                          : a.min_y + a.max_y;
                const double bc = split_x ? b.min_x + b.max_x
                                          : b.min_y + b.max_y;
                return ac < bc;
            });

        const std::size_t left = BuildNode(begin, middle);
        const std::size_t right = BuildNode(middle, end);
        m_nodes[node_index].left = left;
        m_nodes[node_index].right = right;
        m_nodes[node_index].count = 0;
        return node_index;
    }

    void QueryNode(std::size_t node_index, const Bounds &query,
                   std::vector<std::size_t> *matches) const {
        const Node &node = m_nodes[node_index];
        if( !node.bounds.Intersects(query) ) return;
        if( node.IsLeaf() ) {
            for( std::size_t i = node.begin; i < node.begin + node.count; ++i ) {
                const std::size_t feature = m_order[i];
                if( m_bounds[feature].Intersects(query) )
                    matches->push_back(feature);
            }
            return;
        }
        QueryNode(node.left, query, matches);
        QueryNode(node.right, query, matches);
    }

    std::vector<Bounds> m_bounds;
    std::vector<std::size_t> m_order;
    std::vector<Node> m_nodes;
    std::size_t m_root;
};

}  // namespace chart_safety
}  // namespace ocharts

#endif  // O_CHARTS_CHART_SAFETY_SPATIAL_INDEX_H
