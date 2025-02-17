#pragma once

#include "../common.hpp"
#include "concurrent_deque_better_v2.hpp"
#include "concurrent_deque_v2.hpp"
#include "segmented_stack_v2.hpp"
#include "task_v2.hpp"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>
// TODO think about setbaseTask and the first task ?
template <typename TaskContext, typename TaskManager, int segmentSize = 48>
class WorkStealingScheduler_v2 {

public:
  WorkStealingScheduler_v2(
      uint16_t numThreads, std::function<void()> idleFunction,
      std::function<bool(common::TaskReference)> executeTask)
      : numThreads_(numThreads), idleFunction_(std::move(idleFunction)),
        executeTask_(std::move(executeTask)), stopFlag_(true),
        taskManager_(numThreads), condition_(false),
        reusable_tasks_(numThreads) {
    for (uint16_t i = 0; i < numThreads_; ++i) {
      threads_.emplace_back([this, i]() { this->workerThread(i); });
      segmentedStacks_.emplace_back(i);
    }
  }

  ~WorkStealingScheduler_v2() {
    joinThreads_ = true;
    stop();
    {
      std::unique_lock<std::mutex> lock(mutex_);
      condition_ = true;
      conditionVar_.notify_all();
    }
    for (auto &thread : threads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  template <typename... Args> void setBaseTaskArgs(Args &&...args) {
    assert(stopFlag_);
    baseTaskArgs_ = TaskContext(std::forward<Args>(args)...);
    newBaseTask_ = true;
  }

  void resumeTask(common::TaskReference task) { taskManager_.resumeTask(task); }

  void cancelTask(common::TaskReference task) { endTask(task); }

  void restart() {
    stopFlag_ = false;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      condition_ = true;
      conditionVar_.notify_all();
    }
  }

  void runAndWait() {
    assert(threads_working_ == 0);

    stopFlag_ = false;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      condition_.store(true, std::memory_order_release);
      conditionVar_.notify_all();
    }

    {
      std::unique_lock<std::mutex> lock(end_mutex_);
      end_conditionVar_.wait(lock, [this] {
        conditionVar_.notify_all();
        return stopFlag_.load();
      });
      {
        std::unique_lock<std::mutex> inner_lock(mutex_);
        condition_.store(false, std::memory_order_release);
      }
      while (threads_working_ > 0) {
        std::this_thread::yield();
      }
      taskManager_.clear();

      for (std::size_t i = 0; i < numThreads_; ++i) {
        segmentedStacks_[i].clear();
        reusable_tasks_[i].clear();
      }
    }
  }

  void waitTask(common::TaskReference task_ref) {
    auto &task = segmentedStacks_[std::get<0>(task_ref)]
                     .stacks_[std::get<1>(task_ref)][std::get<2>(task_ref)];
    if (task.task_group.finished_tasks_.load(std::memory_order_acquire) ==
        task.task_group.total_tasks_.load(std::memory_order_acquire)) {
      taskManager_.resumeTask(task_ref);
    } else {
      auto tasks = task.task_group.tasks_;
      task.task_group.tasks_.clear();
      taskManager_.addTasks(tasks);
    }
  }

  void stop() {
    stopFlag_ = true;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      condition_ = false;
    }
    end_conditionVar_.notify_all();
  }

  template <typename... Args>
  void addChild(common::TaskReference parentTask, Args &&...args) {
    common::TaskReference taskTuple =
        reusable_tasks_[common::thread_index_].pop();
    if (common::isInvalidReference(taskTuple)) {
      taskTuple = segmentedStacks_[common::thread_index_].push_back(
          parentTask, std::forward<Args>(args)...);
    } else {
      segmentedStacks_[common::thread_index_].create_task_at(
          taskTuple, parentTask, std::forward<Args>(args)...);
    }
    accessTask(parentTask).task_group.addTask(taskTuple);
  }
  Task_v2<TaskContext> &accessTask(common::TaskReference taskTuple) {
    // std::cout << std::get<0>(taskTuple) << " " << std::get<1>(taskTuple) << "
    // "
    //           << std::get<2>(taskTuple) << "\n";
    return segmentedStacks_[std::get<0>(taskTuple)][std::get<1>(taskTuple)]
                           [std::get<2>(taskTuple)];
  }

private:
  void workerThread(uint16_t thread_index) {
    common::thread_index_ = thread_index;
    while (!joinThreads_) {
      {
        std::unique_lock<std::mutex> lock(mutex_);
        conditionVar_.wait(lock, [this] {
          return condition_.load(std::memory_order_acquire);
        });
        threads_working_++;
      }

      while (!stopFlag_) {
        if (thread_index == 0 && newBaseTask_) {
          addBaseTask();
        }
        common::TaskReference nextTask = taskManager_.getTask();
        if (!common::isInvalidReference(nextTask)) {
          // std::cout << "run task " << std::get<0>(nextTask) << " "
          //           << std::get<1>(nextTask) << " " << std::get<2>(nextTask)
          //           << "\n";
          runTask(nextTask);
        } else {
          this->idleFunction_();
        }
      }

      {
        threads_working_--;
        std::unique_lock<std::mutex> lock(end_mutex_);
        end_conditionVar_.notify_all();
      }
    }
  }

  void addBaseTask() {
    assert(newBaseTask_ == true);
    common::TaskReference baseTask =
        segmentedStacks_[common::thread_index_].push_back(
            common::InvalidTaskReference, baseTaskArgs_);

    newBaseTask_ = false;
    // ensure segmented Stack etc is clearedand resetted
    taskManager_.resumeTask(baseTask);
  }

  void runTask(common::TaskReference task_ref) {
    // assert(accessTask(task_ref).task_group.tasks_.empty());
    // assert(accessTask(task_ref).task_group.total_tasks_.load() ==
    //        accessTask(task_ref).task_group.finished_tasks_.load());

    bool completed = this->executeTask_(task_ref);
    if (completed) {
      endTask(task_ref);
    }
  }

  void endTask(common::TaskReference task_ref) {
    Task_v2<TaskContext> &task =
        segmentedStacks_[std::get<0>(task_ref)]
            .stacks_[std::get<1>(task_ref)][std::get<2>(task_ref)];
    auto parentRef = task.parentTask;
    if (!common::isInvalidReference(parentRef)) {
      Task_v2<TaskContext> &parent =
          segmentedStacks_[std::get<0>(parentRef)]
              .stacks_[std::get<1>(parentRef)][std::get<2>(parentRef)];
      if (parent.task_group.taskCompleted()) {
        // std::cout << "restart " << std::get<0>(parentRef) << " "
        //           << std::get<1>(parentRef) << " " << std::get<2>(parentRef)
        //           << "\n";
        taskManager_.addTask(parentRef);
      }
    } else {
      stop();
    }
    // add memory to reusable
    reusable_tasks_[std::get<0>(task_ref)].push(task_ref);
  }

  uint64_t numThreads_;
  std::function<void()> idleFunction_;
  std::function<bool(common::TaskReference)> executeTask_;
  std::vector<std::thread> threads_;
  std::atomic<bool> stopFlag_;
  TaskManager taskManager_;
  std::mutex mutex_;
  std::condition_variable conditionVar_;
  std::atomic<bool> condition_;
  std::atomic<bool> joinThreads_ = false;
  std::mutex end_mutex_;
  std::condition_variable end_conditionVar_;
  std::atomic<int> threads_working_ = 0;
  std::vector<SegmentedStack_v2<TaskContext, segmentSize>> segmentedStacks_;
  std::vector<LockFreeQueue_v2> reusable_tasks_;
  TaskContext baseTaskArgs_;
  bool newBaseTask_ = false;
};
