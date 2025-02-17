#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
// Same as before
struct Edge {
  std::uint64_t from;
  std::uint64_t to;
  double length;
};

using EdgeList = std::vector<Edge>;

/**
 * Returns the list of edges and the number of nodes.
 */
inline auto
readEdges(const std::string &file) -> std::pair<EdgeList, std::size_t> {
  std::pair<EdgeList, std::size_t> edges;
  std::ifstream in(file);
  if (!in) {
    throw std::invalid_argument("Could not open file");
  }
  in >> edges.second;
  std::uint64_t from = 0;
  std::uint64_t to = 0;
  double length = 0;
  while (in >> from >> to >> length) {
    edges.first.push_back({from, to, length});
  }

  return edges;
};
