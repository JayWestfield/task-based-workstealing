#pragma once

#include "../../common.hpp"
#include "../task_v1.hpp"
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

template <typename TaskContext> class ownQueue_v1 {
public:
  ownQueue_v1(uint64_t numThreads)
      : numThreads_(numThreads), taskQueues_(numThreads),
        queueMutexes_(numThreads) {}

  void addTasks(std::vector<std::shared_ptr<Task_v1<TaskContext>>> tasks) {
    std::lock_guard<std::mutex> lock(queueMutexes_[common::thread_index_]);

    for (auto &task : tasks) {
      taskQueues_[common::thread_index_].push_back(std::move(task));
    }
  }

  void addTask(std::shared_ptr<Task_v1<TaskContext>> task) {
    std::lock_guard<std::mutex> lock(queueMutexes_[common::thread_index_]);
    taskQueues_[common::thread_index_].push_back(std::move(task));
  }

  void resumeTask(std::shared_ptr<Task_v1<TaskContext>> task) {
    std::lock_guard<std::mutex> lock(queueMutexes_[common::thread_index_]);
    taskQueues_[common::thread_index_].push_back(std::move(task));
  }
  void clear() {
    std::lock_guard<std::mutex> lock(queueMutexes_[common::thread_index_]);
    taskQueues_[common::thread_index_].clear();
  }
  std::shared_ptr<Task_v1<TaskContext>> getTask() {
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

    return nullptr;
  }

private:
  uint64_t numThreads_;
  std::vector<std::deque<std::shared_ptr<Task_v1<TaskContext>>>> taskQueues_;
  std::vector<std::mutex> queueMutexes_;
};