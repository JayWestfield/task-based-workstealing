#pragma once

#include "../common.hpp"
#include <atomic>
#include <cassert>
#include <vector>

template <typename TaskContext> class TaskGroup_v2 {
public:
  TaskGroup_v2() : total_tasks_(0), finished_tasks_(0) {}

  void addTask(common::TaskReference newTask) {
    total_tasks_.fetch_add(1, std::memory_order_release);
    tasks_.push_back(newTask);
  }

  bool taskCompleted() {
    int total_tasks = total_tasks_.load(std::memory_order_acquire);
    int task_that_finished =
        finished_tasks_.fetch_add(1, std::memory_order_release) + 1;
    assert(task_that_finished <= total_tasks);
    return task_that_finished == total_tasks;
  }

  std::atomic<int> total_tasks_;
  std::atomic<int> finished_tasks_;
  std::vector<common::TaskReference> tasks_;
};