#pragma once

#include "../../common.hpp"
#include "../task_v3.hpp"
#include <atomic>
#include <cerrno>
#include <cstddef>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>
template <typename TaskContext> class ChaseLevDeque_v3 {

public:
  ChaseLevDeque_v3(size_t capacity = 1024)
      : capacity_(capacity), top_(0), bottom_(0), mask(capacity - 1) {
    tasks_ = new Task_v3<TaskContext> *[capacity];
  }

  ~ChaseLevDeque_v3() { delete[] tasks_; }

  void clear() {
    top_.store(0, std::memory_order_relaxed);
    bottom_.store(0, std::memory_order_relaxed);
  }

  void push(Task_v3<TaskContext> *task) {
    size_t b = bottom_.load(std::memory_order_relaxed);
    size_t t = top_.load(std::memory_order_acquire);
    const int size = static_cast<int>(b) - static_cast<int>(t);
    if (size >= static_cast<int>(capacity_) - 1 && size > 0) {
      resize();
    }
    assert(!(size >= static_cast<int>(capacity_) - 1 && size > 0));
    tasks_[b & mask] = task;
    std::atomic_thread_fence(
        std::memory_order_release); // Ensure the write to tasks_ is visible
                                    // before updating bottom_
    bottom_.store(b + 1, std::memory_order_release);
    assert((bottom_.load(std::memory_order_acquire) >=
            top_.load(std::memory_order_acquire)));
  }

  void pushBatch() {
    // for (auto task : tasks) {
    //   push(task);
    // }
    const size_t number_of_tasks = common::tasks.size();
    size_t b = bottom_.load(std::memory_order_relaxed);
    size_t t = top_.load(std::memory_order_acquire);
    const int size = static_cast<int>(b) - static_cast<int>(t);
    // std::cout << "size " << size << " number_of_tasks " << number_of_tasks
    //           << std::endl;
    if (size + static_cast<int>(number_of_tasks) >=
            static_cast<int>(capacity_) &&
        size > 0) {
      resize();
    }
#pragma omp simd
    for (size_t i = 0; i < number_of_tasks; ++i) {
      tasks_[(b + i) & mask] =
          reinterpret_cast<Task_v3<TaskContext> *>(common::tasks[i]);
    }
    std::atomic_thread_fence(
        std::memory_order_release); // Ensure the writes to tasks_ are visible
                                    // before updating bottom_
    bottom_.store(b + number_of_tasks, std::memory_order_release);
    assert((bottom_.load(std::memory_order_acquire) >=
            top_.load(std::memory_order_acquire)));
    common::tasks.clear();
  }

  Task_v3<TaskContext> *pop() {
    size_t b = bottom_.load(std::memory_order_relaxed) - 1;
    bottom_.store(b, std::memory_order_acquire);
    size_t t = top_.load(std::memory_order_acquire);

    const int size = static_cast<int>(b) - static_cast<int>(t);
    if (size < 0) {
      bottom_.store(t, std::memory_order_acquire);
      return nullptr; // Indicate empty
    }
    Task_v3<TaskContext> *task = tasks_[b & mask];

    // std::atomic_thread_fence(
    //     std::memory_order_acquire); // Ensure the read from tasks_ is
    //     consistent
    if (size > 0) {
      return task;
    }

    // error if the cas fails it loads the new value into t => wrong value for b
    size_t new_t = t;

    if (!top_.compare_exchange_strong(new_t, t + 1)) {
      task = nullptr; // Indicate empty
    }
    //  //concurrentWriteTaskRef("task return pop ", task, t, b);
    //  //concurrentWriteTaskRef("task return pop at t ", tasks_[t & mask], t,
    //  b);
    bottom_.store(t + 1, std::memory_order_relaxed);

    assert(bottom_.load(std::memory_order_acquire) >=
           top_.load(std::memory_order_acquire));
    return task;
  }

  Task_v3<TaskContext> *steal() {
    size_t t = top_.load(std::memory_order_acquire);
    size_t b = bottom_.load(std::memory_order_acquire);
    const int size = static_cast<int>(b) - static_cast<int>(t);
    if (size <= 0) {
      return nullptr; // Indicate empty
    }

    Task_v3<TaskContext> *task = tasks_[t & mask];
    assert(task != nullptr);
    auto new_t = t;
    if (!top_.compare_exchange_strong(new_t, t + 1,
                                      std::memory_order_seq_cst)) {
      //  std::cout << "contention " << t << std::endl;
      return nullptr; // Indicate empty
    }
    //  //concurrentWriteTaskRef("task return steal ", task, t, b);
    //  //concurrentWriteTaskRef("task return steal at t ", tasks_[t & mask], t,
    //  b);

    return task;
  }

private:
  void resize() { std::cout << "error resize not supported" << std::endl; }
  size_t capacity_;

  std::atomic<size_t> top_, bottom_;
  Task_v3<TaskContext> **tasks_;
  size_t mask;
};

template <typename TaskContext> class ChaseLevOwnQueue_v3 {
public:
  ChaseLevOwnQueue_v3(uint64_t numThreads)
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
  std::vector<ChaseLevDeque_v3<TaskContext>> taskQueues_;
};