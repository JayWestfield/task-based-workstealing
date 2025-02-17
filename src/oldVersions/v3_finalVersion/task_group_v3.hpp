#pragma once

#include "../common.hpp"
#include <atomic>
#include <cassert>
#include <tuple>
#include <vector>

template <typename TaskContext> class TaskGroup_v3 {
public:
  TaskGroup_v3() : total_tasks_(0), finished_tasks_(0) {}

  void addTask() { total_tasks_.fetch_add(1, std::memory_order_release); }

  bool taskCompleted() {
    int total_tasks = total_tasks_.load(std::memory_order_acquire);
    int task_that_finished =
        finished_tasks_.fetch_add(1, std::memory_order_release) + 1;
    assert(task_that_finished <= total_tasks);
    return task_that_finished == total_tasks;
  }

  std::atomic<int> total_tasks_;
  std::atomic<int> finished_tasks_;
};