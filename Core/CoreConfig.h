#pragma once

/// The capacity of a `Core` `Log`'s message pool.
#define ARES_CORE_LOG_MESSAGE_POOL_CAPACITY 1024

/// The capacity of a `Core` `TaskScheduler`'s fiber pool. 
#define ARES_CORE_SCHEDULER_FIBER_POOL_CAPACITY 256

/// The stack size in bytes of each fiber in a `Core` `TaskScheduler`'s fiber pool.
#define ARES_CORE_SCHEDULER_FIBER_STACK_SIZE (128 * 1024)

/// The maximum number of entities in a `Core`'s `Scene`.
#define ARES_CORE_SCENE_ENTITY_CAPACITY 1024
