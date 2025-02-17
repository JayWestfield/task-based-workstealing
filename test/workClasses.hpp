#include "../src/oldVersions/v2_customAllocation/work_stealing_scheduler_v2.hpp"
#include "oldVersions/v1_sharedPointerBased/task_v1.hpp"
#include "oldVersions/v1_sharedPointerBased/v1_taskManager/ChaseLevOwnQueue_v1.hpp"
#include "oldVersions/v1_sharedPointerBased/work_stealing_scheduler_v1.hpp"
#include "oldVersions/v2_customAllocation/v2_taskManager/ChaseLevOwnQueue_v2.hpp"
#include "oldVersions/v3_finalVersion/task_v3.hpp"
#include "oldVersions/v3_finalVersion/v3_taskManager/ChaseLevOwnQueue_v3.hpp"
#include "oldVersions/v3_finalVersion/work_stealing_scheduler_v3.hpp"
#include "oldVersions/v3_finalVersion/work_stealing_scheduler_v3_1.hpp"
#include "releaseVersion/work_stealing_scheduler.hpp"
#include "structs/task.hpp"
#include <atomic>
#include <iostream>
#include <memory>
#include <vector>
struct TestContext {
  int node;
  int started;
};

class TestWorkClass {
public:
  TestWorkClass(std::vector<std::atomic<int>> &workTracker)
      : workTracker_(workTracker),
        scheduler(
            4,
            std::make_shared<Task_v1<TestContext>>(
                nullptr, 0, static_cast<std::uint64_t>(0)),
            [this]() { this->idleFunction(); },
            [this](std::shared_ptr<Task_v1<TestContext>> task) {
              return this->executeTask(task);
            }) {}
  void run() {
    scheduler.setBaseTask(std::make_shared<Task_v1<TestContext>>(
        nullptr, 0, static_cast<std::uint64_t>(0)));
    scheduler.runAndWait();
  }
  bool executeTask(std::shared_ptr<Task_v1<TestContext>> task) {
    auto &[node, started] = task->context;
    // std::cout << "Execute task " << node << " " << started << "\n";
    switch (started) {
    case 0: {
      if (workTracker_[node].load() != 0) {
        std::cerr << "Task is started twice\n";
        assert(false);
        return false;
      }
      workTracker_[node] = 1;
      started = 1;
      // Add child tasks dynamically
      if (node == 0) {
        task->addChild(1, 0);
        task->addChild(2, 0);
      } else if (node == 1) {
        task->addChild(3, 0);
        task->addChild(4, 0);
      } else if (node == 2) {
        task->addChild(5, 0);
        task->addChild(6, 0);
      }
      // for (auto &child : task->task_group.tasks_) {
      //   alltasks.push_back(child);
      // }
      scheduler.waitTask(task);
      return false;
    }
    case 1:

      if (workTracker_[node].load(std::memory_order_seq_cst) != 1) {
        std::cerr << "Task " << node
                  << " should already be started but is in state "
                  << workTracker_[node].load() << started << "\n";
        assert(false);

        return true;
      }
      std::atomic_thread_fence(std::memory_order_seq_cst);

      started = 2;
      workTracker_[node].store(2,
                               std::memory_order_seq_cst); // Mark as finished
      return true;
    default:
      std::cerr << "Unknown task state\n";
      assert(false);

      return false;
    }
  }
  // std::vector<std::shared_ptr<Task<TestContext>>> alltasks;

  void idleFunction() { std::this_thread::yield(); }

private:
  std::vector<std::atomic<int>> &workTracker_;
  WorkStealingScheduler_v1<TestContext, ChaseLevOwnQueue_v1<TestContext>>
      scheduler;
};
class TestWorkClassCustom {
public:
  TestWorkClassCustom(std::vector<std::atomic<int>> &workTracker)
      : workTracker_(workTracker), scheduler(
                                       4, [this]() { this->idleFunction(); },
                                       [this](common::TaskReference task) {
                                         return this->executeTask(task);
                                       }) {}
  void run() {
    scheduler.setBaseTaskArgs(0, static_cast<std::uint64_t>(0));
    scheduler.runAndWait();
  }
  bool executeTask(common::TaskReference taskReference) {
    auto &[node, started] = scheduler.accessTask(taskReference).context;
    // std::cout << "Execute task " << node << " " << started << "\n";
    switch (started) {
    case 0: {
      if (workTracker_[node].load() != 0) {
        std::cerr << "Task is started twice\n";
        assert(false);
        return false;
      }
      workTracker_[node] = 1;
      started = 1;
      // Add child tasks dynamically
      if (node == 0) {
        scheduler.addChild(taskReference, 1, 0);
        scheduler.addChild(taskReference, 2, 0);
      } else if (node == 1) {
        scheduler.addChild(taskReference, 3, 0);
        scheduler.addChild(taskReference, 4, 0);
      } else if (node == 2) {
        scheduler.addChild(taskReference, 5, 0);
        scheduler.addChild(taskReference, 6, 0);
      }
      // for (auto &child : task->task_group.tasks_) {
      //   alltasks.push_back(child);
      // }
      scheduler.waitTask(taskReference);
      return false;
    }
    case 1:

      if (workTracker_[node].load(std::memory_order_seq_cst) != 1) {
        std::cerr << "Task " << node
                  << " should already be started but is in state "
                  << workTracker_[node].load() << started << "\n";
        assert(false);

        return true;
      }
      started = 2;
      workTracker_[node].store(2,
                               std::memory_order_seq_cst); // Mark as finished
      return true;
    default:
      std::cerr << "Unknown task state\n";
      assert(false);

      return false;
    }
  }
  // std::vector<std::shared_ptr<Task<TestContext>>> alltasks;

  void idleFunction() { std::this_thread::yield(); }

private:
  std::vector<std::atomic<int>> &workTracker_;
  WorkStealingScheduler_v2<TestContext, ChaseLevOwnQueue_v2<TestContext>>
      scheduler;
};
template <typename TaskManager> class TestWorkClassCustom2 {
public:
  TestWorkClassCustom2(std::vector<std::atomic<int>> &workTracker, int maxNode)
      : workTracker_(workTracker), maxNode_(maxNode),
        scheduler(
            20, [this]() { this->idleFunction(); },
            [this](common::TaskReference task) {
              return this->executeTask(task);
            }) {}
  void run() {
    scheduler.setBaseTaskArgs(1, 0); // Start with node 1
    scheduler.runAndWait();
  }
  bool executeTask(common::TaskReference taskReference) {
    auto &[node, started] = scheduler.accessTask(taskReference).context;
    // std::cout << "Execute task " << node << " " << started << "\n";
    switch (started) {
    case 0: {
      if (workTracker_[node].load() != 0) {
        std::cerr << "Task is started twice\n";
        assert(false);
        return false;
      }
      workTracker_[node] = 1;
      started = 1;
      // Add child tasks dynamically
      if (2 * node + 1 < maxNode_) {
        // std::cout << "Add children " << 2 * node << " " << 2 * node + 1 <<
        // "\n";
        scheduler.addChild(taskReference, 2 * node, 0);
        scheduler.addChild(taskReference, 2 * node + 1, 0);
      }
      scheduler.waitTask(taskReference);
      return false;
    }
    case 1:

      if (workTracker_[node].load(std::memory_order_seq_cst) != 1) {
        std::cerr << "Task " << node
                  << " should already be started but is in state "
                  << workTracker_[node].load() << started << "\n";
        assert(false);

        return true;
      }
      started = 2;
      workTracker_[node].store(2,
                               std::memory_order_seq_cst); // Mark as finished
      return true;
    default:
      std::cerr << "Unknown task state\n";
      assert(false);

      return false;
    }
  }

  void idleFunction() { std::this_thread::yield(); }

private:
  std::vector<std::atomic<int>> &workTracker_;
  int maxNode_;
  WorkStealingScheduler_v2<TestContext, TaskManager> scheduler;
};
template <typename TaskManager> class TestWorkClassCustom3 {
public:
  TestWorkClassCustom3(std::vector<std::atomic<int>> &workTracker, int maxNode)
      : workTracker_(workTracker), maxNode_(maxNode),
        scheduler(
            20, [this]() { this->idleFunction(); },
            [this](Task_v3<TestContext> *task) {
              return this->executeTask(task);
            }) {}
  void run() {
    scheduler.setBaseTaskArgs(1, 0); // Start with node 1
    scheduler.runAndWait();
  }
  bool executeTask(Task_v3<TestContext> *taskReference) {
    auto &[node, started] = (*taskReference).context;
    // std::cout << "Execute task " << node << " " << started << "\n";
    switch (started) {
    case 0: {
      if (workTracker_[node].load() != 0) {
        std::cerr << "Task is started twice\n";
        assert(false);
        return false;
      }
      workTracker_[node] = 1;
      started = 1;
      // Add child tasks dynamically
      if (2 * node + 1 < maxNode_) {
        // std::cout << "Add children " << 2 * node << " " << 2 * node + 1 <<
        // "\n";
        scheduler.addChild(taskReference, 2 * node, 0);
        scheduler.addChild(taskReference, 2 * node + 1, 0);
      }
      scheduler.waitTask(taskReference);
      return false;
    }
    case 1:

      if (workTracker_[node].load(std::memory_order_seq_cst) != 1) {
        std::cerr << "Task " << node
                  << " should already be started but is in state "
                  << workTracker_[node].load() << started << "\n";
        assert(false);

        return true;
      }
      started = 2;
      workTracker_[node].store(2,
                               std::memory_order_seq_cst); // Mark as finished
      return true;
    default:
      std::cerr << "Unknown task state\n";
      assert(false);

      return false;
    }
  }

  void idleFunction() { std::this_thread::yield(); }

private:
  std::vector<std::atomic<int>> &workTracker_;
  int maxNode_;
  WorkStealingScheduler_v3_1<TestContext, TaskManager> scheduler;
};

template <typename TaskManager> class TestWorkClassDefault {
public:
  TestWorkClassDefault(std::vector<std::atomic<int>> &workTracker, int maxNode)
      : workTracker_(workTracker), maxNode_(maxNode),
        scheduler(
            20, [this]() { this->idleFunction(); },
            [this](Task<TestContext> *task) {
              return this->executeTask(task);
            }) {}
  void run() {
    scheduler.setBaseTaskArgs(1, 0); // Start with node 1
    scheduler.runAndWait();
  }
  bool executeTask(Task<TestContext> *taskReference) {
    auto &[node, started] = (*taskReference).context;
    // std::cout << "Execute task " << node << " " << started << "\n";
    switch (started) {
    case 0: {
      if (workTracker_[node].load() != 0) {
        std::cerr << "Task is started twice\n";
        assert(false);
        return false;
      }
      workTracker_[node] = 1;
      started = 1;
      // Add child tasks dynamically
      if (2 * node + 1 < maxNode_) {
        // std::cout << "Add children " << 2 * node << " " << 2 * node + 1 <<
        // "\n";
        scheduler.addChild(taskReference, 2 * node, 0);
        scheduler.addChild(taskReference, 2 * node + 1, 0);
      }
      scheduler.waitTask(taskReference);
      return false;
    }
    case 1:

      if (workTracker_[node].load(std::memory_order_seq_cst) != 1) {
        std::cerr << "Task " << node
                  << " should already be started but is in state "
                  << workTracker_[node].load() << started << "\n";
        assert(false);

        return true;
      }
      started = 2;
      workTracker_[node].store(2,
                               std::memory_order_seq_cst); // Mark as finished
      return true;
    default:
      std::cerr << "Unknown task state\n";
      assert(false);

      return false;
    }
  }

  void idleFunction() { std::this_thread::yield(); }

private:
  std::vector<std::atomic<int>> &workTracker_;
  int maxNode_;
  WorkStealingScheduler<TestContext, TaskManager> scheduler;
};