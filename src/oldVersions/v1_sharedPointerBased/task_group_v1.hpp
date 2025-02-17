#pragma once

#include <atomic>
#include <cassert>
#include <memory>
#include <vector>
template <typename TaskContext> class Task_v1;

template <typename TaskContext> class TaskGroup_v1 {
public:
  TaskGroup_v1() : total_tasks_(0), finished_tasks_(0), tasks_(0) {}

  template <typename... Args>
  void addTask(std::shared_ptr<Task_v1<TaskContext>> newTask) {
    total_tasks_.fetch_add(1, std::memory_order_release);
    tasks_.push_back(std::move(newTask));
  }

  /**
   * @brief unregisters a child task from the task group
   * @return true if it was the last child that the task group was waiting for
   */
  bool taskCompleted() {
    int total_tasks = total_tasks_.load(std::memory_order_acquire);
    // + 1 because fetch add returns the value before the addition
    int task_that_finished =
        finished_tasks_.fetch_add(1, std::memory_order_release) + 1;
    assert(task_that_finished <= total_tasks);

    return task_that_finished == total_tasks;
  }

  std::atomic<int> total_tasks_;
  std::atomic<int> finished_tasks_;
  std::vector<std::shared_ptr<Task_v1<TaskContext>>> tasks_;
};