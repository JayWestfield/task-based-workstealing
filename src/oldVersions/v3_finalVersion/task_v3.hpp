#pragma once
#include "../common.hpp"
#include "task_group_v3.hpp"

template <typename TaskContext> class Task_v3 {
public:
  TaskGroup_v3<TaskContext> task_group;
  Task_v3<TaskContext> *parentTask;
  common::TaskReference selfReference;
  TaskContext context;

  template <typename... Args>
  Task_v3(common::TaskReference selfReference_, Task_v3<TaskContext> *parent,
          Args &&...args)
      : task_group(), parentTask(parent), selfReference(selfReference_),
        context(std::forward<Args>(args)...) {}
  Task_v3(Task_v3<TaskContext> &&) = default;
  Task_v3(const Task_v3<TaskContext> &) = default;
  Task_v3(common::TaskReference selfReference_, Task_v3<TaskContext> *parent,
          TaskContext &&context_)
      : task_group(), parentTask(parent), selfReference(selfReference_),
        context(context_) {}
  Task_v3()
      : task_group(), parentTask(nullptr),
        selfReference(common::InvalidTaskReference), context(){};
};
