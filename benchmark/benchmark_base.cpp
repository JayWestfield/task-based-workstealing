
#include "benchmark_default.hpp"
#include "benchmark_reference_tbb.hpp"
#include "benchmark_v1.hpp"
#include "benchmark_v2.hpp"
#include "benchmark_v3.hpp"
#include "common_benchmark.hpp"

#include <benchmark/benchmark.h>
#include <iostream>

void recursiveFunction(AdjacencyArray<> &graph,
                       AdjacencyArray<>::NodeHandle node, uint64_t weight,
                       double weightload) {
  auto begin = graph.beginEdges(node);
  auto end = graph.endEdges(node);
  for (auto it = begin; it != end; ++it) {
    auto head = graph.edgeHead(it);
    recursiveFunction(graph, head, weight, graph.edgeLength(it));
  }
  volatile double anti_opt = 0;
  for (double i = 0; i < static_cast<double>(weight) * weightload * 1; ++i) {
    anti_opt = anti_opt + std::sin(i);
  }
}

static void BM_SingleThreadedRecursive(benchmark::State &state,
                                       const std::string &path,
                                       uint64_t weight) {
  const auto [edgelist, num_nodes] = readEdges(path);
  AdjacencyArray<> graph(num_nodes, edgelist);
  for (auto _ : state) {
    recursiveFunction(graph, graph.node(0), weight, 0);
  }
}

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <path> <weighted>\n";
    return 1;
  }
  std::string path = argv[1];
  uint64_t weighted = std::stoull(argv[2]);
  // benchmark::RegisterBenchmark("BM_SingleThreadedRecursive",
  //                              ::BM_SingleThreadedRecursive, path, weighted)
  //     ->Arg(1);

  registerBenchmarks_reference_tbb(path, weighted);
  registerBenchmarks_default(path, weighted);
  // registerBenchmarks_v1(path, weighted);
  // registerBenchmarks_v2(path, weighted);
  registerBenchmarks_v3(path, weighted);

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();

  return 0;
}
