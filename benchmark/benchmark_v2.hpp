#pragma once

#include "common_benchmark.hpp"
#include <benchmark/benchmark.h>
#include <cmath>
#include <iostream>
#include <string>
struct MYContextCustom {
  AdjacencyArray<>::NodeHandle node;
  std::uint64_t childrenDone;
  double work;
};

template <typename TaskManager>
using WorkClass_V2 = WorkClassCustom<MYContextCustom, TaskManager>;

template <typename TaskManager>
static void BM_WorkStealingSchedulerCustom(benchmark::State &state,
                                           const std::string &path,
                                           uint64_t weight) {
  const auto [edgelist, num_nodes] = readEdges(path);
  AdjacencyArray<> graph(num_nodes, edgelist);
  WorkClass_V2<TaskManager> workClass(
      graph, static_cast<uint16_t>(state.range(0)), weight);
  for (auto _ : state) {
    workClass.start();
  }
}

void registerBenchmarks_v2(const std::string &path, uint64_t weight) {
  benchmark::RegisterBenchmark(
      "BM_WSS_V2_ChaseLev",
      BM_WorkStealingSchedulerCustom<ChaseLevOwnQueue_v2<MYContextCustom>>,
      path, weight)
      ->Arg(1)
      ->Arg(2)
      ->Arg(4)
      ->Arg(6)
      ->Arg(9)
      ->Arg(12);
  // benchmark::RegisterBenchmark(
  //     "BM_WSS_V2_simpleQueue",
  //     BM_WorkStealingSchedulerCustom<ownQueueCustom_v2<MYContextCustom>>,
  //     path, weight)
  //     ->Arg(1)
  //     ->Arg(2)
  //     ->Arg(4)
  //     ->Arg(6)
  //     ->Arg(9)
  //     ->Arg(12);
}
