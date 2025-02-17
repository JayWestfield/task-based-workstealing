#pragma once

#include "graphTools/adj_array.hpp"
#include "graphTools/edge_list.hpp"
#include <benchmark/benchmark.h>
#include <cmath>
#include <string>
#include <tbb/global_control.h>
#include <tbb/task_group.h>

void parallelRecursiveFunction(AdjacencyArray<> &graph,
                               AdjacencyArray<>::NodeHandle node,
                               uint64_t weight, double weightload) {
  auto begin = graph.beginEdges(node);
  auto end = graph.endEdges(node);
  tbb::task_group tg;
  for (auto it = begin; it != end; ++it) {
    auto head = graph.edgeHead(it);
    tg.run([&graph, head, weight, &it] {
      parallelRecursiveFunction(graph, head, weight, graph.edgeLength(it));
    });
  }
  tg.wait(); // Wait for all tasks to complete
  volatile double anti_opt = 0;
  for (double i = 0; i < static_cast<double>(weight) * weightload * 1; ++i) {
    anti_opt = anti_opt + std::sin(i);
  }
}

static void BM_TBBTaskGroupRecursive(benchmark::State &state,
                                     const std::string &path, uint64_t weight) {
  const auto [edgelist, num_nodes] = readEdges(path);
  AdjacencyArray<> graph(num_nodes, edgelist);
  tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism,
                                   static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    parallelRecursiveFunction(graph, graph.node(0), weight, 0);
  }
}

void registerBenchmarks_reference_tbb(const std::string &path,
                                      uint64_t weight) {
  benchmark::RegisterBenchmark("BM_WSS_ref_TBB", BM_TBBTaskGroupRecursive, path,
                               weight)
      ->Arg(1)
      ->Arg(2)
      ->Arg(4)
      ->Arg(6)
      ->Arg(9)
      ->Arg(12);
}
