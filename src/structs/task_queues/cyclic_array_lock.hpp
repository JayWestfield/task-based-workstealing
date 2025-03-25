#pragma once
#include <cassert>
#include <cstddef>
#include <mutex>
#include <vector>
// Maybe use a atomic bool for signalling empty to not aquire lock when empty in
// steal / pop
template <typename TaskReferenceType, TaskReferenceType InvalidTaskReference>
class CyclicArrayLock {

public:
  CyclicArrayLock(size_t capacity = 1024)
      : capacity_(capacity), top_(0), bottom_(0), mask(capacity - 1),
        tasks_(capacity) {
    // assert(std::has_single_bit<size_t>(capacity));
  }

  void clear() {
    top_ = 0;
    bottom_ = 0;
  }

  void push(TaskReferenceType task) {
    std::lock_guard<std::mutex> lock(mutex);
    size_t b = bottom_;
    size_t t = top_;
    const int size = static_cast<int>(b) - static_cast<int>(t);
    if (size + 1 >= static_cast<int>(capacity_)) {
      resize();
    }

    assert(!(size >= static_cast<int>(capacity_) - 1 && size > 0));
    tasks_[b & mask] = task;

    bottom_++;
    assert((bottom_ >= top_));
  }

  void pushBatch(std::vector<TaskReferenceType> &tasks) {
    const size_t number_of_tasks = tasks.size();
    std::lock_guard<std::mutex> lock(mutex);

    size_t b = bottom_;
    const int size = static_cast<int>(b) - static_cast<int>(top_);

    while (size + static_cast<int>(number_of_tasks) >=
            static_cast<int>(capacity_) &&
        size > 0) {
      resize();
    }
#pragma omp simd
    for (size_t i = 0; i < number_of_tasks; ++i) {
      tasks_[(b + i) & mask] = reinterpret_cast<TaskReferenceType>(tasks[i]);
    }
    bottom_ += number_of_tasks;
    assert((bottom_ >= top_));
    tasks.clear();
  }

  TaskReferenceType pop() {
    std::lock_guard<std::mutex> lock(mutex);
    const int size = static_cast<int>(bottom_) - static_cast<int>(top_);
    if (size == 0) {
      return InvalidTaskReference;
    }
    TaskReferenceType task = tasks_[--bottom_ & mask];
    return task;
  }

  TaskReferenceType steal() {

    std::unique_lock<std::mutex> lock(mutex, std::try_to_lock);
    if (!lock.owns_lock())
      return InvalidTaskReference;
    const int size = static_cast<int>(bottom_) - static_cast<int>(top_);
    if (size == 0) {
      return InvalidTaskReference;
    }

    TaskReferenceType task = tasks_[top_++ & mask];
    return task;
  }

private:
  void resize() {
    size_t new_capacity = capacity_ * 2;
    std::vector<TaskReferenceType> new_tasks(new_capacity);
    size_t new_mask = new_capacity - 1;
#pragma omp simd
    for (size_t i = top_; i < bottom_; ++i) {
      new_tasks[i & new_mask] = tasks_[i & mask];
    }
    tasks_ = std::move(new_tasks);
    capacity_ = new_capacity;
    mask = new_capacity - 1;
  }
  size_t capacity_;

  size_t top_, bottom_;
  size_t mask;

  std::vector<TaskReferenceType> tasks_;

  std::mutex mutex;
};
