#pragma once

#include "../structs/segmented_stack.hpp"
#include "../structs/task.hpp"
#include "../ws_common.hpp"
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
template <typename TaskContext, typename TaskManager, size_t segmentSize = 48>
class WorkStealingScheduler {

public:
  WorkStealingScheduler(uint16_t numThreads, std::function<void()> idleFunction,
                        std::function<bool(Task<TaskContext> *)> executeTask)
      : numThreads_(numThreads), idleFunction_(std::move(idleFunction)),
        executeTask_(std::move(executeTask)), stopFlag_(true),
        taskManager_(numThreads), condition_(false) {
    for (uint16_t i = 0; i < numThreads_; ++i) {
      threads_.emplace_back([this, i]() {
        ws::pin_thread(i);
        this->workerThread(i);
      });
      segmentedStacks_.emplace_back(i);
    }
  }

  ~WorkStealingScheduler() {
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

  void resumeTask(Task<TaskContext> *task) { taskManager_.resumeTask(task); }

  void cancelTask(Task<TaskContext> *task) { endTask(task); }

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
      }
    }
  }

  void waitTask(Task<TaskContext> *task_ref) {
    auto &task = (*task_ref);
    if (task.task_group.active_children_count_.load(
            std::memory_order_acquire) == 0) {
      taskManager_.resumeTask(task_ref);
    } else {
      assert(ws::tasks.size() > 0);
      assert(ws::nextTask == nullptr);
      ws::nextTask = ws::tasks.back();
      ws::tasks.pop_back();
      taskManager_.addTasks(
          *reinterpret_cast<std::vector<Task<TaskContext> *> *>(&ws::tasks));
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
  void addChild(Task<TaskContext> *parentTask, Args &&...args) {

    Task<TaskContext> *taskPointer = nullptr;

    if (!ws::ownReusableTasks.empty()) {
      taskPointer =
          reinterpret_cast<Task<TaskContext> *>(ws::ownReusableTasks.back());
      ws::ownReusableTasks.pop_back();
      (*taskPointer).override(parentTask, std::forward<Args>(args)...);
      // taskPointer = taskPointer =
      //     segmentedStacks_[ws::thread_index_].create_task_at(
      //         taskTuple, parentTask, std::forward<Args>(args)...);
    } else {
      taskPointer = segmentedStacks_[ws::thread_index_].push_back(
          parentTask, std::forward<Args>(args)...);
    }
    parentTask->task_group.addTask();
    ws::tasks.push_back(taskPointer);
  }
  // deprecated ( use  pointers only)
  Task<TaskContext> &accessTask(ws::TaskReference taskTuple) {
    return segmentedStacks_[std::get<0>(taskTuple)][std::get<1>(taskTuple)]
                           [std::get<2>(taskTuple)];
  }

private:
  void workerThread(uint16_t thread_index) {
    ws::thread_index_ = thread_index;
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
          Task<TaskContext> *nextTask =
              reinterpret_cast<Task<TaskContext> *>(ws::nextTask);
          ws::nextTask = nullptr;
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
        ws::ownReusableTasks.clear();
        threads_working_--;
        std::unique_lock<std::mutex> lock(end_mutex_);
        end_conditionVar_.notify_all();
      }
    }
  }

  void addBaseTask() {
    assert(newBaseTask_ == true);
    Task<TaskContext> *baseTask =
        segmentedStacks_[ws::thread_index_].push_back(nullptr, baseTaskArgs_);

    newBaseTask_ = false;
    // ensure segmented Stack etc is cleared and resetted
    taskManager_.resumeTask(baseTask);
  }

  void runTask(Task<TaskContext> *task_ref) {
    bool completed = this->executeTask_(task_ref);
    if (completed) {
      endTask(task_ref);
    }
  }

  void endTask(Task<TaskContext> *task_ref) {
    Task<TaskContext> &task = *task_ref;
    auto parentRef = task.parentTask;
    if (parentRef != nullptr) {
      Task<TaskContext> &parent = (*parentRef);
      if (parent.task_group.taskCompleted()) {
        taskManager_.addTask(&parent);
      }
    } else {
      stop();
    }
    // free task / make it reusable
    ws::ownReusableTasks.push_back(task_ref);
  }

  uint16_t numThreads_;
  std::function<void()> idleFunction_;
  std::function<bool(Task<TaskContext> *)> executeTask_;
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
  std::vector<SegmentedStack<Task<TaskContext>, segmentSize>> segmentedStacks_;
  TaskContext baseTaskArgs_;
  bool newBaseTask_ = false;
};
