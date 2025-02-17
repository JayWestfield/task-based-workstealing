#pragma once

#include "../common.hpp"
#include "task_group_v1.hpp"
#include "task_v1.hpp"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
// TODO add a add BaseTask Function wich adds the base task to thread with index
// one threadsafe
// maybe provide a condition varaible to stop threads without busy waiting/ the
// option to not terminate them
template <typename TaskContext, typename TaskManager>
class WorkStealingScheduler_v1 {
public:
  WorkStealingScheduler_v1(
      uint16_t numThreads, std::shared_ptr<Task_v1<TaskContext>> baseTask,
      std::function<void()> idleFunction,
      std::function<bool(std::shared_ptr<Task_v1<TaskContext>>)> executeTask)
      : numThreads_(numThreads), baseTask_(std::move(baseTask)),
        idleFunction_(std::move(idleFunction)),
        executeTask_(std::move(executeTask)), stopFlag_(false),
        taskManager_(numThreads), condition_(false) {
    for (uint16_t i = 0; i < numThreads_; ++i) {
      threads_.emplace_back([this, i]() { this->workerThread(i); });
    }
  }

  ~WorkStealingScheduler_v1() {
    joinThreads_ = true;
    stop();
    {
      std::unique_lock<std::mutex> lock(mutex_);
      condition_ = true;
      conditionVar_.notify_all();
    }
    // std::cout << "Joining threads\n";
    for (auto &thread : threads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  void resumeTask(std::shared_ptr<Task_v1<TaskContext>> task) {
    taskManager_.resumeTask(std::move(task));
  }

  void cancelTask(std::shared_ptr<Task_v1<TaskContext>> task) {
    endTask(std::move(task));
  }
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
    // std::cout << "runAndWait\n";
    {
      std::unique_lock<std::mutex> lock(mutex_);
      condition_.store(true, std::memory_order_release);
      conditionVar_.notify_all();
    }
    // std::cout << "notified\n";

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
    }
  }
  void setBaseTask(std::shared_ptr<Task_v1<TaskContext>> baseTask) {
    assert(baseTask != nullptr);
    baseTask_ = std::move(baseTask);
    assert(baseTask_ != nullptr);
  }
  void waitTask(std::shared_ptr<Task_v1<TaskContext>> task) {
    if (task->task_group.finished_tasks_.load(std::memory_order_acquire) ==
        task->task_group.total_tasks_.load(std::memory_order_acquire)) {
      taskManager_.resumeTask(task);
    } else {
      auto tasks = task->task_group.tasks_;
      task->task_group.tasks_.clear();
      taskManager_.addTasks(tasks);
    }
  }

  void stop() {
    stopFlag_ = true;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      condition_ = false;
      // conditionVar_.notify_all();
    }
    end_conditionVar_.notify_all();
  }

private:
  void workerThread(uint16_t thread_index) {
    common::thread_index_ = thread_index;
    // atm just for the unused variable warning
    // while (!joinThreads_) {
    //   if (!stopFlag_) {
    //     std::cout << "thread_index: " << thread_index << std::endl;
    //     if (thread_index == 0) {
    //       // std::cout << "thread_index: " << thread_index << std::endl;
    //       // std::unique_lock<std::mutex> lock(mutex_);
    //       // conditionVar_.wait(lock, [this] { return condition_.load(); });
    //       taskManager_.resumeTask(baseTask_);
    //       // std::cout << " " << thread_index;
    //     }

    //     while (!stopFlag_) {
    //       std::shared_ptr<Task_v1<TaskContext>> nextTask =
    //           std::move(taskManager_.getTask());
    //       if (nextTask != nullptr)
    //         runTask(std::move(nextTask));
    //       else
    //         this->idleFunction_();
    //     }
    //   }
    //   // TODO cond variable waiting for stopFlag
    //   std::this_thread::yield();
    //   end_conditionVar_.notify_all();
    // }
    while (!joinThreads_) {
      {
        std::unique_lock<std::mutex> lock(mutex_);
        conditionVar_.wait(lock, [this] {
          return condition_.load(std::memory_order_acquire);
        });
        threads_working_++;
      }

      // if (stopFlag_.load()) {
      //   std::cout << "Stopping thread: " << thread_index << std::endl;
      //   break;
      // }

      while (!stopFlag_) {
        if (thread_index == 0 && baseTask_ != nullptr) {
          taskManager_.resumeTask(std::move(baseTask_));
          baseTask_ = nullptr;
        }
        std::shared_ptr<Task_v1<TaskContext>> nextTask = taskManager_.getTask();
        if (nextTask != nullptr) {
          runTask(std::move(nextTask));
        } else {
          this->idleFunction_();
        }
      }
      if (stopFlag_) {
        taskManager_.clear();
      }

      {
        threads_working_--;
        std::unique_lock<std::mutex> lock(end_mutex_);
        end_conditionVar_.notify_all();
      }
    }
  }

  void runTask(std::shared_ptr<Task_v1<TaskContext>> task) {
    bool completed = this->executeTask_(task);
    if (completed) {
      endTask(std::move(task));
    }
  }

  void endTask(std::shared_ptr<Task_v1<TaskContext>> task) {
    std::shared_ptr<Task_v1<TaskContext>> parent = task->parentTask;
    if (parent != nullptr) {
      if (parent->task_group.taskCompleted())
        taskManager_.addTask(std::move(parent));
    } else {
      // base task is the only task without a parent
      stop();
    }
  }

  uint64_t numThreads_;
  std::shared_ptr<Task_v1<TaskContext>> baseTask_;
  std::function<void()> idleFunction_;
  std::function<bool(std::shared_ptr<Task_v1<TaskContext>>)> executeTask_;
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
};