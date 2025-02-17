#pragma once

#include "edge_list.hpp"

#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>
// same a as in previous exercise but now also supports weighted edges
template <class nodeID = uint32_t> class AdjacencyArray {
public:
  using NodeHandle = nodeID;
  using EdgeInfo = std::pair<NodeHandle, double>;
  using EdgeIterator = std::vector<EdgeInfo>::const_iterator;

  AdjacencyArray() = default;
  AdjacencyArray(std::size_t num_nodes, const EdgeList &edges)
      : index_(num_nodes + 1, 0) {
    static_assert(std::is_integral<nodeID>::value);
    assert(num_nodes <= std::numeric_limits<nodeID>::max());
    if (num_nodes >= std::numeric_limits<nodeID>::max())
      throw std::invalid_argument("bad Type");

    assert(index_.size() - 1 == num_nodes);
    for (const auto &edge : edges) {
      ++index_[edge.from + 1];
    }
    for (std::size_t i = 1; i < index_.size(); ++i) {
      index_[i] += index_[i - 1];
    }
    edges_.resize(edges.size());
    auto current_index = index_;
    for (const auto &edge : edges) {
      edges_[current_index[edge.from]++] = {static_cast<NodeHandle>(edge.to),
                                            edge.length};
    }
  }

  [[nodiscard]] auto numNodes() const -> std::size_t {
    return index_.size() - 1;
  }

  auto node(std::size_t n) const -> NodeHandle {
    return static_cast<NodeHandle>(n);
  }

  auto nodeId(NodeHandle n) const -> std::size_t { return n; }

  auto beginEdges(NodeHandle n) const -> EdgeIterator {
    return edges_.begin() +
           static_cast<std::vector<EdgeInfo>::difference_type>(index_[n]);
  }

  auto endEdges(NodeHandle n) const -> EdgeIterator {
    return edges_.begin() +
           static_cast<std::vector<EdgeInfo>::difference_type>(index_[n + 1]);
  }

  auto edgeHead(EdgeIterator it) const -> NodeHandle { return it->first; }

  auto edgeLength(EdgeIterator it) const -> double { return it->second; }

private:
  std::vector<std::size_t> index_;
  std::vector<EdgeInfo> edges_;
};