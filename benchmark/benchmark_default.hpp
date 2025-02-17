#pragma once

#include "common_benchmark.hpp"
#include "structs/task_queues/cyclic_array_atomic.hpp"
#include <benchmark/benchmark.h>
#include <cmath>
#include <string>

struct MYContextCustomDefault {
  AdjacencyArray<>::NodeHandle node;
  std::uint64_t childrenDone;
  double work;
};

template <typename TaskManager>
static void BM_WSS_default(benchmark::State &state, const std::string &path,
                           uint64_t weight) {
  const auto [edgelist, num_nodes] = readEdges(path);
  AdjacencyArray<> graph(num_nodes, edgelist);
  WorkClassCustomDefault<MYContextCustomDefault, TaskManager> workClass(
      graph, static_cast<uint16_t>(state.range(0)), weight);
  for (auto _ : state) {
    workClass.start();
  }
}
void registerBenchmarks_default(const std::string &path, uint64_t weight) {
  benchmark::RegisterBenchmark(
      "BM_WSS_default_CircularArrayLock",
      BM_WSS_default<seperateTaskQeues<
          Task<MYContextCustomDefault> *, nullptr,
          CyclicArrayLock<Task<MYContextCustomDefault> *, nullptr>>>,
      path, weight)
      ->Arg(1)
      ->Arg(2)
      ->Arg(4)
      ->Arg(6)
      ->Arg(9)
      ->Arg(12);
  benchmark::RegisterBenchmark(
      "BM_WSS_default_CircularArrayAtomic",
      BM_WSS_default<seperateTaskQeues<
          Task<MYContextCustomDefault> *, nullptr,
          CyclicArrayAtomic<Task<MYContextCustomDefault> *, nullptr>>>,
      path, weight)
      ->Arg(1)
      ->Arg(2)
      ->Arg(4)
      ->Arg(6)
      ->Arg(9)
      ->Arg(12);
}
