#pragma once

#include "../../common.hpp"
#include "../task_v2.hpp"
#include <atomic>
#include <cerrno>
#include <cstddef>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

template <typename TaskContext> class ChaseLevDequeCustom_v2 {

public:
  ChaseLevDequeCustom_v2(size_t capacity = 1024)
      : capacity_(capacity), top_(0), bottom_(0), mask(capacity - 1) {
    tasks_ = new common::TaskReference[capacity];
  }

  ~ChaseLevDequeCustom_v2() { delete[] tasks_; }

  void clear() {
    top_.store(0, std::memory_order_relaxed);
    bottom_.store(0, std::memory_order_relaxed);
  }

  void push(common::TaskReference task) {
    size_t b = bottom_.load(std::memory_order_relaxed);
    size_t t = top_.load(std::memory_order_acquire);
    const int size = static_cast<int>(b) - static_cast<int>(t);
    if (size >= static_cast<int>(capacity_) - 1 && size > 0) {
      resize();
    }
    tasks_[b & mask] = task;
    std::atomic_thread_fence(
        std::memory_order_release); // Ensure the write to tasks_ is visible
                                    // before updating bottom_
    bottom_.store(b + 1, std::memory_order_release);
    assert((bottom_.load(std::memory_order_acquire) >=
            top_.load(std::memory_order_acquire)));
  }

  void pushBatch(std::vector<common::TaskReference> &tasks) {
    // for (auto task : tasks) {
    //   push(task);
    // }
    const size_t number_of_tasks = tasks.size();
    size_t b = bottom_.load(std::memory_order_relaxed);
    size_t t = top_.load(std::memory_order_acquire);
    const int size = static_cast<int>(b) - static_cast<int>(t);

    if (size + static_cast<int>(tasks.size()) >= static_cast<int>(capacity_) &&
        size > 0) {
      resize();
    }
#pragma omp simd
    for (size_t i = 0; i < number_of_tasks; ++i) {
      tasks_[(b + i) & mask] = tasks[i];
    }
    std::atomic_thread_fence(
        std::memory_order_release); // Ensure the writes to tasks_ are visible
                                    // before updating bottom_
    bottom_.store(b + number_of_tasks, std::memory_order_release);
    assert((bottom_.load(std::memory_order_acquire) >=
            top_.load(std::memory_order_acquire)));
  }

  common::TaskReference pop() {
    size_t b = bottom_.load(std::memory_order_relaxed) - 1;
    bottom_.store(b, std::memory_order_seq_cst);
    size_t t = top_.load(std::memory_order_seq_cst);
    const int size = static_cast<int>(b) - static_cast<int>(t);
    if (size < 0) {
      bottom_.store(t, std::memory_order_seq_cst);
      return common::InvalidTaskReference; // Indicate empty
    }

    common::TaskReference task = tasks_[b & mask];
    std::atomic_thread_fence(
        std::memory_order_acquire); // Ensure the read from tasks_ is consistent
    if (size > 0) {
      return task;
    }

    // error if the cas fails it loads the new value into t => wrong value for b
    size_t new_t = t;

    if (!top_.compare_exchange_strong(new_t, t + 1,
                                      std::memory_order_seq_cst)) {
      task = common::InvalidTaskReference; // Indicate empty
    }
    //  //concurrentWriteTaskRef("task return pop ", task, t, b);
    //  //concurrentWriteTaskRef("task return pop at t ", tasks_[t & mask], t,
    //  b);
    bottom_.store(t + 1, std::memory_order_seq_cst);

    assert(bottom_.load(std::memory_order_acquire) >=
           top_.load(std::memory_order_acquire));
    return task;
  }

  common::TaskReference steal() {
    size_t t = top_.load(std::memory_order_acquire);
    size_t b = bottom_.load(std::memory_order_acquire);
    const int size = static_cast<int>(b) - static_cast<int>(t);
    if (size <= 0) {
      return common::InvalidTaskReference; // Indicate empty
    }

    common::TaskReference task = tasks_[t & mask];
    assert(!common::isInvalidReference(task));
    auto new_t = t;
    if (!top_.compare_exchange_strong(new_t, t + 1,
                                      std::memory_order_seq_cst)) {
      //  std::cout << "contention " << t << std::endl;
      return common::InvalidTaskReference; // Indicate empty
    }
    //  //concurrentWriteTaskRef("task return steal ", task, t, b);
    //  //concurrentWriteTaskRef("task return steal at t ", tasks_[t & mask], t,
    //  b);

    return task;
  }

private:
  void resize() {
    std::cout << "error resize not supported yet" << std::endl;
    assert(false);
    size_t old_capacity = capacity_;
    size_t new_capacity = old_capacity * 2;

    auto *new_tasks = new common::TaskReference[new_capacity];
    std::size_t new_mask = new_capacity - 1;
    for (size_t i = top_.load(std::memory_order_relaxed);
         i < bottom_.load(std::memory_order_relaxed); ++i) {
      new_tasks[i & new_mask] = std::move(tasks_[i & mask]);
    }

    delete[] tasks_;
    mask = new_mask;
    tasks_ = new_tasks;
    capacity_ = new_capacity;
  }
  size_t capacity_;

  std::atomic<size_t> top_, bottom_;
  common::TaskReference *tasks_;
  size_t mask;
};

template <typename TaskContext> class ChaseLevOwnQueue_v2 {
public:
  ChaseLevOwnQueue_v2(uint64_t numThreads)
      : numThreads_(numThreads), taskQueues_(numThreads) {}

  void addTasks(std::vector<common::TaskReference> &tasks) {
    taskQueues_[common::thread_index_].pushBatch(tasks);
  }

  void addTask(common::TaskReference task) {
    taskQueues_[common::thread_index_].push(task);
  }

  void resumeTask(common::TaskReference task) {
    taskQueues_[common::thread_index_].push(task);
  }

  void clear() {
    for (size_t i = 0; i < numThreads_; ++i) {
      taskQueues_[i].clear();
    }
  }

  common::TaskReference getTask() {
    common::TaskReference task = taskQueues_[common::thread_index_].pop();
    if (!common::isInvalidReference(task))
      return task;

    for (uint64_t i = common::thread_index_ + 1; i < numThreads_; ++i) {
      task = taskQueues_[i].steal();
      if (!common::isInvalidReference(task))
        return task;
    }
    for (uint64_t i = 0; i < common::thread_index_; ++i) {
      task = taskQueues_[i].steal();
      if (!common::isInvalidReference(task))
        return task;
    }

    return common::InvalidTaskReference; // Indicate empty
  }

private:
  uint64_t numThreads_;
  std::vector<ChaseLevDequeCustom_v2<TaskContext>> taskQueues_;
};