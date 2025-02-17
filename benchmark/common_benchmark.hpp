#pragma once

#include "graphTools/adj_array.hpp"
#include "graphTools/edge_list.hpp"
#include "oldVersions/v3_finalVersion/work_stealing_scheduler_v3.hpp"
#include "releaseVersion/work_stealing_scheduler.hpp"
#include "work_stealing_config.hpp"
#include <benchmark/benchmark.h>
#include <cmath>
#include <iostream>
#include <string>

struct MYContext {
  AdjacencyArray<>::NodeHandle node;
  std::uint64_t childrenDone;
  double work;
};

template <typename Context, typename TaskManager> class WorkClass {
public:
  WorkClass(AdjacencyArray<> &graph, uint16_t numThreads, uint64_t weightload)
      : graph_(graph),
        scheduler_(
            numThreads,
            std::make_shared<Task_v1<Context>>(nullptr, graph.node(0),
                                               static_cast<std::uint64_t>(0),
                                               static_cast<double>(0)),
            [this]() { this->idleFunction(); },
            [this](std::shared_ptr<Task_v1<Context>> task) {
              return this->executeTask(task);
            }),
        numThreads_(numThreads), weightload_(weightload) {}

  bool executeTask(std::shared_ptr<Task_v1<Context>> task) {
    assert(task != nullptr);
    auto &[node, childrenDone, work] = task->context;
    switch (childrenDone) {
    case 0: { // Spawn children
      auto begin = graph_.beginEdges(node);
      auto end = graph_.endEdges(node);
      for (auto it = begin; it != end; ++it) {
        auto head = graph_.edgeHead(it);
        task->addChild(head, static_cast<uint64_t>(0), graph_.edgeLength(it));
      }
      childrenDone = 1;
      scheduler_.waitTask(task);
      return false;
    } break;
    case 1: {
      volatile double anti_opt = 0;
      for (double i = 0; i < work * static_cast<double>(weightload_) * 1; ++i) {
        anti_opt = anti_opt + std::sin(i);
      }
    }
      return true;
    default:
      return true;
    }
  }

  void start() {
    scheduler_.setBaseTask(std::make_shared<Task_v1<Context>>(
        nullptr, graph_.node(0), static_cast<std::uint64_t>(0),
        static_cast<double>(0)));
    scheduler_.runAndWait();
  }

  void idleFunction() { std::this_thread::yield(); }

private:
  AdjacencyArray<> &graph_;
  wss1<Context, TaskManager> scheduler_;
  uint64_t numThreads_;
  uint64_t weightload_;
};

template <typename Context, typename TaskManager> class WorkClassCustom {
public:
  WorkClassCustom(AdjacencyArray<> &graph, uint16_t numThreads,
                  uint64_t weightload)
      : graph_(graph), scheduler_(
                           numThreads, [this]() { this->idleFunction(); },
                           [this](common::TaskReference task) {
                             return this->executeTask(task);
                           }),
        numThreads_(numThreads), weightload_(weightload) {}

  bool executeTask(common::TaskReference task) {
    auto &[node, childrenDone, work] = scheduler_.accessTask(task).context;
    switch (childrenDone) {
    case 0: { // Spawn children
      auto begin = graph_.beginEdges(node);
      auto end = graph_.endEdges(node);
      for (auto it = begin; it != end; ++it) {
        auto head = graph_.edgeHead(it);
        scheduler_.addChild(task, head, static_cast<uint64_t>(0),
                            graph_.edgeLength(it));
      }
      childrenDone = 1;
      scheduler_.waitTask(task);
      return false;
    } break;
    case 1: {
      volatile double anti_opt = 0;
      for (double i = 0; i < work * static_cast<double>(weightload_) * 1; ++i) {
        anti_opt = anti_opt + std::sin(i);
      }
    }
      return true;
    default:
      return true;
    }
  }

  void start() {
    scheduler_.setBaseTaskArgs(graph_.node(0), static_cast<std::uint64_t>(0),
                               static_cast<double>(0));
    scheduler_.runAndWait();
  }

  void idleFunction() { std::this_thread::yield(); }

private:
  AdjacencyArray<> &graph_;
  WorkStealingScheduler_v2<Context, TaskManager> scheduler_;
  uint64_t numThreads_;
  uint64_t weightload_;
};

template <typename Context, typename TaskManager> class WorkClassCustom2 {
public:
  WorkClassCustom2(AdjacencyArray<> &graph, uint64_t numThreads,
                   uint64_t weightload)
      : graph_(graph),
        scheduler_(
            numThreads, [this]() { this->idleFunction(); },
            [this](Task_v3<Context> *task) { return this->executeTask(task); }),
        numThreads_(numThreads), weightload_(weightload) {}

  bool executeTask(Task_v3<Context> *task) {
    auto &[node, childrenDone, work] = (*task).context;
    // std::cout << "node " << node << " childrenDone " << childrenDone << "
    // work "
    //           << work << std::endl;
    switch (childrenDone) {
    case 0: { // Spawn children
      auto begin = graph_.beginEdges(node);
      auto end = graph_.endEdges(node);
      for (auto it = begin; it != end; ++it) {
        auto head = graph_.edgeHead(it);
        scheduler_.addChild(task, head, static_cast<uint64_t>(0),
                            graph_.edgeLength(it));
      }
      childrenDone = 1;
      scheduler_.waitTask(task);
      return false;
    } break;
    case 1: {
      volatile double anti_opt = 0;
      for (double i = 0; i < work * static_cast<double>(weightload_) * 1; ++i) {
        anti_opt = anti_opt + std::sin(i);
      }
    }
      return true;
    default:
      return true;
    }
  }

  void start() {
    scheduler_.setBaseTaskArgs(graph_.node(0), static_cast<std::uint64_t>(0),
                               static_cast<double>(0));
    scheduler_.runAndWait();
  }

  void idleFunction() { std::this_thread::yield(); }

private:
  AdjacencyArray<> &graph_;
  WorkStealingScheduler_v3<Context, TaskManager> scheduler_;
  uint64_t numThreads_;
  uint64_t weightload_;
};
template <typename Context, typename TaskManager>
class WorkClassCustom2Instant {
public:
  WorkClassCustom2Instant(AdjacencyArray<> &graph, uint16_t numThreads,
                          uint64_t weightload)
      : graph_(graph),
        scheduler_(
            numThreads, [this]() { this->idleFunction(); },
            [this](Task_v3<Context> *task) { return this->executeTask(task); }),
        numThreads_(numThreads), weightload_(weightload) {}

  bool executeTask(Task_v3<Context> *task) {
    auto &[node, childrenDone, work] = (*task).context;
    // std::cout << "node " << node << " childrenDone " << childrenDone << "
    // work "
    //           << work << std::endl;
    switch (childrenDone) {
    case 0: { // Spawn children
      auto begin = graph_.beginEdges(node);
      auto end = graph_.endEdges(node);
      for (auto it = begin; it != end; ++it) {
        auto head = graph_.edgeHead(it);
        scheduler_.addChild(task, head, static_cast<uint64_t>(0),
                            graph_.edgeLength(it));
      }
      childrenDone = 1;
      scheduler_.waitTask(task);
      return false;
    } break;
    case 1: {
      volatile double anti_opt = 0;
      for (double i = 0; i < work * static_cast<double>(weightload_) * 1; ++i) {
        anti_opt = anti_opt + std::sin(i);
      }
    }
      return true;
    default:
      return true;
    }
  }

  void start() {
    scheduler_.setBaseTaskArgs(graph_.node(0), static_cast<std::uint64_t>(0),
                               static_cast<double>(0));
    scheduler_.runAndWait();
  }

  void idleFunction() { std::this_thread::yield(); }

private:
  AdjacencyArray<> &graph_;
  WorkStealingScheduler_v3_1<Context, TaskManager> scheduler_;
  uint64_t numThreads_;
  uint64_t weightload_;
};

template <typename Context, typename TaskManager> class WorkClassCustomDefault {
public:
  WorkClassCustomDefault(AdjacencyArray<> &graph, uint16_t numThreads,
                         uint64_t weightload)
      : graph_(graph),
        scheduler_(
            numThreads, [this]() { this->idleFunction(); },
            [this](Task<Context> *task) { return this->executeTask(task); }),
        numThreads_(numThreads), weightload_(weightload) {}

  bool executeTask(Task<Context> *task) {
    auto &[node, childrenDone, work] = (*task).context;
    // std::cout << "node " << node << " childrenDone " << childrenDone << "
    // work "
    //           << work << std::endl;
    switch (childrenDone) {
    case 0: { // Spawn children
      auto begin = graph_.beginEdges(node);
      auto end = graph_.endEdges(node);
      for (auto it = begin; it != end; ++it) {
        auto head = graph_.edgeHead(it);
        scheduler_.addChild(task, head, static_cast<uint64_t>(0),
                            graph_.edgeLength(it));
      }
      childrenDone = 1;
      scheduler_.waitTask(task);
      return false;
    } break;
    case 1: {
      volatile double anti_opt = 0;
      for (double i = 0; i < work * static_cast<double>(weightload_) * 1; ++i) {
        anti_opt = anti_opt + std::sin(i);
      }
    }
      return true;
    default:
      return true;
    }
  }

  void start() {
    scheduler_.setBaseTaskArgs(graph_.node(0), static_cast<std::uint64_t>(0),
                               static_cast<double>(0));
    scheduler_.runAndWait();
  }

  void idleFunction() { std::this_thread::yield(); }

private:
  AdjacencyArray<> &graph_;
  WorkStealingScheduler<Context, TaskManager> scheduler_;
  uint64_t numThreads_;
  uint64_t weightload_;
};