#pragma once

#include "common_benchmark.hpp"
#include "oldVersions/v3_finalVersion/v3_taskManager/easyLockcirculararray_v3.hpp"
#include <benchmark/benchmark.h>
#include <cmath>
#include <string>

struct MYContextCustom2 {
  AdjacencyArray<>::NodeHandle node;
  std::uint64_t childrenDone;
  double work;
};

template <typename TaskManager>
using WorkClassCustomType2 = WorkClassCustom2<MYContextCustom2, TaskManager>;
template <typename TaskManager>

using WorkClassCustomType2Instant =
    WorkClassCustom2Instant<MYContextCustom2, TaskManager>;

template <typename TaskManager>
static void BM_WSS_v3(benchmark::State &state, const std::string &path,
                      uint64_t weight) {
  const auto [edgelist, num_nodes] = readEdges(path);
  AdjacencyArray<> graph(num_nodes, edgelist);
  WorkClassCustomType2<TaskManager> workClass(
      graph, static_cast<uint64_t>(state.range(0)), weight);
  for (auto _ : state) {
    workClass.start();
  }
}
template <typename TaskManager>
static void BM_WSS_v3_1(benchmark::State &state, const std::string &path,
                        uint64_t weight) {
  const auto [edgelist, num_nodes] = readEdges(path);
  AdjacencyArray<> graph(num_nodes, edgelist);
  WorkClassCustomType2Instant<TaskManager> workClass(
      graph, static_cast<uint16_t>(state.range(0)), weight);
  for (auto _ : state) {
    workClass.start();
  }
}
void registerBenchmarks_v3(const std::string &path, uint64_t weight) {
  benchmark::RegisterBenchmark(
      "BM_WSS_V3_1_easyLockCircularArray",
      BM_WSS_v3_1<easyLockCircularArray_v3<MYContextCustom2>>, path, weight)
      ->Arg(1)
      ->Arg(2)
      ->Arg(4)
      ->Arg(6)
      ->Arg(9)
      ->Arg(12);
  // benchmark::RegisterBenchmark("BM_WSS_V3_0_ChaseLev",
  //                              BM_WSS_v3<ChaseLevOwnQueue_v3<MYContextCustom2>>,
  //                              path, weight)
  //     ->Arg(1)
  //     ->Arg(2)
  //     ->Arg(4)
  //     ->Arg(6)
  //     ->Arg(9)
  //     ->Arg(12);
  benchmark::RegisterBenchmark(
      "BM_WSS_V3_1_ChaseLev",
      BM_WSS_v3_1<ChaseLevOwnQueue_v3<MYContextCustom2>>, path, weight)
      ->Arg(1)
      ->Arg(2)
      ->Arg(4)
      ->Arg(6)
      ->Arg(9)
      ->Arg(12);

  // benchmark::RegisterBenchmark(
  //     "BM_WSS_V3_1_growableChaseLev",
  //     BM_WSS_v3_1<ChaseLevOwnQueue_v3experimental<MYContextCustom2>>, path,
  //     weight)
  //     ->Arg(1)
  //     ->Arg(2)
  //     ->Arg(4)
  //     ->Arg(6)
  //     ->Arg(9)
  //     ->Arg(12);

  // benchmark::RegisterBenchmark("BM_WSS_V3_1_OwnQueue",
  //                              BM_WSS_v3_1<ownQueue_v3<MYContextCustom2>>,
  //                              path, weight)
  //     ->Arg(1)
  //     ->Arg(2)
  //     ->Arg(4)
  //     ->Arg(6)
  //     ->Arg(9)
  //     ->Arg(12);
  // benchmark::RegisterBenchmark(
  //     "BM_WSS_V3_1_chase_fallback",
  //     BM_WSS_v3_1<ChaseLevOwnQueue_v3_fallback<MYContextCustom2>>, path,
  //     weight)
  //     ->Arg(1)
  //     ->Arg(2)
  //     ->Arg(4)
  //     ->Arg(6)
  //     ->Arg(9)
  //     ->Arg(12);
}
