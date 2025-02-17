
#include "benchmark_v3.hpp"
#include "common_benchmark.hpp"
#include "graphTools/adj_array.hpp"
#include "graphTools/edge_list.hpp"
#include "v3_finalVersion/v3_taskManager/ChaseLevOwnQueue_v3.hpp"
#include "v3_finalVersion/v3_taskManager/ChaseLevOwnQueue_v3_fallback_grow.hpp"
#include <cassert>
#include <cmath>
#include <gperftools/profiler.h>
#include <iostream>
#include <iterator>
#include <ostream>
#include <tbb/global_control.h>
#include <tbb/task_arena.h>
#include <tbb/task_group.h>

// struct MYContext {
//   AdjacencyArray<>::NodeHandle node;
//   std::uint64_t childrenDone;
//   double work;
// };

// template <typename TaskManager> class WorkClass {
// public:
//   WorkClass(AdjacencyArray<> &graph, uint64_t numThreads, uint64_t
//   weightload)
//       : graph_(graph),
//         scheduler_(
//             numThreads,
//             std::make_shared<Task<MYContext>>(nullptr, graph.node(0),
//                                               static_cast<std::uint64_t>(0),
//                                               static_cast<double>(0)),
//             [this]() { this->idleFunction(); },
//             [this](std::shared_ptr<Task<MYContext>> task) {
//               return this->executeTask(task);
//             }),
//         numThreads_(numThreads), weightload_(weightload) {}

//   bool executeTask(std::shared_ptr<Task<MYContext>> task) {
//     assert(task != nullptr);
//     auto &[node, childrenDone, work] = task->context;
//     switch (childrenDone) {
//     case 0: { // Spawn children
//       auto begin = graph_.beginEdges(node);
//       auto end = graph_.endEdges(node);
//       for (auto it = begin; it != end; ++it) {
//         auto head = graph_.edgeHead(it);
//         task->addChild(head, static_cast<uint64_t>(0),
//         graph_.edgeLength(it));
//       }
//       childrenDone = 1;
//       scheduler_.waitTask(task);
//       return false;
//     } break;
//     case 1: {
//       volatile double anti_opt = 0;
//       for (double i = 0; i < work * static_cast<double>(weightload_) * 1;
//       ++i) {
//         anti_opt = anti_opt + std::sin(i);
//       }
//     }
//       return true;
//     default:
//       return true;
//     }
//   }
//   void start() {
//     scheduler_.setBaseTask(std::make_shared<Task<MYContext>>(
//         nullptr, graph_.node(0), static_cast<std::uint64_t>(0),
//         static_cast<double>(0)));
//     scheduler_.runAndWait();
//   }

//   void idleFunction() { std::this_thread::yield(); }

// private:
//   AdjacencyArray<> &graph_;
//   WorkStealingSchedulerCustom<WorkClass, MYContext, TaskManager> scheduler_;
//   uint64_t numThreads_;
//   uint64_t weightload_;
// };
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

struct MYContextCustom {
  AdjacencyArray<>::NodeHandle node;
  std::uint64_t childrenDone;
  double work;
};

template <typename TaskManager>
using WorkClassCustom1 = WorkClass<MYContextCustom, TaskManager>;
template <typename TaskManager>
using WorkClass_V2 = WorkClassCustom<MYContextCustom, TaskManager>;

// template <typename TaskManager>
// static void BM_WorkStealingSchedulerCustom(benchmark::State &state,
//                                            const std::string &path,
//                                            uint64_t weight) {
//   const auto [edgelist, num_nodes] = readEdges(path);
//   AdjacencyArray<> graph(num_nodes, edgelist);
//   WorkClassCustom<TaskManager> workClass(
//       graph, static_cast<uint64_t>(state.range(0)), weight);
//   for (auto _ : state) {
//     workClass.start();
//   }
// }
int main(int argc, char **argv) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " <path> <weighted> <numThreads>\n";
    return 1;
  }
  // std::cout << "i" << std::endl;

  std::string path = argv[1];
  uint64_t weighted = std::stoull(argv[2]);
  uint64_t numThreads = std::stoull(argv[3]);
  const auto [edgelist, num_nodes] = readEdges(path);
  AdjacencyArray<> graph(num_nodes, edgelist);
  tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism,
                                   (numThreads));
  // WorkClass<ChaseLevOwnQueue<MYContext>> workClass(graph, numThreads,
  // weighted);
  // WorkClassCustomType<ChaseLevOwnQueueCustom<MYContext>> workClass(
  //     graph, numThreads, weighted);
  // tbb::global_control
  // global_limit(tbb::global_control::max_allowed_parallelism,
  //                                  (numThreads));
  // WorkClassCustomType2<ChaseLevOwnQueueCustom2<MYContextCustom2>> workClass(
  //     graph, numThreads, weighted);
  // std::cout << "i";

  // WorkClassCustomType2Instant<ChaseLevOwnQueue_v3_fallback<MYContextCustom2>>
  //     workClass(graph, numThreads, weighted);

  WorkClassCustomType2Instant<easyLockCircularArray_v3<MYContextCustom2>>
      workClass(graph, numThreads, weighted);

  ProfilerStart("work_stealing_scheduler.prof");
  for (int i = 0; i < 100; ++i) {
    std::cout << i << std::endl;
    workClass.start();
  }
  // for (int i = 0; i < 100; ++i) {
  //   parallelRecursiveFunction(graph, graph.node(0), weighted, 0);
  // }
  ProfilerStop();

  return 0;
}