[中文](../zh/fiber_user_guide.md)

# Overview

This article provides a developer's perspective on how to use Fiber, including configuration, commonly used classes and interfaces, FAQ, and more. For a deeper understanding of the Fiber principles, you can refer to the following resource[Fiber](./fiber.md)。

# Configuration

Currently, Fiber's functionality needs to run within a Fiber execution environment. The first step in setting up the Fiber environment is to configure it correctly. Here, we will only focus on the configuration specific to Fiber.

To simplify usage, developers only need to fill in a few required configuration items. Below is a simplified Fiber configuration:

```yaml
global:
  threadmodel:
    fiber:                              
      - instance_name: fiber_instance   
        concurrency_hint: 8             
xxx
```

It is recommended to manually fill in the concurrency_hint configuration item to avoid issues caused by reading system configurations.

In addition to the simplified configuration, you can also customize advanced configuration options. Here is the complete Fiber configuration:

```yaml
global:
  threadmodel:
    fiber:
      - instance_name: fiber_instance
        concurrency_hint: 8                                       #Suggested configuration, indicating the total number of physical fiber worker threads to run fiber tasks. If not configured, the default value is to read the number of /proc/cpuinfo (in container scenarios, the actual number of available cores may be much lower than the value in /proc/cpuinfo, which can cause frequent thread switching and impact performance). Therefore, it is recommended to set this value to be the same as the actual number of available cores.
        scheduling_group_size: 4                                  #Suggested configuration, indicating the number of fiber worker threads per scheduling group (to reduce contention, the framework introduces multiple scheduling groups to manage physical fiber worker threads). If not configured, the framework will automatically create one or more scheduling groups based on the concurrency_hint value and strategy. If you want to have only one scheduling group, you can set this value the same as concurrency_hint. If you want to have multiple scheduling groups, you can refer to the example configuration: indicating each scheduling group has 4 fiber worker threads, with a total of 2 scheduling groups.
        reactor_num_per_scheduling_group: 1                       #It indicates the number of reactor models per scheduling group. If not configured, the default value is 1. For scenarios with heavy I/O, you can increase this parameter appropriately, but avoid setting it too high.
        reactor_task_queue_size: 65536                            #reactor_task_queue_size
        fiber_stack_size: 131072                                  #fiber_stack_size default 128K
        fiber_run_queue_size: 131072                              #fiber_run_queue_size
        fiber_pool_num_by_mmap: 30720                             #fiber_pool_num_by_mmap
        numa_aware: false                                         #numa_aware
        fiber_worker_accessible_cpus: 0-4,6,7                     #It indicates the need to specify running on specific CPU IDs. If not configured, the default value is empty. If you want to specify, refer to the current example configuration: specifying CPU IDs from 0 to 4, as well as 6 and 7.
        fiber_worker_disallow_cpu_migration: false                #It indicates whether to allow fiber workers to run on different CPUs, i.e., whether to bind cores. If not configured, the default value is false, which means cores are not bound by default.
        work_stealing_ratio: 16                                   #It represents the proportion of task stealing between different scheduling groups. If not configured, the default value is 16, indicating task stealing is performed in a 16% proportion.
        cross_numa_work_stealing_ratio: 0                         #It represents the frequency of task stealing between different nodes in a NUMA architecture.
        fiber_stack_enable_guard_page: true                       #fiber_stack_enable_guard_page
        fiber_scheduling_name: v1                                 #fiber_scheduling_name
```


# Class && Interface

## Start and Run Fiber

Interface：

```cpp
// @brief Create a fiber and run it
// @note  If the program creates too many fibers, system has not memory, it will return failure.
bool StartFiberDetached(Function<void()>&& start_proc);

// @brief Create a fiber by attrs and run it
bool StartFiberDetached(Fiber::Attributes&& attrs, Function<void()>&& start_proc);
```

By default, Fiber is created with runtime attributes initiated and executed by the framework, which may be scheduled in other dispatch groups. However, it is also possible to customize the Fiber runtime attributes using Fiber::Attributes:

```cpp
  struct Attributes {
    // How to start a Fiber: By default, Post will enqueue the Fiber for scheduling execution. 
    // Alternatively, you can choose the Dispatch method for immediate execution.
    fiber::Launch launch_policy = fiber::Launch::Post;

    // Which scheduling group should the Fiber execute in? By default, it selects the nearest group 
    // (i.e., Fibers from the same calling thread execute in the same group).
    // You can choose kUnspecifiedSchedulingGroup for random selection or specify a fixed group (0 to the number of groups - 1).
    std::size_t scheduling_group = kNearestSchedulingGroup;

    // Is it allowed for the Fiber to execute in other scheduling groups? By default, it is allowed.
    bool scheduling_group_local = false;
  };
```

To create a Fiber with default attributes：

```cpp
trpc::StartFiberDetached([&] {
  Handle();
});
```

You can also specify Fiber::Attributes to run the Fiber in a specific scheduling group:

```cpp
trpc::Fiber::Attributes attr;
attr.scheduling_group = 1;

trpc::StartFiberDetached(attr, [&] {
  Handle();
});
```

## Collaboration between multiple Fibers

FiberLatch used for collaboration between multiple Fibers, as shown in the example:

```cpp
trpc::FiberLatch l(1);
trpc::StartFiberDetached([&] {
  Handle();
      
  // As the execution unit is about to finish, the FiberLatch is decremented. When it reaches zero, it will awaken the synchronous waiting operation.
  l.CountDown();
});    
    
// The synchronous waiting operation is awakened only when the value of the FiberLatch is reduced to 0.
l.Wait();
```

FiberLatch Interface:

|Interface |  Function |Parameter |Return value|
| :-------------------------------------------------------| :---------------------|:---------------------|:----------------------|
| FiberLatch(std::ptrdiff_t count) |  Creates a FiberLatch with an internal count of count|count: internal count | void |
| CountDown(std::ptrdiff_t update = 1) |  Decrements the internal count by update|update: value to subtract from the internal count | void |
| TryWait() |  Checks if the internal count is already 0| Noop | bool: returns true if the internal count is 0, otherwise false |
| Wait() |  Blocks and waits until the internal count is 0| Noop  | void |
| ArriveAndWait(std::ptrdiff_t update = 1) |  Combined interface of CountDown() and Wait()| | void |
| bool WaitFor(const std::chrono::duration<Rep, Period>& timeout)  | Waits for the internal count to be 0 within the specified timeout duration|timeout: timeout duration | void |
| bool WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout)  | Waits for the internal count to be 0 within the specified timeout duration|timeout: timeout time point | void |

## Mutual exclusive access between Fibers

FiberMutex used for mutual exclusive access to shared data between Fibers, as shown in the example:

```cpp
trpc::FiberLatch latch(10);
trpc::FiberMutex mu;
for(size_t i=0; i<10; ++i) {
  trpc::StartFiberDetached([&latch, &mu] {
    for(size_t i=0; i<1000; ++i) {
      // FiberMutex exclusive access
      std::scoped_lock _(mu)

      Update();
    }

    latch.count_down();
  });
}

latch.wait();
```

FiberMutex can be used in conjunction with RAII tools like std::scoped_lock to avoid forgetting to release lock resources.

FiberLatch Interface:

|Interface |  Function |Parameter |Return value|
| :-------------------------------------------------------| :---------------------|:---------------------|:----------------------|
| void wait(`std::unique_lock<FiberMutex>`& lock)  | Wait until notify_xxx()  | lock FiberMutex| void |
| void lock()  | Protect shared data with locks | Noop | void |
| void unlock()  | Release the lock and wake up the corresponding Fiber | Noop | void |
| bool try_lock() | Can the lock be acquired? If it has already been locked, return failure. | Noop | bool, return false if it has been locked, otherwise return true |

## Shared data when read priority

Read-write lock used for shared data between Fibers: FiberSharedMutex. Example:

```cpp
trpc::FiberSharedMutex rwlock_;

// use write_lock
void Update(){
  rwlock_.lock();
  // ...Update

  rwlock_.unlock();
}
 
// use read_lock
void Get(){
  rwlock_.lock_shared();
  // ...Get

  rwlock_.unlock_shared();
}
```

## Shared data when write priority

Read-write lock used for shared data between Fibers: FiberSeqLoc. Example

```cpp
trpc::FiberSeqLoc seqlock;

std::string Get(const std::string& key){
  std::string ret;
  do {
    seq = seqlock.ReadSeqBegin();
    // read
    ret = map[key];

  } while (seqlock.ReadSeqRetry(seq));

  return ret;
}

void Update(const std::string& key, const std::string& value){
  seqlock.WriteSeqLock();

  // Write
  map[key]= value;

  seqlock.WriteSeqUnlock();
}
```

## ConditionVariable

Used for Fiber condition notification: FiberConditionVariable. Example:

```cpp

FiberConditionVariable cv;
FiberMutex lock;
bool set;

void Wait(){
  std::unique_lock lk(lock);
  cv.wait(lk, [&] { return set; });
}

void Notify(){
  std::scoped_lock _(lock);
  set = true;
  cv.notify_one();
}
```

FiberConditionVariable Interface:

|Interface |  Function |Parameter |Return value|
| :-------------------------------------------------------| :-------------------------------|:--------------------------|:--------------------------------|
| void wait(`std::unique_lock<FiberMutex>`& lock)  | wait notify_xxx()  | lock FiberMutex| void |
| std::cv_status wait_for(`std::unique_lock<FiberMutex>`& lock,const std::chrono::duration<Rep, Period>& expires_in)  | wait notify or timeout | lock FiberMutex，expires_in | cv_status ，return std::cv_status::no_timeout if no_timeout ，or return std::cv_status::timeout |
| std::cv_status wait_until(`std::unique_lock<FiberMutex>`& lock,const std::chrono::time_point<Clock, Duration>& expires_at)  | wait notify or timeout  | lock FiberMutex，expires_at | cv_status ，return std::cv_status::no_timeout if no_timeout ，or return std::cv_status::timeout |
| notify_one()  | notify one waiter | | void |
| notify_all()  | notify all waiter | | void |
| void wait(`std::unique_lock<FiberMutex>`& lock, Predicate pred)  | until pre is true to Wait() | lock FiberMutex，pred| void |

## Timer

If there is a need to execute timed tasks within a Fiber, currently there are several options available:

- 1.SetFiberTimer+KillFiberTimer:

  ```cpp
  // create fiber timer，run after 100ms
  auto timer_id = SetFiberTimer(trpc::ReadSteadyClock() + 100ms, [&]() {
    Handle();
  });
   
  // kill timer 
  KillFiberTimer(timer_id);
  ```

- 2.SetFiberTimer+FiberTimerKiller as RAII:

  ```cpp
  // FiberTimerKiller
  FiberTimerKiller killer;
  killer.reset(SetFiberTimer(trpc::ReadSteadyClock() + 100ms, [&]() {
    Handle();
  }));
  
  // When the FiberTimerKiller is destructed, it automatically releases the corresponding timer.
  
  ```

- 3.CreateFiberTimer+EnableFiberTimer:

  ```cpp
  // create fiber timer，run after 100ms
  auto timer_id = CreateFiberTimer(trpc::ReadSteadyClock() + 100ms, [&](auto timer_id) {
    // NOT Run In FiberWorker
    Handle();
  });
   
  // Enable
  EnableFiberTimer(timer_id);
  
  // Callback
  void OnTimeout(std::uint64_t timer_id){
    HandleTimeout();
  
    // Kill
    KillFiberTimer(timer_id);
  }
  ```

Note:

- The SetFiberTimer interface creates a Fiber to run a custom callback.
- The CreateFiberTimer interface runs the custom callback in the TimerThread, so Fiber features cannot be used in the callback.

## Synchronize and collaborate with non-framework threads

Synchronize and collaborate with non-framework threads：FiberEvent

```cpp
auto ev = std::make_unique<FiberEvent>();

// Out thread，not FiberWorker
thread_pool.push([&](){
  Handle();

  // Wake up FiberEvent
  ev->Set();

});

// FiberEvent wait
ev->Wait();
```

## Asynchronous programming

It also supports using trpc::Future for asynchronous programming within a Fiber.

```cpp
trpc::StartFiberDetached([&] {
  auto rpc_future = Async_rpc();
  
  // Block and wait for the current Fiber to complete using BlockingGet
  auto rpc_result = trpc::fiber::BlockingGet(rpc_future);
  
  // As trpc::Future programming
  if(rpc_result.IsReady()){
    ...
  }else{
    ...
  }
});

```

# FAQ

See more [Fiber FAQ](./fiber_faq.md)
