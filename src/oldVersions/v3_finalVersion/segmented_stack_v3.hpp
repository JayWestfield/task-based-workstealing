#pragma once

#include "../common.hpp"
#include "task_v3.hpp"
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <utility>
#include <vector>

template <typename TaskContext, size_t size> class SegmentedStack_v3 {
public:
  SegmentedStack_v3(uint16_t owning_thread) : owning_thread_(owning_thread) {
    stacks_.emplace_back(size);
    assert(stacks_.size() == 1);
  }

  template <typename... Args>
  Task_v3<TaskContext> *push_back(Task_v3<TaskContext> *parent_ref,
                                  Args &&...args) {
    // static_assert(
    //     std::is_constructible<Task_v3<TaskContext>,
    //                           Task_v3<TaskContext> *,
    //                           Task_v3<TaskContext> *, Args...>::value,
    //     "Task_v3 must be constructible with "
    //     "Task_v3<TaskContext> * and Args...");

    if (task_index_ == size) {
      stacks_.emplace_back(size);
      task_index_ = 0;
      stack_index_++;
    }
    // std::cout << "Push task " << owning_thread_ << " " << stack_index_ << " "
    //           << task_index_ << "\n";
    overrideTask(
        stacks_[stack_index_][task_index_],
        common::TaskReference(owning_thread_, stack_index_, task_index_),
        parent_ref, std::forward<Args>(args)...);
    task_index_++;
    return &stacks_[stack_index_][task_index_ - 1];
  }

  template <typename... Args>
  Task_v3<TaskContext> *create_task_at(common::TaskReference task_place,
                                       Task_v3<TaskContext> *parent_ref,
                                       Args &&...args) {
    overrideTask(stacks_[std::get<1>(task_place)][std::get<2>(task_place)],
                 task_place, parent_ref, std::forward<Args>(args)...);
    return &stacks_[std::get<1>(task_place)][std::get<2>(task_place)];
  }

  void clear() {
    stack_index_ = 0;
    task_index_ = 0;
    stacks_.clear();
    stacks_.emplace_back(size);
  }

  std::vector<Task_v3<TaskContext>> &operator[](std::size_t stack_index) {
    assert(stack_index <= stack_index_);
    return stacks_[stack_index];
  }
  std::vector<std::vector<Task_v3<TaskContext>>> stacks_;

private:
  size_t stack_index_ = 0;
  size_t task_index_ = 0;
  const uint16_t owning_thread_;

  template <typename... Args>
  void overrideTask(Task_v3<TaskContext> &task,
                    common::TaskReference selfReference,
                    Task_v3<TaskContext> *parent_ref, Args &&...args) {
    task.context = TaskContext(std::forward<Args>(args)...);
    task.parentTask = parent_ref;
    task.selfReference = selfReference;
    assert(task.task_group.total_tasks_ == task.task_group.finished_tasks_);
    // we do not need to override the total/ finished tasks since we only care
    // about the difference and if a task was freed the difference is 0
    // task.task_group.total_tasks_= 0;
    // task.task_group.finished_tasks_ = 0;
  }
};