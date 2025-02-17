#pragma once
#include "../common.hpp"
#include "task_group_v2.hpp"

template <typename TaskContext> class Task_v2 {
public:
  TaskGroup_v2<TaskContext> task_group;
  common::TaskReference parentTask;
  TaskContext context;

  template <typename... Args>
  Task_v2(common::TaskReference parent, Args &&...args)
      : task_group(), parentTask(parent), context(std::forward<Args>(args)...) {
  }
  Task_v2(Task_v2<TaskContext> &&) = default;
  Task_v2(const Task_v2<TaskContext> &) = default;
  Task_v2(common::TaskReference parent, TaskContext &&context_)
      : task_group(), parentTask(parent), context(context_) {}
  Task_v2()
      : task_group(), parentTask(common::InvalidTaskReference), context(){};
};
