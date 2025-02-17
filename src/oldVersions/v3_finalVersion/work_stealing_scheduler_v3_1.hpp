#pragma once

#include "../v2_customAllocation/concurrent_deque_better_v2.hpp"
#include "../v2_customAllocation/concurrent_deque_v2.hpp"

#include "../common.hpp"
#include "segmented_stack_v3.hpp"
#include "task_v3.hpp"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>
// TODO think about setbaseTask and the first task ?
template <typename TaskContext, typename TaskManager, size_t segmentSize = 48,
          size_t reuse_tasks_batch_size = 20, size_t free_tasks_batch_size = 10>
class WorkStealingScheduler_v3_1 {

public:
  WorkStealingScheduler_v3_1(
      uint16_t numThreads, std::function<void()> idleFunction,
      std::function<bool(Task_v3<TaskContext> *)> executeTask)
      : numThreads_(numThreads), idleFunction_(std::move(idleFunction)),
        executeTask_(std::move(executeTask)), stopFlag_(true),
        taskManager_(numThreads), condition_(false),
        reusable_tasks_(numThreads) {
    for (uint16_t i = 0; i < numThreads_; ++i) {
      threads_.emplace_back([this, i]() {
        common::pin_thread(i);
        this->workerThread(i);
      });
      segmentedStacks_.emplace_back(i);
    }
  }

  ~WorkStealingScheduler_v3_1() {
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

  void resumeTask(Task_v3<TaskContext> *task) { taskManager_.resumeTask(task); }

  void cancelTask(Task_v3<TaskContext> *task) { endTask(task); }

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

  void waitTask(Task_v3<TaskContext> *task_ref) {
    auto &task = (*task_ref);
    if (task.task_group.finished_tasks_.load(std::memory_order_acquire) ==
        task.task_group.total_tasks_.load(std::memory_order_acquire)) {
      taskManager_.resumeTask(task_ref);
    } else {
      assert(common::tasks.size() > 0);
      assert(common::nextTask == nullptr);
      common::nextTask = common::tasks.back();
      common::tasks.pop_back();
      taskManager_.addTasks();
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
  void addChild(Task_v3<TaskContext> *parentTask, Args &&...args) {
    // common::TaskReference taskTuple =
    // reusable_tasks_[common::thread_index_].pop();
    Task_v3<TaskContext> *taskPointer = nullptr;

    if (!common::ownReusableTasks.empty()) {
      common::TaskReference taskTuple =
          common::ownReusableTasks.back(); // get last element
      common::ownReusableTasks.pop_back(); // remove last element
      taskPointer = segmentedStacks_[common::thread_index_].create_task_at(
          taskTuple, parentTask, std::forward<Args>(args)...);
    } else {
      reusable_tasks_[common::thread_index_].popBatch(common::ownReusableTasks);
      taskPointer = segmentedStacks_[common::thread_index_].push_back(
          parentTask, std::forward<Args>(args)...);
    }
    parentTask->task_group.addTask();
    common::tasks.push_back(taskPointer);
  }
  Task_v3<TaskContext> &accessTask(common::TaskReference taskTuple) {
    // std::cout << std::get<0>(taskTuple) << " " << std::get<1>(taskTuple) << "
    // "
    //           << std::get<2>(taskTuple) << "\n";
    return segmentedStacks_[std::get<0>(taskTuple)][std::get<1>(taskTuple)]
                           [std::get<2>(taskTuple)];
  }

private:
  void workerThread(uint16_t thread_index) {
    common::thread_index_ = thread_index;
    common::reusableTasks.resize(numThreads_);
    common::tasks.reserve(free_tasks_batch_size);
    for (uint16_t i = 0; i < numThreads_; i++) {
      common::reusableTasks[i].reserve(free_tasks_batch_size);
    }
    common::ownReusableTasks.reserve(reuse_tasks_batch_size);
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
        } else {
          Task_v3<TaskContext> *nextTask =
              reinterpret_cast<Task_v3<TaskContext> *>(common::nextTask);
          common::nextTask = nullptr;
          if (nextTask != nullptr) {
            runTask(nextTask);
          } else {
            nextTask = taskManager_.getTask();
            if (nextTask != nullptr) {
              // std::cout << "run task " << std::get<0>(nextTask) << " "
              //           << std::get<1>(nextTask) << " " <<
              //           std::get<2>(nextTask)
              //           << "\n";
              runTask(nextTask);
            } else {
              this->idleFunction_();
            }
          }
        }
      }
      {
        for (uint16_t i = 0; i < numThreads_; i++) {
          common::reusableTasks[i].clear();
        }
        common::ownReusableTasks.clear();
        threads_working_--;
        std::unique_lock<std::mutex> lock(end_mutex_);
        end_conditionVar_.notify_all();
      }
    }
  }

  void addBaseTask() {
    assert(newBaseTask_ == true);
    Task_v3<TaskContext> *baseTask =
        segmentedStacks_[common::thread_index_].push_back(nullptr,
                                                          baseTaskArgs_);

    newBaseTask_ = false;
    // ensure segmented Stack etc is clearedand resetted
    taskManager_.resumeTask(baseTask);
  }

  void runTask(Task_v3<TaskContext> *task_ref) {
    // assert(accessTask(task_ref).task_group.tasks_.empty());
    // assert(accessTask(task_ref).task_group.total_tasks_.load() ==
    //        accessTask(task_ref).task_group.finished_tasks_.load());

    bool completed = this->executeTask_(task_ref);
    if (completed) {
      endTask(task_ref);
    }
  }

  void endTask(Task_v3<TaskContext> *task_ref) {
    Task_v3<TaskContext> &task = *task_ref;
    auto parentRef = task.parentTask;
    if (parentRef != nullptr) {
      Task_v3<TaskContext> &parent = (*parentRef);
      if (parent.task_group.taskCompleted()) {
        // std::cout << "restart " << std::get<0>(parentRef) << " "
        //           << std::get<1>(parentRef) << " " <<
        //           std::get<2>(parentRef)
        //           << "\n";
        taskManager_.addTask(&parent);
      }
    } else {
      stop();
    }
    common::TaskReference taskTuple = task.selfReference;
    // add memory to reusable
    auto &reusable = common::reusableTasks[std::get<0>(taskTuple)];
    reusable.push_back(taskTuple);
    if (reusable.size() > free_tasks_batch_size) {
      reusable_tasks_[std::get<0>(taskTuple)].pushBatch(reusable);
      reusable.clear();
    }
    // reusable_tasks_[std::get<0>(taskTuple)].push(taskTuple);
  }

  uint16_t numThreads_;
  std::function<void()> idleFunction_;
  std::function<bool(Task_v3<TaskContext> *)> executeTask_;
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
  std::vector<SegmentedStack_v3<TaskContext, segmentSize>> segmentedStacks_;
  std::vector<ConcurrentDeque_v2<reuse_tasks_batch_size, free_tasks_batch_size>>
      reusable_tasks_;
  TaskContext baseTaskArgs_;
  bool newBaseTask_ = false;
};
