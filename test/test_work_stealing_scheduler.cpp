
#include "oldVersions/v2_customAllocation/v2_taskManager/ChaseLevOwnQueue_v2.hpp"
#include "oldVersions/v2_customAllocation/v2_taskManager/ownQueue_v2.hpp"
#include "structs/seperate_task_queues.hpp"
#include "structs/task.hpp"
#include "structs/task_queues/cyclic_array_atomic.hpp"
#include "structs/task_queues/cyclic_array_lock.hpp"
#include "workClasses.hpp"
#include <atomic>
#include <cassert>
#include <iostream>
#include <memory>
#include <tuple>
#include <vector>
// TODO clean the tests and make sure all versions are tested
template <typename WorkClass> bool testTaskStartAfterParent() {
  const int numTasks = 7;
  std::vector<std::atomic<int>> workTracker(numTasks);
  WorkClass work(workTracker);

  for (int i = 0; i < 100; i++) {
    for (auto &tracker : workTracker) {
      tracker = 0; // Not started
    }
    // std::cout << "RunStd " << i << "\n";
    work.run();
    for (auto &tracker : workTracker) {
      if (tracker.load() != 2) {
        std::cerr << "Task not finished " << i << "\n";
        assert(false);
        return false;
      }
    }
  }
  std::cout << "finished a Version" << std::endl;
  return true;
}
template <typename WorkClass> bool testBinaryTreeWorkStealer(int maxNode) {
  std::vector<std::atomic<int>> workTracker(maxNode + 1); // Nodes are 1-based
  WorkClass work(workTracker, maxNode);

  for (int i = 0; i < 50; i++) {
    for (auto &tracker : workTracker) {
      tracker = 0; // Not started
    }
    // std::cout << "RunB " << i << "\n";
    work.run();
    for (int i = 1; i < maxNode - 1; i++) {
      if (workTracker[i].load() != 2) {
        std::cerr << "Task not finished " << i << "\n";
        assert(false);
        return false;
      }
    }
  }
  std::cout << "finished a Version" << std::endl;

  return true;
}
template <typename... Types> bool runTreeTests() {
  // This correctly expands the parameter pack with fold expression
  return (testBinaryTreeWorkStealer<Types>(30) && ...);
}
template <typename... Types> bool runSimpleTests() {
  // This correctly expands the parameter pack with fold expression
  return (testTaskStartAfterParent<Types>() && ...);
}
int main() {
  std::cout << "Start Testing" << std::endl;
  if (!runSimpleTests<TestWorkClass, TestWorkClassCustom>()) {
    return 1;
  }
  std::cout << "Finish SimpleTests" << std::endl;

  if (!runTreeTests<TestWorkClassCustom2<ChaseLevOwnQueue_v2<TestContext>>,
                    TestWorkClassCustom2<ownQueueCustom_v2<TestContext>>,
                    TestWorkClassDefault<seperateTaskQeues<
                        Task<TestContext> *, nullptr,
                        CyclicArrayLock<Task<TestContext> *, nullptr>>>,
                    TestWorkClassDefault<seperateTaskQeues<
                        Task<TestContext> *, nullptr,
                        CyclicArrayAtomic<Task<TestContext> *, nullptr>>>>()) {
    return 1;
  }

  std::cout << "All tests passed\n";
  return 0;
}
