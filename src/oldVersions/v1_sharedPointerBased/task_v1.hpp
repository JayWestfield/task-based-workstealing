#pragma once
#include "allocator.hpp"
#include "task_group_v1.hpp"
#include <memory>
#include <tbb/scalable_allocator.h>

template <typename TaskContext>
class Task_v1 : public std::enable_shared_from_this<Task_v1<TaskContext>> {
public:
  using AllocatorType = DefaultAllocator<Task_v1<TaskContext>>;

  TaskGroup_v1<TaskContext> task_group;
  std::shared_ptr<Task_v1<TaskContext>> parentTask;
  TaskContext context;

  template <typename... Args>
  Task_v1(std::shared_ptr<Task_v1<TaskContext>> parent, Args &&...args)
      : task_group(), parentTask(std::move(parent)),
        context(std::forward<Args>(args)...) {}

  template <typename... Args> void addChild(Args &&...args) {
    std::shared_ptr<Task_v1<TaskContext>> child =
        // std::make_shared<Task_v1<TaskContext>>(this->shared_from_this(),
        //                                     std::forward<Args>(args)...);
        std::allocate_shared<Task_v1<TaskContext>>(AllocatorType(),
                                                   this->shared_from_this(),
                                                   std::forward<Args>(args)...);
    task_group.addTask(child);
  }
};