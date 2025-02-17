#pragma once

#include "../common.hpp"
#include <cassert>
#include <cstddef>
#include <deque>
#include <mutex>
#include <tuple>
// TODO use a cyclic array with a lock for the push and a atomic for bottom and
// top pop compares bottom and top if equal return empty else take the element
// at top and increment push takes a lock, stores the value and increments
// bottom ( resize can be done in the lock and should be doable without blocking
// the pop)
// ( main load is on the pop operation so it should not be blocked)
// if you want to be really fancy one could add a support structure that takes
// the elements that want to be pushed while locked? but thats complicated and
// moost likely not worth the effort
template <size_t reuse_tasks_batch_size, size_t free_tasks_batch_size>
class ConcurrentDeque_v2 {
public:
  void push(common::TaskReference task) {
    std::lock_guard<std::mutex> lock(mutex_);
    deque_.emplace_back(task);
  }
  void pushBatch(std::vector<common::TaskReference> tasks) {
    std::lock_guard<std::mutex> lock(mutex_);
    // #pragma omp
    //     for (auto task : tasks) {
    //       deque_.emplace_back(task);
    //     }

    deque_.insert(deque_.end(), tasks.begin(),
                  tasks.end()); // Efficient batch insert ?
  }
  common::TaskReference pop() {
    std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
    if (!lock.owns_lock()) {
      return common::InvalidTaskReference; // Indicate lock not acquired
    } // currently disabled
    if (deque_.empty()) {
      return common::InvalidTaskReference; // Indicate empty
    }
    common::TaskReference value = deque_.front();
    deque_.pop_front();
    return value;
  }
  // concurrent counter for the size accessed by batches only => less contention
  // ??
  void popBatch(std::vector<common::TaskReference> &tasks) {
    assert(tasks.empty() && tasks.max_size() >= reuse_tasks_batch_size);
    std::unique_lock<std::mutex> lock(mutex_);
    if (deque_.size() < reuse_tasks_batch_size) {
      return; // Indicate empty
    }

    // test unrolling
    // for (std::size_t i = 0; i < reuse_tasks_batch_size; i++) {
    //   tasks.push_back(deque_.front());
    //   deque_.pop_front();
    // }
    // TODO try this batch remove???
    tasks.assign(
        std::make_move_iterator(deque_.begin()),
        std::make_move_iterator(deque_.begin() + reuse_tasks_batch_size));
    deque_.erase(deque_.begin(),
                 deque_.begin() + reuse_tasks_batch_size); // Remove batch
    //     if constexpr (reuse_tasks_batch_size <= 8) {
    // // Volles Unrolling für kleine Werte
    // #pragma omp unroll
    //       for (std::size_t i = 0; i < reuse_tasks_batch_size; i++) {
    //         tasks[i] = deque_.front();
    //         deque_.pop_front();
    //       }
    //     } else {
    //       constexpr std::size_t unroll_factor = 8;
    //       std::size_t i = 0;

    // #pragma omp unroll
    //       for (; i + unroll_factor <= reuse_tasks_batch_size; i +=
    //       unroll_factor) {
    // #pragma omp simd
    //         for (std::size_t j = 0; j < unroll_factor; j++) {
    //           tasks.push_back(deque_.front());
    //           deque_.pop_front();
    //         }
    //       }

    //       for (; i < reuse_tasks_batch_size; i++) {
    //         tasks.push_back(deque_.front());
    //         deque_.pop_front();
    //       }
    //     }
  }
  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    deque_.clear();
  }

private:
  std::deque<common::TaskReference> deque_;
  std::mutex mutex_;
};
