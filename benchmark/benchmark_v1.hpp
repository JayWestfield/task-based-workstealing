#pragma once

#include "common_benchmark.hpp"
#include <benchmark/benchmark.h>
#include <cmath>
#include <string>

struct MYContext_V1 {
  AdjacencyArray<>::NodeHandle node;
  std::uint64_t childrenDone;
  double work;
};

template <typename TaskManager>
using WorkClassChaseType = WorkClass<MYContext_V1, TaskManager>;

template <typename TaskManager>
static void BM_WSS_V1(benchmark::State &state, const std::string &path,
                      uint64_t weight) {
  const auto [edgelist, num_nodes] = readEdges(path);
  AdjacencyArray<> graph(num_nodes, edgelist);
  WorkClassChaseType<TaskManager> workClass(
      graph, static_cast<uint16_t>(state.range(0)), weight);
  for (auto _ : state) {
    workClass.start();
  }
}
void registerBenchmarks_v1(const std::string &path, uint64_t weight) {
  benchmark::RegisterBenchmark("BM_WSS_V1_ChaseLev",
                               BM_WSS_V1<ChaseLevOwnQueue_v1<MYContext_V1>>,
                               path, weight)
      ->Arg(1)
      ->Arg(2)
      ->Arg(4)
      ->Arg(6)
      ->Arg(9)
      ->Arg(12);
  // benchmark::RegisterBenchmark(
  //     "BM_WSS_V1_tbb_globalk_queue",
  //     BM_WSS_V1<tbbConcurrentQueueTaskManager_v1<MYContext_V1>>, path,
  //     weight)
  //     ->Arg(1)
  //     ->Arg(2)
  //     ->Arg(4)
  //     ->Arg(6)
  //     ->Arg(9)
  //     ->Arg(12);
  // benchmark::RegisterBenchmark(
  //     "BM_WSS_V1__SimpleOwnQueue",
  //     BM_WSS_V1<ownQueue_v1<MYContext_V1>>, path, weight)
  //     ->Arg(1)
  //     ->Arg(2)
  //     ->Arg(4)
  //     ->Arg(6)
  //     ->Arg(9)
  //     ->Arg(12);
}