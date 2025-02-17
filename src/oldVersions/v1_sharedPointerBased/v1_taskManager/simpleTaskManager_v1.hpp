#pragma once

#include "../task_v1.hpp"
#include <memory>
#include <mutex>
#include <queue>

template <typename TaskContext> class SimpleTaskManager_v1 {
public:
  SimpleTaskManager_v1(uint64_t numThreads)
      : numThreads_(numThreads), taskQueue_(), queueMutex_() {}
  void clear() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    while (!taskQueue_.empty()) {
      taskQueue_.pop();
    }
  }
  void addTasks(std::vector<std::shared_ptr<Task_v1<TaskContext>>> tasks) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    for (auto &task : tasks) {
      taskQueue_.push(std::move(task));
    }
  }
  void addTask(std::shared_ptr<Task_v1<TaskContext>> task) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    taskQueue_.push(std::move(task));
  }
  void resumeTask(std::shared_ptr<Task_v1<TaskContext>> task) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    taskQueue_.push(std::move(task));
  }

  std::shared_ptr<Task_v1<TaskContext>> getTask() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (taskQueue_.empty()) {
      return nullptr;
    }
    auto task = std::move(taskQueue_.front());

    taskQueue_.pop();
    return task;
  }

private:
  uint64_t numThreads_;
  std::queue<std::shared_ptr<Task_v1<TaskContext>>> taskQueue_;
  std::mutex queueMutex_;
};