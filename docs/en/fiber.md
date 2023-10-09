[TOC]

# 1 Overview
This article delves into the principles and implementation of Fiber from various perspectives, aiming to provide a better understanding of Fiber. For information on how to use Fiber, please refer to the following resource[Fiber User Guide](./fiber_user_guide.md)。

# 2 Principle
Fiber is essentially an M:N coroutine, which means it schedules and runs multiple user-level threads on multiple native threads of the system, similar to goroutines in the Go programming language. The diagram below illustrates the principle of M:N coroutines:
![img](/docs/images/m_n_coroutine_en.png)

Compared to native threads, M:N coroutines have the following advantages:

1. Facilitates synchronous programming and provides better usability;

2. Lower cost and higher efficiency compared to native threads: Lightweight and fast operations such as creation and context switching;

3. High parallelism, making full use of resources: Can run concurrently on multiple threads;

After researching and analyzing various M:N coroutines, tRPC Cpp has designed and implemented its own M:N coroutine called Fiber. Fiber has the following characteristics：
1. Multiple threads compete for a shared Fiber queue, which, with careful design, can achieve low scheduling latency. Currently, the size of the scheduling group (number of threads within a group) is limited to between 8 and 15;

2. To reduce contention and improve multi-core scalability, a mechanism for multiple scheduling groups has been designed, allowing configuration of the number and size of the groups;

3. To mitigate extreme cases of imbalance, Fiber supports fiber stealing between scheduling groups: intra-NUMA node group stealing and inter-NUMA node group stealing. It also allows setting stealing coefficients to control the stealing tendency strategy;

4. Comprehensive synchronization primitives;

5. Good interoperability with Pthread;

6. Supports Future-style API;


# 3 Design

## 1 Running mechanism
![img](/docs/images/fiber_threadmodel_arch.png)

In order to adapt to NUMA architecture, Fiber supports multiple scheduling groups (SchedulingGroup). The leftmost group in the diagram shows the complete running and calling relationships. The other groups are simplified to demonstrate Fiber stealing (Work-Stealing) between groups.

The diagram shows components with fixed quantities: Main Thread (globally unique), master fiber (unique within FiberWorker), Queue (unique within SchedulingGroup), TimerWorker (unique within SchedulingGroup). The quantities of other components are only illustrative (configurable or dynamically changeable).


### 1 SchedulingGroup

The M:N thread model, compared to the N:1 model and common coroutine designs, introduces more cache misses, resulting in performance degradation. This is an unavoidable cost to ensure the stability of performance metrics such as response time. However, this does not mean that the performance is inferior to other N:1 frameworks or coroutine frameworks. The overall performance of a system can be optimized in various ways, and the scheduling model alone does not determine the overall performance of a framework. Here, we focus on the scheduling model, while other performance optimization aspects of the framework are not discussed in detail for now
#### Why?

In order to improve scalability in a multi-core, multi-threaded environment, the concept of group management is introduced, where fibers allocated to the same group are referred to as a scheduling group. By dividing resources based on scheduling groups, internal shared data can be partitioned, reducing competition and improving scalability：

1. The scheduling logic typically avoids fiber stealing between scheduling groups, so the fiber workload is usually concentrated within the same scheduling group. This means that the shared data that may need to be accessed by multiple fibers is typically located within this scheduling group. Therefore, the only significant interference between different scheduling groups is the thread synchronization (mutex, etc.) overhead caused by stolen fibers accessing the original scheduling group. In other cases, interference between different scheduling groups is minimal, significantly improving the overall scalability of the framework;

2. Additionally, in NUMA environments, even if task stealing occurs, stealing between scheduling groups across NUMA nodes is more restricted compared to stealing between different scheduling groups within the same NUMA node. This helps to concentrate the workloads of each fiber within a specific NUMA node, avoiding access to remote memory and reducing memory access latency;

3. Lastly, in multi-socket environments, dividing the workload based on scheduling groups also helps to improve the number of communication messages across CPU nodes, thereby enhancing scalabilit;

#### How design

1. Task stealing: Allows fibers to be migrated between scheduling groups through task stealing, but the frequency of task stealing is limited. Task stealing between scheduling groups belonging to different NUMA nodes is disabled by default, and if manually enabled, it is further restricted compared to stealing between scheduling groups within the same NUMA node;

2. The number of FiberWorkers within a scheduling group depends on the parameter scheduling_group_size and cannot exceed 64;

3. Each scheduling group can have a maximum of fiber_run_queue_size executable fibers, and this parameter must be a power of 2;

Therefore, a scheduling group includes:

**1. Queue: Used to store all the ready-to-run fibers;**

To better meet the performance requirements in different scenarios, we currently have two scheduling strategies, and one difference between these strategies lies in the design of the queue:

**a. Scheduling Strategy v1 (default strategy): There is only one global queue within the scheduling group. This global queue approach is more general-purpose;**

**b. Scheduling Strategy v2: Utilizes the industry-standard global queue along with a local queue with a single producer and multiple consumers. This strategy is more suitable for CPU-intensive scenarios as it allows for prioritized execution within the local queue;**

Regardless of whether it is v1 or v2, the design of the queue itself is a bounded lock-free queue. The lock-free design aims to avoid frequent contention and performance degradation. As for the bounded nature of the queue, we conducted an analysis and comparison between bounded and unbounded queues, ultimately choosing a bounded queue for the following reasons:

| Option          |  Advantages | Disadvantages |
| :----------| :-------------------------------------------------------------------------------|:--------------------------------------------------|
|Bounded Queue|1. Lock-free implementation eliminates the need to consider memory deallocation, making it easier to implement. <br> 2. Multiple implementation options such as arrays, linked lists, etc., provide more optimization possibilities in terms of performance. <br> 3. Stable performance without occasional delays caused by memory allocation.|Requires introducing configuration options, which may require manual tuning.|
|Unbounded Queue|Simple，no need to introduce configuration options.|1. Many factors limit the number of fibers that can coexist. <br> 2. If there is truly a massive number of fibers that need to be executed in the system, it usually indicates other bottlenecks in the system|

Therefore, in most cases, a sufficiently large preset value can satisfy the majority of environments. When the preset value is insufficient, the program itself often cannot run properly, and the benefits brought by automatic expansion of an unbounded queue are rarely seen in practical scenarios. Hence, a bounded queue was chosen as our run queue.

**2. A group of threads called FiberWorkers: they retrieve fibers from the Queue and execute them;**

**3. A thread called TimerWorker: responsible for generating fibers for timed tasks on demand;**

Each FiberWorker maintains a thread-local array of timers. Each timer corresponds to a uint64 ID, which represents the address of an Entry object. The TimerWorker maintains a priority queue of timers and iteratively reads the arrays of each FiberWorker, adding them to the priority queue. It then executes the timers in order based on the current time and sleeps (blocks on a condition variable) until the next timer expires.

Typically, the expiration callback of a timer will start a fiber to execute the actual task (although it is not mandatory). The reason for using a separate system thread to implement the timer is twofold: it maintains the purity of FiberWorkers and avoids blocking operations occupying a FiberWorker thread for an extended period

### 2 FiberEntity

A fiber is represented by FiberEntity, and here are some of its important members displayed：
```cpp
  DoublyLinkedListEntry chain;
  std::uint64_t debugging_fiber_id;
  volatile std::uint64_t ever_started_magic;
  Spinlock scheduler_lock;
  FiberState state = FiberState::Ready;
  std::uint64_t last_ready_tsc;
  SchedulingGroup* scheduling_group = nullptr;
  std::size_t stack_size = 0;
  void* state_save_area = nullptr;
  bool is_from_system = false;
  bool scheduling_group_local = false;
  Function<void()> resume_proc = nullptr;
  object_pool::LwSharedPtr<ExitBarrier> exit_barrier;
  Function<void()> start_proc = nullptr;
```

The physical memory layout of Fiber
```
+--------------------------+  <- Stack bottom
| fiber control block      |
+--------------------------+  <- 512 byte
| ...                      |
| ...                      |  <- (Used stack space)
| ...                      |
+--------------------------+  <- Stack top.
| ...                      |
| ...                      |  <- (Unused stack space)
| ...                      |
+--------------------------+  <- Stack limit
| guard page (opt)         |  <- (User fiber only)
+--------------------------+  <- Stack limit + PAGE_SIZE
```
Note: Each stack may also have an inaccessible guard page to detect stack overflow. By default, the guard page is enabled, which means that each stack has two memory segments (VMA). If not enabled, usually only one VMA is needed (but there is a risk of stack overflow detection failure). The configuration option 'fiber_stack_enable_guard_page' is used to indicate whether the guard page is enabled。


## 2 Task scheduling

### 1 Scheduling

The process of a fiber being created and executed by a FiberWorker is a typical producer-consumer scenario. Typically, we have the following methods for scheduling:

1. Idle worker thread polling the queue: This helps improve wake-up latency, as the producer does not need to initiate a system call to wake up the worker thread. However, continuously polling the queue leads to 100% CPU load, which is generally unacceptable in production environments.

2. Shared queue with condition variables: This approach involves intense competition for the shared data structure. Inserting into the shared queue without locking may result in wakeup loss, while locking significantly impacts overall throughput. It is not possible to avoid the system call for waking up worker threads.

3. One queue per worker thread: This approach can lead to load imbalance and requires a system call to wake up worker threads.

4. One queue per worker thread with task stealing: The effectiveness of this approach depends on the intensity of task stealing. If the stealing intensity is low, it may not resolve load imbalance issues. If the stealing intensity is high, it can result in excessive ineffective stealing during low loads, leading to lock contention in various queues and requiring system calls to wake up worker threads

Due to the significant drawbacks of the aforementioned algorithms, we have designed our own wake-up algorithm:

**a. Scheduling Strategy v1 (default strategy): Fiber adopts a balanced strategy by employing up to 2 threads for parallel polling**

1. When fetching a Fiber, if one is available, it is executed directly. If no Fiber is available and there are fewer than 2 polling threads, it enters the polling state;

2. If a polling thread successfully fetches a Fiber, it wakes up another thread before executing the Fiber. If polling times out, it goes to sleep;

3. When producing a Fiber, if there are polling threads available, it instructs one of them to exit polling and fetch the Fiber. Otherwise, it wakes up a thread;

In the selection of polling threads/sleeping threads, they are ordered based on a fixed numbering scheme, and the first (LSB) option is chosen. This is because multiple fibers are likely cooperative tasks, and running them on specific threads (FiberWorkers can be configured to bind to specific cores, and the operating system tends to run the same thread on a fixed CPU) improves CPU cache efficiency.

**b. Scheduling Strategy v2: Prioritizing local queues while also supporting task stealing between global and local queues (inspired by the implementation ideas of the taskflow runtime)**
1. The task queue consists of a global (MPMC) and local (SPMC) queue, and supporting task stealing between global and local queues;

2. When executing parallel tasks, the worker thread can add tasks to its local queue without notification, avoiding system call overhead;

3. Taskflow's native notifier is used for inter-thread communication, reducing notification overhead;

4. In Taskflow's task scheduling, tasks cannot block the thread during execution. Once this happens, one of the threads will consume 100% of the CPU. However, when the fiber reactor executes epoll_wait and there are no network events, it may block the thread. To solve this problem, we treat the reactor fiber as an special type fiber and put it into the separate queue to avoid frequent detection of reactor tasks in the running queue, which would cause the worker thread to be unable to sleep and result in 100% CPU usage.

Relevant information can be found at [taskflow executor](https://github.com/taskflow/taskflow/blob/master/taskflow/core/executor.hpp)
### 2 Steal
If the Fiber creation attribute specifies that stealing is not allowed, the Fiber will only execute within its initial scheduling group, otherwise it may be stolen by other groups. Depending on performance considerations, stealing can be categorized into two types:

1. Within the NUMA Node: Performance is the same as normal scheduling. This is primarily used to balance the load between groups and is enabled by default;

2. Across NUMA Nodes: Performance is poorer. This is not enabled by default;

Whether within or across groups, the principle remains the same, with the only difference being the target Queue that the working thread acquires. The algorithm is also the same, with the only variation being the stealing frequency coefficient (steal_every_n, where 0 disables stealing, and a larger value indicates a lower stealing frequency), which can be specified as a parameter.


## 3 User Interface
Fiber's user-level threading has similarities to Pthreads in terms of thread positioning, so there are some similarities in the capabilities provided：

| Description | Pthread  |   Fiber   |    Note   |  
| :-----------------------------| :-------------------------|:----------------------------|:----------------------------------------------------------|
|Create && run|std::thread|Fiber && StartFiberDetached|Custom runtime attributes need to be provided to meet different scenarios|
|Synchronization with Latch|std::latch|FiberLatch|Used for multi-task cooperative synchronization, such as creating N parallel Fiber tasks and waiting for all results|
|Synchronization with mutex|std::mutex|FiberMutex|Mutual exclusion access|
|Synchronization with shared mutex|std::shared_mutex|FiberSharedMutex|Read-priority, suitable for scenarios with more reads than writes|
|Synchronization with seq mutex|Seqlock|FiberSeqLock|Write-priority|
|Condition Variable|std::condition_variable|FiberConditionVariable|Notify if the condition is met|

After that, there are also:
1. The FiberLatch class for interacting with external threads;

2. Tools like BlockingGet for asynchronous programming with Futures;