#pragma once

#include "../../common.hpp"
#include "../task_v3.hpp"
#include <deque>
#include <mutex>
#include <vector>

template <typename TaskContext> class ownQueue_v3 {
public:
  ownQueue_v3(uint64_t numThreads)
      : numThreads_(numThreads), taskQueues_(numThreads),
        queueMutexes_(numThreads) {}

  void addTasks() {
    std::lock_guard<std::mutex> lock(queueMutexes_[common::thread_index_]);
    for (auto task : common::tasks) {
      taskQueues_[common::thread_index_].push_back(
          reinterpret_cast<Task_v3<TaskContext> *>(task));
    }
    common::tasks.clear();
  }

  void addTask(Task_v3<TaskContext> *task) {
    std::lock_guard<std::mutex> lock(queueMutexes_[common::thread_index_]);
    taskQueues_[common::thread_index_].push_back(task);
  }

  void resumeTask(Task_v3<TaskContext> *task) {
    std::lock_guard<std::mutex> lock(queueMutexes_[common::thread_index_]);
    taskQueues_[common::thread_index_].push_back(task);
  }

  void clear() {
    for (uint64_t i = 0; i < numThreads_; ++i) {
      std::lock_guard<std::mutex> lock(queueMutexes_[i]);
      taskQueues_[i].clear();
    }
  }

  Task_v3<TaskContext> *getTask() {
    // Try to get a task from the local queue
    {
      std::lock_guard<std::mutex> lock(queueMutexes_[common::thread_index_]);
      if (!taskQueues_[common::thread_index_].empty()) {
        auto task = taskQueues_[common::thread_index_].back();
        taskQueues_[common::thread_index_].pop_back();
        return task;
      }
    }

    // Steal work from other threads
    for (uint64_t i = common::thread_index_ + 1; i < numThreads_; ++i) {
      std::lock_guard<std::mutex> lock(queueMutexes_[i]);
      if (!taskQueues_[i].empty()) {
        auto task = taskQueues_[i].front();
        taskQueues_[i].pop_front();
        return task;
      }
    }
    for (uint64_t i = 0; i < common::thread_index_; ++i) {
      std::lock_guard<std::mutex> lock(queueMutexes_[i]);
      if (!taskQueues_[i].empty()) {
        auto task = taskQueues_[i].front();
        taskQueues_[i].pop_front();
        return task;
      }
    }

    return nullptr; // Indicate empty
  }

private:
  uint64_t numThreads_;
  std::vector<std::deque<Task_v3<TaskContext> *>> taskQueues_;
  std::vector<std::mutex> queueMutexes_;
};
