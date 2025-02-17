#pragma once

#include "../../common.hpp"
#include "../task_v1.hpp"
#include <atomic>
#include <cassert>
#include <cerrno>
#include <iostream>
#include <memory>
#include <vector>
template <typename TaskContext> class ChaseLevDeque_v1 {
public:
  ChaseLevDeque_v1(size_t capacity = 1024)
      : top_(0), bottom_(0), capacity_(capacity), mask(1023) {
    tasks_ = new std::shared_ptr<Task_v1<TaskContext>>[capacity];
  }

  ~ChaseLevDeque_v1() { delete[] tasks_; }
  void clear() { top_ = bottom_.load(); }
  void push(std::shared_ptr<Task_v1<TaskContext>> task) {
    size_t b = bottom_; //.load(std::memory_order_relaxed);
    size_t t = top_;    //.load(std::memory_order_acquire);

    if (b - t >= capacity_ && b > t) {
      resize();
    }
    // TODO capacity_ as a power of 2 => use a bitmask instead of modulo
    tasks_[b & mask] = std::move(task);
    bottom_.store(b + 1); //, std::memory_order_release);
    assert((bottom_ >= top_));
    assert((b + 1 == bottom_.load()));

    // if (!(bottom_ >= top_)) {
    //   std::cerr << "Error: " << "bottom_ >= top_" << std::endl;
    // }
    // if (!(b + 1 == bottom_.load())) {
    //   std::cerr << "Error: " << "t1 + tasks.size() == top.load()" <<
    //   std::endl;
    // }
  }

  void pushBatch(std::vector<std::shared_ptr<Task_v1<TaskContext>>> &tasks) {
    const size_t number_of_tasks = tasks.size();
    // auto b1 = bottom_.load(); //.load(std::memory_order_acquire);
    // for (auto &task : tasks) {
    //   // if (task == nullptr) {
    //   //   std::cout << "error nulpointerChild" << std::endl;
    //   // }
    //   push(std::move(task));
    // }
    // if (!(b1 + tasks.size() == bottom_.load())) {
    //   std::cerr << "Error: " << "b1 + tasks.size() == bottom_.load()"
    //             << std::endl;
    // // }
    size_t b = bottom_; //.load(std::memory_order_relaxed);
    size_t t = top_;    //.load(std::memory_order_acquire);

    if (b + tasks.size() - t >= capacity_ && b > t) {
      resize();
    }
#pragma omp simd
    for (size_t i = 0; i < number_of_tasks; ++i) {
      tasks_[(b + i) & mask] = std::move(tasks[i]);
    }
    // TODO capacity_ as a power of 2 => use a bitmask instead of modulo
    bottom_.store(b + number_of_tasks); //, std::memory_order_release);
    assert((bottom_ >= top_));
    assert((b + number_of_tasks == bottom_.load()));
  }

  std::shared_ptr<Task_v1<TaskContext>> pop() {
    size_t b = bottom_; //.load(std::memory_order_relaxed);
    // TODO why not use a fetchadd -1?
    if (b == 0) { // ✅ Prevent underflow
      return nullptr;
    }
    --b;
    bottom_.store(b); //, std::memory_order_relaxed);
    // TODO ensure this is correct
    size_t t = top_;    //.load(std::memory_order_acquire);
    if (t > b) {        // Queue empty
      bottom_.store(t); //, std::memory_order_relaxed);
      return nullptr;
    }

    // TODO capacity_ as a power of 2 => use a bitmask instead of modulo
    std::shared_ptr<Task_v1<TaskContext>> task = tasks_[b & mask];

    if (t == b) { // Last element, synchronize with stealers
      size_t new_t = t;
      if (!top_.compare_exchange_strong(new_t, t + 1)) {
        task = nullptr;
        // if (!(false)) {
        //   std::cerr << "Error: " << "contention end" << std::endl;
        // }
      }
      bottom_.store(t + 1); //, std::memory_order_relaxed);
    }
    // if (!(bottom_ >= top_)) {
    //   std::cerr << "Error: " << "bottom_ >= top_" << bottom_ << top_
    //             << std::endl;
    // }

    assert(bottom_ >= top_);
    return task;
  }

  std::shared_ptr<Task_v1<TaskContext>> steal() {
    size_t t = top_;    //.load(std::memory_order_acquire);
    size_t b = bottom_; //.load(std::memory_order_acquire);

    if (t >= b) { // Empty queue
      return nullptr;
    }

    std::shared_ptr<Task_v1<TaskContext>> task = tasks_[t & mask];

    if (!top_.compare_exchange_strong(t, t + 1)) {
      return nullptr; // Failed due to contention
    }

    // tasks_[t & mask] = nullptr;

    return task;
  }

private:
  void resize() {
    std::cout << "error resize not supported yet" << std::endl;
    assert(false); // TODO i think that is wrongly done
    size_t old_capacity = capacity_;
    size_t new_capacity = old_capacity * 2;

    auto *new_tasks = new std::shared_ptr<Task_v1<TaskContext>>[new_capacity];
    size_t new_mask = new_capacity - 1;
    for (size_t i = top_; //.load(std::memory_order_acquire);
         i < bottom_.load(); ++i) {
      new_tasks[i & new_mask] = std::move(tasks_[i & mask]);
    }

    delete[] tasks_;
    mask = new_mask;
    tasks_ = new_tasks;
    capacity_ = new_capacity;
  }

  std::atomic<size_t> top_, bottom_;
  size_t capacity_;
  std::shared_ptr<Task_v1<TaskContext>> *tasks_;
  size_t mask;
};

template <typename TaskContext> class ChaseLevOwnQueue_v1 {
public:
  ChaseLevOwnQueue_v1(uint64_t numThreads)
      : numThreads_(numThreads), taskQueues_(numThreads) {}

  void addTasks(std::vector<std::shared_ptr<Task_v1<TaskContext>>> &tasks) {
    taskQueues_[common::thread_index_].pushBatch(tasks);
  }

  void addTask(std::shared_ptr<Task_v1<TaskContext>> task) {
    taskQueues_[common::thread_index_].push(std::move(task));
  }

  void resumeTask(std::shared_ptr<Task_v1<TaskContext>> task) {
    taskQueues_[common::thread_index_].push(std::move(task));
  }
  void clear() { taskQueues_[common::thread_index_].clear(); }
  std::shared_ptr<Task_v1<TaskContext>> getTask() {
    auto task = taskQueues_[common::thread_index_].pop();
    if (task)
      return task;

    for (uint64_t i = common::thread_index_ + 1; i < numThreads_; ++i) {
      task = taskQueues_[i].steal();
      if (task)
        return task;
    }
    for (uint64_t i = 0; i < common::thread_index_; ++i) {
      task = taskQueues_[i].steal();
      if (task)
        return task;
    }

    return nullptr;
  }

private:
  uint64_t numThreads_;
  std::vector<ChaseLevDeque_v1<TaskContext>> taskQueues_;
};
