#pragma once

#include "../task_v1.hpp"
#include <memory>
#include <tbb/concurrent_queue.h>
#include <vector>

template <typename TaskContext> class tbbConcurrentQueueTaskManager_v1 {
public:
  tbbConcurrentQueueTaskManager_v1(uint64_t numThreads)
      : numThreads_(numThreads) {}

  void addTasks(std::vector<std::shared_ptr<Task_v1<TaskContext>>> tasks) {
    for (auto &task : tasks) {
      taskQueue_.push(std::move(task));
    }
  }
  void addTask(std::shared_ptr<Task_v1<TaskContext>> task) {
    taskQueue_.push(std::move(task));
  }
  void resumeTask(std::shared_ptr<Task_v1<TaskContext>> task) {
    taskQueue_.push(std::move(task));
  }
  void clear() {
    std::shared_ptr<Task_v1<TaskContext>> task;
    while (taskQueue_.try_pop(task)) {
    }
  }

  std::shared_ptr<Task_v1<TaskContext>> getTask() {
    std::shared_ptr<Task_v1<TaskContext>> task;
    if (taskQueue_.try_pop(task)) {
      return task;
    }
    return nullptr;
  }

private:
  uint64_t numThreads_;
  tbb::concurrent_queue<std::shared_ptr<Task_v1<TaskContext>>> taskQueue_;
};