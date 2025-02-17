#pragma once
#include "../ws_common.hpp"
#include "task_group.hpp"
#include <cassert>

template <typename TaskContext> class Task {
public:
  TaskGroup task_group;
  Task<TaskContext> *parentTask;
  TaskContext context;

  template <typename... Args>
  Task(Task<TaskContext> *parent, Args &&...args)
      : task_group(), parentTask(parent), context(std::forward<Args>(args)...) {
  }
  Task(Task<TaskContext> &&) = default;
  Task(const Task<TaskContext> &) = default;
  Task(Task<TaskContext> *parent, TaskContext &&context_)
      : task_group(), parentTask(parent), context(context_) {}
  Task() noexcept : task_group(), parentTask(nullptr), context(){};
  template <typename... Args>
  void inline override(Task<TaskContext> *parent, Args &&...args) {
    assert(task_group.active_children_count_ == 0);
    parentTask = parent;
    context = TaskContext(std::forward<Args>(args)...);
  }
  Task &operator=(const Task &) = default;
  Task &operator=(Task &&) = default;
};
