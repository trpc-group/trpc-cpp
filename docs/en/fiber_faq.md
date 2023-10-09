[TOC]
# 1 Overview

In actual usage, it is not that we are unfamiliar with or unable to use the features of Fiber, but rather that there are more possibilities of encountering issues. Broadly speaking, there are two main categories of problems: performance-related and troubleshooting related to coredump.

# 2 Performance FAQ

## 1 Performance issues?
You can follow the following steps to troubleshoot in the context of Fiber
1. If there is a significant performance difference compared to the original version, ensure that compiler optimizations, such as O2 level, are enabled; Verify if the libraries being compared are exactly the same. For example, check if the original version was compiled with cmake and had dependencies on different versions of third-party libraries compared to Fiber, which was compiled with Bazel.

2. If the machine load is low and remains low, check if there are any significant critical sections. If so, consider breaking down the locks into smaller granularities; Check for any synchronous blocking logic. If found, consider transforming it into asynchronous operations, such as dispatching it to a thread pool for processing.

3. If the machine load can reach high levels and performance degradation occurs under heavy QPS pressure, try adjusting Fiber configurations accordingly. For example, configure multiple FiberWorkers (recommended to match the available CPU cores), set up multiple scheduling groups for isolation, and disable NUMA stealing if applicable.

4. If ServiceProxy is used to access downstream services, prioritize connection reuse (if supported), followed by Pipeline mode (if supported, such as with native Redis Server), and then connection pooling mode (for protocols like HTTP, where it is advisable to configure a larger value for max_conn_number). Avoid using short-lived connection mode whenever possible.

## 2 When load testing a backend Fiber-developed server, if the overall QPS remains the same but there is a noticeable performance degradation when there are more upstream connections?
This is expected. The good P99 latency performance in handling RPCs in Fiber mode is achieved through high parallelism across different connections and within the same connection (for example, different readable events within the same connection are processed in parallel). This implementation itself requires the creation of Fibers to handle these tasks. Therefore, when the QPS is the same, the more upstream connections there are, the more connections are distributed to the backend Fiber Servers, resulting in more Fibers that need to be created. The more Fibers created, the more resources are consumed during their creation and execution, leading to increased performance overhead. Therefore, when there are a large number of connections, the performance of Fiber mode may significantly decrease.

## 3 When there is a significant difference between the time of initiating a call and the actual execution time?
From the initiation of the call to the actual execution, it goes through the stages of construction, enqueuing, and waiting for an available FiberWorker to schedule the Fiber. If there is a significant gap in between, it may indicate that the created Fiber is waiting in the queue for processing. The reason for this queueing could be that the FiberWorker threads are busy, in which case increasing the number of FiberWorker threads may be necessary. Alternatively, it could be due to slow execution of Fiber tasks on the FiberWorker threads.

Therefore, you can try reducing the number of currently created Fibers, optimizing Fiber tasks that consume a significant amount of CPU resources, increasing the number of consumer FiberWorker threads, or setting up multiple scheduling groups to isolate them from each other as possible solutions.


## 4 Can thread-level synchronization primitives, such as locks, be used in Fiber?
It depends on the situation. If the critical section is small or there is minimal competition, using thread-level synchronization primitives is recommended. Using Fiber-level synchronization primitives in such cases may lead to performance degradation due to coroutine rescheduling. Although thread-level synchronization primitives may block the thread running the current Fiber, the impact is relatively small if the granularity is small.

However, if the critical section is large or there is significant competition, it is still recommended to use Fiber-level synchronization primitives.

## 5 How to resolve high memory usage?
If you are using the ServiceProxy client to access downstream services and the client is using Pipeline, and there are a large number of backend nodes (such as accessing Redis Server with hundreds or even thousands of nodes), it may be appropriate to reduce the values of fiber_pipeline_connector_queue_size and max_conn_num.

If you are using Fiber synchronization primitives, check if there are any incorrect usage of the synchronization primitives (e.g., FiberLatch not being CountDown), which may prevent the corresponding Fiber from being released. It is recommended to add exception handling to ensure that the Fiber can be executed to completion.

## 6 How to obtain the current accumulated number of Fiber tasks and how to get the total number of all current Fiber tasks?
To obtain the current accumulated number of Fiber tasks: Use the trpc::fiber::GetFiberQueueSize() interface. This value is also reported as the trpc.FiberTaskQueueSize metric. If this metric remains consistently high for a long time, it indicates a potential accumulation of Fiber tasks in the system.

To get the total number of all current Fiber tasks: Use the trpc::GetFiberCount() interface. This represents the total number of valid Fiber tasks within the current lifecycle. At the implementation level, this count is incremented by one each time a Fiber is created and decremented by one when resources are released after each Fiber completes execution.

## 7 Is it expected to have uneven load on FiberWorkers when using the default Fiber scheduling strategy (v1)?
If the current system load is low, such as having a small number of Fibers, it is possible to have uneven load distribution, specifically with two FiberWorker threads within the same scheduling group having a higher load. This behavior is expected due to the default Fiber scheduling strategy, which prioritizes these two FiberWorker threads. Therefore, in a low-load scenario, these two threads may have a higher load compared to others within the same scheduling group.

In such cases, you can try increasing the QPS (queries per second) and increasing the system load. This should eliminate the uneven load distribution phenomenon.

## 8 Is a small value of vm.max_map_count causing Fiber creation failures?

For each allocated Fiber stack, two memory segments are introduced (represented as a single line in /proc/self/maps). These segments serve as the Fiber stack and the guard page used for stack overflow detection.

Linux systems limit the maximum number of memory segments allowed per process, with a default value of 65536. For services with high QPS and long individual request processing times (with high concurrency), this limit (32768) can be reached. Considering the memory segments occupied by stack objects in thread-local caches caused by object pools, as well as other code and file mappings, the upper limit is approximately 30,000 Fibers.

This parameter can be resolved by modifying vm.max_map_count. The specific value can be adjusted according to business needs.

Here is the modification method：

```
echo 1048576 > /proc/sys/vm/max_map_count
```

The side effects of this option can be referred to in 'Side effects when increasing vm.max_map_count,' but generally, it does not have any impact on the service or the machine.

## 9 Run queue overflow. Too many ready fibers to run？
If there are too many Fibers, there may be occurrences of Fiber queue overflow, which can be indicated by similar log messages:

```
Run queue overflow. Too many ready fibers to run. If you're still not overloaded, consider increasing run_queue_size
```

You can consider increasing the 'run_queue_size' configuration parameter in the Fiber settings to address this issue.

# 3 Performance FAQ

## 8 How to troubleshoot a Coredump?
1. When the application crashes at startup, check if a valid concurrency_hint is configured. Solution: If the concurrency_hint is not manually configured, the framework will read the CPU information from the system (e.g., /proc/cpuinfo). In containerized environments, there may be inaccuracies in reading the system's CPU information and the actual available number of cores. Fiber environment initialization requires resource allocation, and if the concurrency_hint is set too high, it may result in insufficient resources and cause a crash. It is recommended to manually adjust the concurrency_hint to be 1-2 times the actual available number of cores (based on experience).

2. Check if there are too many Fiber creations. Solution: Observe the trpc.FiberTaskQueueSize metric to verify if there is a long-term accumulation. If there is, consider reducing the QPS, increasing the number of FiberWorkers, or using Fiber rate limiting to ensure a degraded service.

3. Check for potential thread-level race conditions. Solution: Protect critical sections with locks or avoid competition at the source.

4. Check if there are large stack variables, such as exceeding the default Fiber stack size of 128K. Solution: Increase the default stack size by adjusting the fiber_stack_size configuration option. Alternatively, try to avoid defining large stack memory variables and use heap memory instead.

5. Check for issues with variable lifecycles during capture and execution. Solution: Since Fiber execution is asynchronous while capture is real-time, there may be scenarios where the variable's lifecycle has already expired during execution, leading to crashes. It is recommended to use smart pointers for capture by reference, std::move for values, or directly capture and copy values.

6. Check for uncaught exceptions. tRPC-Cpp programming does not use exceptions, so it is necessary to handle exceptions manually. If an uncaught exception occurs within a Fiber, it can lead to a crash. 
# 4 Others FAQ

## 1 Can asynchronous RPC interfaces be used to access downstream services within a Fiber?
Yes, it is supported.

## 2 Is it possible to configure multiple instances of Fiber threads?
Currently, only one instance of Fiber thread can be configured. Multiple instances are not supported.

## 3 If you need to write unit tests for Fibers?
You can consider using the RunAsFiber utility tool:

```cpp
TEST(Latch, WaitFor) {
  RunAsFiber([] {
    FiberLatch l(1);
    ASSERT_FALSE(l.WaitFor(std::chrono::milliseconds(100)));
    l.CountDown();
    ASSERT_TRUE(l.WaitFor(std::chrono::milliseconds(0)));
  });
}
```

## 4 Can Fiber features, such as Fiber synchronization primitives, be run on non-FiberWorker threads?
Currently, it is not supported. If used, it will result in a failure to check 'IsFiberContextPresent' and cause a Coredump.

Support for this will be available in the future. Please stay tuned.