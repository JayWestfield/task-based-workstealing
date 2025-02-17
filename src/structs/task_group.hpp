#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>

class TaskGroup {
public:
  TaskGroup() : active_children_count_(0) {}

  void addTask() {
    active_children_count_.fetch_add(1, std::memory_order_release);
  }

  bool taskCompleted() {
    size_t activeChilds =
        active_children_count_.fetch_sub(1, std::memory_order_acquire);
    assert(activeChilds >= 1);
    return activeChilds == 1;
  }

  std::atomic<size_t> active_children_count_;
};