#pragma once
#include "../common.hpp"
#include <atomic>
#include <cassert>
#include <iostream>
#include <mutex>
#include <vector>

class concurrent_deque_better_v2 {
public:
  concurrent_deque_better_v2(size_t capacity = 1024)
      : capacity_(capacity), buffer_(capacity), bottom_(0), top_(0) {}

  void push(common::TaskReference task) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t b = bottom_.load(std::memory_order_relaxed);
    if (b - top_.load(std::memory_order_acquire) >= capacity_) {
      std::cout << "error resize not supported yet" << std::endl;
      assert(false);
      return;
    }
    buffer_[b % capacity_] = task;
    // TODO check i might need the atomic thread fence here
    bottom_.store(b + 1, std::memory_order_release);
  }

  common::TaskReference pop() {
    size_t t = top_.load(std::memory_order_relaxed);
    if (t >= bottom_.load(std::memory_order_acquire)) {
      return common::InvalidTaskReference; // Queue empty
    }
    common::TaskReference task = buffer_[t % capacity_];
    top_.store(t + 1, std::memory_order_release);
    return task;
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    bottom_.store(0, std::memory_order_relaxed);
    top_.store(0, std::memory_order_relaxed);
  }

private:
  size_t capacity_;
  std::vector<common::TaskReference> buffer_;
  std::mutex mutex_;
  std::atomic<size_t> bottom_;
  std::atomic<size_t> top_;
};

#include <atomic>
#include <vector>
// TODO mask instead of modulo?
class LockFreeQueue_v2 {
public:
  LockFreeQueue_v2(size_t capacity = 1024000)
      : capacity_(capacity), buffer_(capacity), ready_(capacity), bottom_(0),
        top_(0) {
    for (auto &flag : ready_) {
      flag.store(false, std::memory_order_relaxed);
    }
  }
  void clear() {
    bottom_ = 0;
    top_ = 0;
    for (auto &flag : ready_) {
      flag.store(false, std::memory_order_relaxed);
    }
  }
  void push(common::TaskReference task) {
    size_t b =
        bottom_.fetch_add(1, std::memory_order_relaxed); // Reserviere Index

    if (b - top_.load(std::memory_order_relaxed) >= capacity_) {
      bottom_.fetch_sub(1, std::memory_order_relaxed); // Rückgängig machen
      std::cout << "cannot push due to lack in Size" << std::endl;
      return;
    }

    buffer_[b % capacity_] = task; // Speichere Task
    ready_[b % capacity_].store(
        true, std::memory_order_release); // Markiere als bereit
  }

  common::TaskReference pop() {
    size_t t = top_.load(std::memory_order_relaxed);
    if (t >= bottom_.load(std::memory_order_acquire) ||
        !ready_[t % capacity_].load(std::memory_order_acquire)) {
      return common::InvalidTaskReference; // Queue leer
    }

    common::TaskReference task = buffer_[t % capacity_];
    ready_[t % capacity_].store(
        false, std::memory_order_relaxed); // Slot wieder freigeben
    top_.store(t + 1, std::memory_order_release);
    return task;
  }

private:
  size_t capacity_;
  std::vector<common::TaskReference> buffer_;
  std::vector<std::atomic<bool>> ready_; // Signalisiert, ob Eintrag bereit ist
  std::atomic<size_t> bottom_;
  std::atomic<size_t> top_;
};
