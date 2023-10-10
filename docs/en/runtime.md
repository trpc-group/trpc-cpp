[中文版](../zh/runtime.md)

# Overview

In order to meet the needs of diverse business scenarios: for services with heavy network IO, such as access layer services, they require high QPS (Queries Per Second) performance; for services with heavy CPU computation, such as recommendation and search services, they typically adopt synchronous programming and focus on reducing tail latency.

Therefore, the tRPC-Cpp framework supports multiple runtime types in a plugin-based manner, using different thread models to meet the needs of different business scenarios. Currently, the framework supports the following three runtime:

1. Fiber M:N coroutine model
2. IO/Handle separated thread model
3. IO/Handle merged thread model

When using the framework, you need to choose the appropriate runtime based on your business scenario to achieve better performance and latency.

Next, we will explain how to choose and configure the runtime, and introduce the specific implementations of various runtimes.

# How to choose and configure the runtime

## How to choose the runtime

| Type | Programming Model | Pros and Cons | Suitable Scenarios |
| -----| ------------------| --------------| -------- |
| Fiber M:N coroutine model | Coroutine synchronous programming | Pros: It adopts coroutine synchronous programming, making it easier to write complex business logic; network IO and business processing logic can be parallelized across multiple cores, making fully use of multiple cores to achieve low long-tail latency. <br> Cons: To avoid blocking threads, it may be necessary to use specific coroutine synchronization primitives for synchronization, which can result in strong code invasiveness. Additionally, the number of coroutines is limited by the system, and coroutine scheduling incurs additional overhead, leading to suboptimal performance in scenarios with high QPS or a large number of connections. | CPU-bound business scenarios with high requirements for reducing long-tail latency, such as recommendation/search/advertising services. |
| IO/Handle merged thread model | future/promise asynchronous programming | Pros: Request processing does not cross threads, resulting in low average latency. <br> Cons: The business processing logic cannot be parallelized across multiple cores. When there are few connections or uneven computational workload, it can lead to uneven thread load. It is not suitable for scenarios with heavy business logic or blocking operations. | IO-bound business scenarios that require low latency and high QPS, such as access layer gateway services. | 
| IO/Handle separated thread model | future/promise asynchronous programming | Pros: Relatively versatile, allowing for few blocking operations in the business logic. <br> Cons: Handling requests across two threads introduces additional scheduling delays and cache misses, which can impact the overall average latency of the requests. By default, the request processing logic only executes in the current handle thread, which is not very convenient for programming in scenarios that require parallel computing on multiple CPU cores (additional logic is needed to enqueue tasks to the global queue). | Business scenarios that do not have extreme requirements for QPS and latency. |

When using it, you should just choose one runtime based on business scenario and avoid mixing multiple models.

## Configure the runtime

After selecting the runtime, you need to configure the corresponding settings in the framework configuration file:

1. Fiber M:N coroutine model

   ```yaml
   global:
     threadmodel:
       fiber:
         - instance_name: fiber_instance  # thread model instance name, required
           concurrency_hint: 8            # number of fiber worker threads, if not configured, it will adapt based on machine information automatically
   ```

2. IO/Handle merged thread model

   ```yaml
   global:
     threadmodel:
       default:
         - instance_name: merge_instance  # thread model instance name, required
           io_handle_type: merge          # running mode, separate or merge, required
           io_thread_num: 8               # number of io thread, required
   ```

3. IO/Handle separated thread model

   ```yaml
   global:
     threadmodel:
       default:
         - instance_name: separate_instance  # thread model instance name, required
           io_handle_type: separate          # running mode, separate or merge, required
           io_thread_num: 2                  # number of io thread, required
           handle_thread_num: 6              # number of handle thread, required in separate model, no need in merge model
   ```

For more configuration details, please refer to the [framework configuration](framework_config_full.md).

# The specific implementations of various runtimes

## Fiber M:N coroutine model

![img](/docs/images/fiber_threadmodel_arch.png)

Similar to Golang's goroutine, by encapsulating network I/O tasks and business processing logic into coroutine tasks, then scheduled and executed tasks in parallel across multiple cores by a user-level coroutine scheduler.

The implementation adopts a design with multiple scheduling groups and supports NUMA architecture. This allows to run different scheduling groups under NUMA architectures, reducing inter-node communication.

All worker threads in the same scheduling group share a global queue and support task stealing between groups,enabling parallel processing of tasks across multiple CPU cores. Additionally, in the v2 scheduler, each worker thread has its own local queue (single producer, multiple consumers), allowing tasks to be produced and consumed within the same thread as much as possible, further reducing cache misses.

## IO/Handle merged thread model

![img](/docs/images/merge_threadmodel_arch.png)

Drawing inspiration from the shared-nothing concept, each worker thread has its own task queue (multiple producers, single consumer) and reactor, which fixes the network IO and business logic on a certain connection to the same thread for processing. In this way, requests on the connection are processed in a fixed thread, with no thread switching overhead and minimal cache misses. Therefore, a very low average delay can be achieved, thereby achieving a very high QPS.

However, because requests on each thread need to be queued for processing and cannot be parallelized by multiple cores, if the processing logic of a certain request is relatively heavy, it will easily affect the processing of subsequent requests, so it is not suitable for heavy CPU computing scenarios.

## IO/Handle separated thread model

![img](/docs/images/separate_threadmodel_arch.png)

IO threads are responsible for handling network I/O logic, while Handle threads handle business logic. When using it, users need to adjust the ratio of IO and Handle threads based on their application scenarios.

In implementation, each IO thread has its own task queue, and Handle threads share a global queue,
allowing parallel execution of tasks in the global queue. Therefore, a small amount of blocking business logic does not affect the processing of requests, making it highly versatile. Additionally, each Handle thread has its own local queue (multiple producers, single consumer), enabling request processing to be executed on specific threads.

However, due to request processing spanning two threads, the average latency and QPS in network I/O-bound scenarios are not as good as the merged thread model. Network I/O events on a specific reactor cannot be parallelized across multiple CPU cores, and the request processing logic by default only executes in the current thread, resulting in higher long-tail latency compared to the fiber m:n coroutine model in CPU-intensive scenarios.
