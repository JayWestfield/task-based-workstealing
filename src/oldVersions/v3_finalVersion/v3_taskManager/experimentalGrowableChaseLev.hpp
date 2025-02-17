#pragma once
#include "../task_v3.hpp"
template <typename TaskContext> struct CircularArray {
  Task_v3<TaskContext> **tasks_;
  size_t mask;
  size_t capacity_;

  CircularArray(size_t capacity)
      : tasks_(new Task_v3<TaskContext> *[capacity]), mask(capacity - 1),
        capacity_(capacity) {}

  ~CircularArray() { delete[] tasks_; }

  void put(size_t index, Task_v3<TaskContext> *task) {
    tasks_[index & mask] = task;
  }

  Task_v3<TaskContext> *get(size_t index) { return tasks_[index & mask]; }

  CircularArray *grow(size_t b, size_t t) {
    size_t new_capacity = capacity_ * 2;
    CircularArray *new_array = new CircularArray(new_capacity);

    for (size_t i = t; i < b; ++i) {
      new_array->put(i, tasks_[i & mask]);
    }

    return new_array;
  }
};

template <typename TaskContext> class ChaseLevDequeexperimental_v3 {

public:
  ChaseLevDequeexperimental_v3(size_t capacity = 1024)
      : top_(0), bottom_(0), tasks_(new CircularArray<TaskContext>(capacity)) {}

  ~ChaseLevDequeexperimental_v3() { delete tasks_; }

  void clear() {
    top_.store(0, std::memory_order_relaxed);
    bottom_.store(0, std::memory_order_relaxed);
  }

  void push(Task_v3<TaskContext> *task) {
    size_t b = bottom_.load(std::memory_order_relaxed);
    size_t t = top_.load(std::memory_order_acquire);
    const int size = static_cast<int>(b) - static_cast<int>(t);

    if (size >= static_cast<int>(tasks_->capacity_) - 1 && size > 0) {
      resize();
    }

    tasks_->put(b, task);
    std::atomic_thread_fence(
        std::memory_order_release); // Ensure task is visible before updating
                                    // bottom_
    bottom_.store(b + 1, std::memory_order_release);
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
#pragma omp simd
    for (size_t i = 0; i < number_of_tasks; ++i) {
      tasks_->put(b + i,
                  reinterpret_cast<Task_v3<TaskContext> *>(common::tasks[i]));
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

    Task_v3<TaskContext> *task = tasks_->get(b);
    if (size > 0) {
      return task;
    }

    size_t new_t = t;
    if (!top_.compare_exchange_strong(new_t, t + 1)) {
      task = nullptr; // Indicate empty
    }
    bottom_.store(t + 1, std::memory_order_relaxed);

    return task;
  }

  Task_v3<TaskContext> *steal() {
    size_t t = top_.load(std::memory_order_acquire);
    size_t b = bottom_.load(std::memory_order_acquire);
    const int size = static_cast<int>(b) - static_cast<int>(t);

    if (size <= 0) {
      return nullptr; // Indicate empty
    }

    Task_v3<TaskContext> *task = tasks_->get(t);
    size_t new_t = t;
    if (!top_.compare_exchange_strong(new_t, t + 1)) {
      return nullptr; // Indicate empty
    }

    return task;
  }

private:
  void resize() {
    CircularArray<TaskContext> *new_array =
        tasks_->grow(bottom_.load(std::memory_order_relaxed),
                     top_.load(std::memory_order_acquire));

    // Atomically replace the current array with the new one
    tasks_ = new_array;
  }

  std::atomic<size_t> top_, bottom_;
  CircularArray<TaskContext> *tasks_;
};
template <typename TaskContext> class ChaseLevOwnQueue_v3experimental {
public:
  ChaseLevOwnQueue_v3experimental(uint64_t numThreads)
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
  std::vector<ChaseLevDequeexperimental_v3<TaskContext>> taskQueues_;
};