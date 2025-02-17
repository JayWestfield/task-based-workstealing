#pragma once

#include "../../common.hpp"
#include <deque>
#include <mutex>
#include <vector>

template <typename TaskContext> class ownQueueCustom_v2 {
public:
  ownQueueCustom_v2(uint64_t numThreads)
      : numThreads_(numThreads), taskQueues_(numThreads),
        queueMutexes_(numThreads) {}

  void addTasks(std::vector<common::TaskReference> tasks) {
    std::lock_guard<std::mutex> lock(queueMutexes_[common::thread_index_]);

    for (auto &task : tasks) {
      taskQueues_[common::thread_index_].push_back(std::move(task));
    }
  }

  void addTask(common::TaskReference task) {
    std::lock_guard<std::mutex> lock(queueMutexes_[common::thread_index_]);
    taskQueues_[common::thread_index_].push_back(std::move(task));
  }

  void resumeTask(common::TaskReference task) {
    std::lock_guard<std::mutex> lock(queueMutexes_[common::thread_index_]);
    taskQueues_[common::thread_index_].emplace_back(task);
  }

  void clear() {
    // clear is only called when no one is working
    for (uint16_t i = 0; i < numThreads_; ++i) {
      taskQueues_[common::thread_index_].clear();
    }
  }

  common::TaskReference getTask() {
    {
      std::lock_guard<std::mutex> lock(queueMutexes_[common::thread_index_]);
      if (!taskQueues_[common::thread_index_].empty()) {
        auto task = std::move(taskQueues_[common::thread_index_].back());
        taskQueues_[common::thread_index_].pop_back();
        return task;
      }
    }

    for (uint64_t i = common::thread_index_ + 1; i < numThreads_; ++i) {
      if (!taskQueues_[i].empty()) {
        std::lock_guard<std::mutex> lock(queueMutexes_[i]);
        if (!taskQueues_[i].empty()) {
          auto task = std::move(taskQueues_[i].front());
          taskQueues_[i].pop_front();
          return task;
        }
      }
    }
    for (uint64_t i = 0; i < common::thread_index_; ++i) {
      if (!taskQueues_[i].empty()) {
        std::lock_guard<std::mutex> lock(queueMutexes_[i]);
        if (!taskQueues_[i].empty()) {
          auto task = std::move(taskQueues_[i].front());
          taskQueues_[i].pop_front();
          return task;
        }
      }
    }

    return common::InvalidTaskReference; // Indicate empty
  }

private:
  uint64_t numThreads_;
  std::vector<std::deque<common::TaskReference>> taskQueues_;
  std::vector<std::mutex> queueMutexes_;
};
