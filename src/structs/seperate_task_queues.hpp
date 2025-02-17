#pragma once
#include "../ws_common.hpp"
template <typename TaskReferenceType, TaskReferenceType InvalidTaskReference,
          typename TaskQueue>
class seperateTaskQeues {
public:
  seperateTaskQeues(ws::ThreadIndexType numThreads)
      : numThreads_(numThreads), taskQueues_(numThreads) {}

  void inline addTasks(std::vector<TaskReferenceType> &tasks) {
    taskQueues_[ws::thread_index_].pushBatch(tasks);
  }

  void inline addTask(TaskReferenceType task) {
    taskQueues_[ws::thread_index_].push(task);
  }

  void inline resumeTask(TaskReferenceType task) {
    taskQueues_[ws::thread_index_].push(task);
  }

  void inline clear() { taskQueues_[ws::thread_index_].clear(); }

  TaskReferenceType getTask() {
    TaskReferenceType task = taskQueues_[ws::thread_index_].pop();
    if (task != InvalidTaskReference)
      return task;

    for (uint64_t i = ws::thread_index_ + 1; i < numThreads_; ++i) {
      task = taskQueues_[i].steal();
      if (task != InvalidTaskReference)
        return task;
    }
    for (uint64_t i = 0; i < ws::thread_index_; ++i) {
      task = taskQueues_[i].steal();
      if (task != InvalidTaskReference)
        return task;
    }
    return InvalidTaskReference;
  }

private:
  ws::ThreadIndexType numThreads_;
  std::vector<TaskQueue> taskQueues_;
};
