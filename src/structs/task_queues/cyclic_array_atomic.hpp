#pragma once
#include <atomic>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <vector>
template <typename TaskReferenceType, TaskReferenceType InvalidTaskReference>
class CyclicArrayAtomic {

public:
  CyclicArrayAtomic(size_t capacity = 1024)
      : capacity_(capacity), top_(0), bottom_(0), mask(capacity - 1),
        tasks_(capacity) {}

  void clear() {
    top_.store(0, std::memory_order_relaxed);
    bottom_.store(0, std::memory_order_relaxed);
  }

  void push(TaskReferenceType task) {
    size_t b = bottom_.load(std::memory_order_relaxed);
    size_t t = top_.load(std::memory_order_acquire);
    const int size = static_cast<int>(b) - static_cast<int>(t);
    if (size >= static_cast<int>(capacity_) - 1 && size > 0) {
      resize();
    }
    assert(!(size >= static_cast<int>(capacity_) - 1 && size > 0));
    tasks_[b & mask] = task;
    std::atomic_thread_fence(std::memory_order_release);
    bottom_.store(b + 1, std::memory_order_release);
    assert((bottom_.load(std::memory_order_acquire) >=
            top_.load(std::memory_order_acquire)));
  }

  void pushBatch(std::vector<TaskReferenceType> &tasks) {

    const size_t number_of_tasks = tasks.size();
    size_t b = bottom_.load(std::memory_order_relaxed);
    size_t t = top_.load(std::memory_order_acquire);
    const int size = static_cast<int>(b) - static_cast<int>(t);
    if (size + static_cast<int>(number_of_tasks) >=
            static_cast<int>(capacity_) &&
        size > 0) {
      resize();
    }
#pragma omp simd
    for (size_t i = 0; i < number_of_tasks; ++i) {
      tasks_[(b + i) & mask] = reinterpret_cast<TaskReferenceType>(tasks[i]);
    }
    std::atomic_thread_fence(std::memory_order_release);
    bottom_.store(b + number_of_tasks, std::memory_order_release);
    assert((bottom_.load(std::memory_order_acquire) >=
            top_.load(std::memory_order_acquire)));
    tasks.clear();
  }

  TaskReferenceType pop() {
    size_t b = bottom_.load(std::memory_order_relaxed) - 1;
    bottom_.store(b, std::memory_order_acquire);
    size_t t = top_.load(std::memory_order_acquire);

    const int size = static_cast<int>(b) - static_cast<int>(t);
    if (size < 0) {
      bottom_.store(t, std::memory_order_acquire);
      return InvalidTaskReference;
    }
    TaskReferenceType task = tasks_[b & mask];
    if (size > 0) {
      return task;
    }

    size_t new_t = t;

    if (!top_.compare_exchange_strong(new_t, t + 1)) {
      task = InvalidTaskReference;
    }

    bottom_.store(t + 1, std::memory_order_relaxed);

    assert(bottom_.load(std::memory_order_acquire) >=
           top_.load(std::memory_order_acquire));
    return task;
  }

  TaskReferenceType steal() {
    size_t t = top_.load(std::memory_order_acquire);
    size_t b = bottom_.load(std::memory_order_acquire);
    const int size = static_cast<int>(b) - static_cast<int>(t);
    if (size <= 0) {
      return InvalidTaskReference;
    }

    TaskReferenceType task = tasks_[t & mask];
    assert(task != InvalidTaskReference);
    auto new_t = t;
    if (!top_.compare_exchange_strong(new_t, t + 1,
                                      std::memory_order_seq_cst)) {
      return InvalidTaskReference;
    }
    return task;
  }

private:
  void resize() { throw std::runtime_error("error resize not supported"); }
  size_t capacity_;

  std::atomic<size_t> top_, bottom_;
  size_t mask;
  std::vector<TaskReferenceType> tasks_;
};