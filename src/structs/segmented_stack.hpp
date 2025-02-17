#pragma once

#include "../ws_common.hpp"
#include "task_group.hpp"
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <utility>
#include <vector>

template <typename TaskType, size_t size> class SegmentedStack {
public:
  SegmentedStack(uint16_t owning_thread) : owning_thread_(owning_thread) {
    stacks_.emplace_back(size);
    assert(stacks_.size() == 1);
  }

  template <typename... Args>
  TaskType *push_back(TaskType *parent_ref, Args &&...args) {
    // static_assert(
    //     std::is_constructible<TaskType,
    //                           TaskType *,
    //                           TaskType *, Args...>::value,
    //     "Task_v3 must be constructible with "
    //     "TaskType * and Args...");

    if (task_index_ == size) {
      stacks_.emplace_back(size);
      task_index_ = 0;
      stack_index_++;
    }
    // std::cout << "Push task " << owning_thread_ << " " << stack_index_ << " "
    //           << task_index_ << "\n";
    overrideTask(stacks_[stack_index_][task_index_], parent_ref,
                 std::forward<Args>(args)...);
    task_index_++;
    return &stacks_[stack_index_][task_index_ - 1];
  }

  // not needed just reuse the storage by whoever freed the entry => less
  // overhead but maybe do not give the task back to its creater but to whoever
  // frees it => everithing is local
  template <typename... Args>
  TaskType *create_task_at(ws::TaskReference task_place, TaskType *parent_ref,
                           Args &&...args) {
    overrideTask(stacks_[std::get<1>(task_place)][std::get<2>(task_place)],
                 parent_ref, std::forward<Args>(args)...);
    return &stacks_[std::get<1>(task_place)][std::get<2>(task_place)];
  }

  void clear() {
    stack_index_ = 0;
    task_index_ = 0;
    stacks_.clear();
    stacks_.emplace_back(size);
  }

  std::vector<TaskType> &operator[](std::size_t stack_index) {
    assert(stack_index <= stack_index_);
    return stacks_[stack_index];
  }
  std::vector<std::vector<TaskType>> stacks_;

private:
  size_t stack_index_ = 0;
  size_t task_index_ = 0;
  const ws::ThreadIndexType owning_thread_;

  template <typename... Args>
  void overrideTask(TaskType &task, TaskType *parent_ref, Args &&...args) {
    task.override(parent_ref, std::forward<Args>(args)...);

    // we do not need to override the total/ finished tasks since we only care
    // about the difference and if a task was freed the difference is 0
    // task.task_group = TaskGroup();
  }
};