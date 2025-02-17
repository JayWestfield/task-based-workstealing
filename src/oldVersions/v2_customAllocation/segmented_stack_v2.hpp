#pragma once

#include "task_v2.hpp"
#include <cassert>
#include <concepts>
#include <iostream>
#include <utility>
#include <vector>

template <typename TaskContext, int size> class SegmentedStack_v2 {
public:
  SegmentedStack_v2(int owning_thread) : owning_thread_(owning_thread) {
    stacks_.emplace_back(size);
    assert(stacks_.size() == 1);
  }

  template <typename... Args>
  common::TaskReference push_back(common::TaskReference parent_ref,
                                  Args &&...args) {
    static_assert(std::is_constructible<Task_v2<TaskContext>,
                                        common::TaskReference, Args...>::value,
                  "TaskCustom must be constructible with common::TaskReference "
                  "and Args...");

    if (task_index_ == size) {
      stacks_.emplace_back(size);
      task_index_ = 0;
      stack_index_++;
    }
    // std::cout << "Push task " << owning_thread_ << " " << stack_index_ << " "
    //           << task_index_ << "\n";
    overrideTask(stacks_[stack_index_][task_index_++], parent_ref,
                 std::forward<Args>(args)...);
    return std::make_tuple(owning_thread_, stack_index_, task_index_ - 1);
  }

  template <typename... Args>
  void create_task_at(common::TaskReference task_place,
                      common::TaskReference parent_ref, Args &&...args) {
    overrideTask(stacks_[std::get<1>(task_place)][std::get<2>(task_place)],
                 parent_ref, std::forward<Args>(args)...);
  }

  void clear() {
    stack_index_ = 0;
    task_index_ = 0;
    stacks_.clear();
    stacks_.emplace_back(size);
  }

  std::vector<Task_v2<TaskContext>> &operator[](std::size_t stack_index) {
    assert(stack_index <= stack_index_);
    return stacks_[stack_index];
  }
  std::vector<std::vector<Task_v2<TaskContext>>> stacks_;

private:
  size_t stack_index_ = 0;
  size_t task_index_ = 0;
  const int owning_thread_;

  template <typename... Args>
  void overrideTask(Task_v2<TaskContext> &task,
                    common::TaskReference parent_ref, Args &&...args) {
    task.context = TaskContext(std::forward<Args>(args)...);
    task.parentTask = parent_ref;
    task.task_group.total_tasks_ = 0;
    task.task_group.finished_tasks_ = 0;
  }
};