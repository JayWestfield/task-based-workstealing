#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <pthread.h>
#include <sched.h>
#include <tuple>
#include <vector>

namespace ws {

// Define a type alias for task references
using ThreadIndexType = uint16_t;
using TaskReference = std::tuple<ThreadIndexType, uint16_t, uint32_t>;

// try the version with the struct that might be more readable

// Define a constant for an invalid task reference
constexpr TaskReference InvalidTaskReference = std::make_tuple(
    std::numeric_limits<uint16_t>::max(), std::numeric_limits<uint16_t>::max(),
    std::numeric_limits<uint32_t>::max());

constexpr bool inline isInvalidReference(const TaskReference &task) {
  return std::get<0>(task) == std::numeric_limits<uint16_t>::max();
}
// Define a thread-local variable for thread index
thread_local ThreadIndexType thread_index_ = 0;
thread_local std::vector<void *> tasks(0);
thread_local void *nextTask = nullptr;
thread_local std::vector<void *> ownReusableTasks(0);
void pin_thread(std::size_t core) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}
} // namespace ws