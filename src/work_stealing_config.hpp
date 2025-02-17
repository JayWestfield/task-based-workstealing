#include "oldVersions/v1_sharedPointerBased/v1_taskManager/ChaseLevOwnQueue_v1.hpp"
#include "oldVersions/v1_sharedPointerBased/work_stealing_scheduler_v1.hpp"
#include "oldVersions/v2_customAllocation/v2_taskManager/ChaseLevOwnQueue_v2.hpp"
#include "oldVersions/v2_customAllocation/work_stealing_scheduler_v2.hpp"
#include "oldVersions/v3_finalVersion/v3_taskManager/ChaseLevOwnQueue_v3.hpp"
#include "oldVersions/v3_finalVersion/work_stealing_scheduler_v3_1.hpp"
#include "releaseVersion/work_stealing_scheduler.hpp"
#include "structs/seperate_task_queues.hpp"
#include "structs/task_queues/cyclic_array_lock.hpp"
#include <cstddef>
#include <cstdint>

template <typename TaskContext,
          typename TaskManager = ChaseLevOwnQueue_v1<TaskContext>>
using wss1 = WorkStealingScheduler_v1<TaskContext, TaskManager>;
template <typename TaskContext,
          typename TaskManager = ChaseLevOwnQueue_v2<TaskContext>>
using wss2 = WorkStealingScheduler_v2<TaskContext, TaskManager>;
template <typename TaskContext,
          typename TaskManager = ChaseLevOwnQueue_v3<TaskContext>>
using wss3 = WorkStealingScheduler_v3_1<TaskContext, TaskManager>;

// default (final one)
template <typename TaskContext, size_t segmentSize,
          size_t reusable_tasks_batch_size, size_t free_tasks_batch_size,
          typename TaskManager = ChaseLevDeque_v3<TaskContext>>
using wss31 = WorkStealingScheduler_v3_1<TaskContext, TaskManager, segmentSize,
                                         reusable_tasks_batch_size,
                                         free_tasks_batch_size>;
template <typename TaskContext,
          typename TaskManager =
              seperateTaskQeues<Task<TaskContext> *, nullptr,
                                CyclicArrayLock<Task<TaskContext> *, nullptr>>,
          size_t segmentSize = 48>
using wss = WorkStealingScheduler<TaskContext, TaskManager, segmentSize>;