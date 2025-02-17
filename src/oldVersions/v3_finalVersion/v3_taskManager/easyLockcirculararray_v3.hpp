#pragma once

#include "../../common.hpp"
#include "../task_v3.hpp"
#include <array>
#include <atomic>
#include <cerrno>
#include <cstddef>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>
template <typename TaskContext> class CircularArray_v3 {

public:
  CircularArray_v3(size_t capacity = 1024)
      : capacity_(capacity), top_(0), bottom_(0), mask(capacity - 1),
        tasks_(capacity) {
    // tasks_.reserve(capacity);
    // , nullptr);
  }

  void clear() {
    top_ = 0;
    bottom_ = 0;
  }

  void push(Task_v3<TaskContext> *task) {
    // push and push batch are only from one queue => no lock?
    // std::lock_guard<std::mutex> lock(mutex);
    size_t b = bottom_;
    size_t t = top_;
    const int size = static_cast<int>(b) - static_cast<int>(t);
    if (size >= static_cast<int>(capacity_) - 1) {
      size_t new_capacity = capacity_ * 2;
      std::vector<Task_v3<TaskContext> *> new_tasks(new_capacity);
      size_t new_mask = new_capacity - 1;
#pragma omp simd
      for (size_t i = top_; i < bottom_; ++i) {
        new_tasks[i & new_mask] = tasks_[i & mask];
      }
      tasks_ = std::move(new_tasks);
      capacity_ = new_capacity;
      mask = new_capacity - 1;
    }
    assert(!(size >= static_cast<int>(capacity_) - 1 && size > 0));
    tasks_[b & mask] = task;

    bottom_++;
    assert((bottom_ >= top_));
  }

  void pushBatch() {
    // for (auto task : tasks) {
    //   push(task);
    // }

    const size_t number_of_tasks = common::tasks.size();
    std::lock_guard<std::mutex> lock(mutex);

    size_t b = bottom_;
    size_t t = top_;
    const int size = static_cast<int>(b) - static_cast<int>(t);
    // std::cout << "size " << size << " number_of_tasks " << number_of_tasks
    //           << std::endl;
    if (size + static_cast<int>(number_of_tasks) >=
            static_cast<int>(capacity_) &&
        size > 0) {
      size_t new_capacity = capacity_ * 2;
      std::vector<Task_v3<TaskContext> *> new_tasks(new_capacity);
      size_t new_mask = new_capacity - 1;
#pragma omp simd
      for (size_t i = top_; i < bottom_; ++i) {
        new_tasks[i & new_mask] = tasks_[i & mask];
      }
      tasks_ = std::move(new_tasks);
      capacity_ = new_capacity;
      mask = new_capacity - 1;
    }
#pragma omp simd
    for (size_t i = 0; i < number_of_tasks; ++i) {
      tasks_[(b + i) & mask] =
          reinterpret_cast<Task_v3<TaskContext> *>(common::tasks[i]);
    }

    bottom_ += number_of_tasks;
    assert((bottom_ >= top_));
    common::tasks.clear();
  }

  Task_v3<TaskContext> *pop() {
    std::lock_guard<std::mutex> lock(mutex);
    const int size = static_cast<int>(bottom_) - static_cast<int>(top_);
    if (size == 0) {
      return nullptr; // Indicate empty
    }
    Task_v3<TaskContext> *task = tasks_[--bottom_ & mask];
    return task;
  }

  Task_v3<TaskContext> *steal() {
    // if (bottom_ == top_)
    //   return nullptr;
    std::unique_lock<std::mutex> lock(mutex, std::try_to_lock);
    if (!lock.owns_lock())
      return nullptr;
    const int size = static_cast<int>(bottom_) - static_cast<int>(top_);
    if (size == 0) {
      return nullptr; // Indicate empty
    }

    Task_v3<TaskContext> *task = tasks_[top_++ & mask];
    return task;
  }

private:
  size_t capacity_;

  size_t top_, bottom_;
  size_t mask;
  std::vector<Task_v3<TaskContext> *> tasks_;

  std::mutex mutex;
};

template <typename TaskContext> class easyLockCircularArray_v3 {
public:
  easyLockCircularArray_v3(uint64_t numThreads)
      : numThreads_(numThreads), taskQueues_(numThreads) {}

  void addTasks() { taskQueues_[common::thread_index_].pushBatch(); }

  void addTask(Task_v3<TaskContext> *task) {
    taskQueues_[common::thread_index_].push(task);
  }

  void resumeTask(Task_v3<TaskContext> *task) {
    taskQueues_[common::thread_index_].push(task);
  }

  void clear() {
    for (size_t i = 0; i < numThreads_; ++i) {
      taskQueues_[i].clear();
    }
  }

  Task_v3<TaskContext> *getTask() {
    Task_v3<TaskContext> *task = taskQueues_[common::thread_index_].pop();
    if (task != nullptr)
      return task;

    for (uint64_t i = common::thread_index_ + 1; i < numThreads_; ++i) {
      task = taskQueues_[i].steal();
      if (task != nullptr)
        return task;
    }
    for (uint64_t i = 0; i < common::thread_index_; ++i) {
      task = taskQueues_[i].steal();
      if (task != nullptr)
        return task;
    }

    return nullptr; // Indicate empty
  }

private:
  uint64_t numThreads_;
  std::vector<CircularArray_v3<TaskContext>> taskQueues_;
};