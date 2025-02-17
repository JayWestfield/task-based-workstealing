#pragma once

#include "../ws_common.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <pthread.h>
#include <sched.h>
#include <tuple>
#include <vector>
namespace common {

using ThreadIndexType = ws::ThreadIndexType;

using TaskReference = ws::TaskReference;
// try the version with the struct that might be more readable

// Define a constant for an invalid task reference
constexpr TaskReference InvalidTaskReference = ws::InvalidTaskReference;

constexpr bool inline isInvalidReference(const TaskReference &task) {
  return std::get<0>(task) == std::numeric_limits<ThreadIndexType>::max();
}
// Define a thread-local variable for thread index
thread_local ThreadIndexType thread_index_ = 0;
thread_local std::vector<void *> tasks(0);
thread_local void *nextTask = nullptr;
thread_local std::vector<std::vector<TaskReference>> reusableTasks(0);
thread_local std::vector<TaskReference> ownReusableTasks(0);
void pin_thread(std::size_t core) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}
} // namespace common
