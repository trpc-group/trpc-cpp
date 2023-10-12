[English](../en/fiber.md)

# 前言

本文从原理及设计实现等角度深入介绍Fiber，以便于更好了解Fiber。关于Fiber使用可以阅读[Fiber用户指南](./fiber_user_guide.md)。

# 原理

Fiber 本质是一种`M:N`协程，即多个系统原生线程上调度运行多个用户态线程，类似 Golan g语言中的 goroutine。`M:N`协程的原理图如下：
![img](/docs/images/m_n_coroutine_zh.png)

相比于原生线程而言，`M:N` 协程优势：

1. 便于同步式编程，易用性更好；

2. 相对于原生线程，成本更低效率更高：创建、上下文切换等操作轻量快速；

3. 并行度高，充分利用资源：能同时运行在多线程；

而 tRPC-Cpp 经过调研和分析各类 `M:N` 协程之后，设计并实现了自己的 `M:N` 协程（Fiber），tRPC-Cpp 的 Fiber 有以下特点：

1. 多个线程间竞争共享 Fiber 队列，在精心设计下可以达到较低的调度延迟。目前调度组大小（组内线程数）限定为 8~15 之间；

2. 为了减少竞争，提升多核扩展性，设计了多调度组机制，调度组个数以及大小可以配置；

3. 为了减少极端情况下的不均衡性，调度组间支持Fiber窃取：分为 NUMA Node 内组间窃取，跨 NUMA Node 组间窃取，并支持分别设置窃取系数，以控制窃取倾向策略；

4. 完善的同步原语；

5. 与 Pthread 良好的互操作性；

6. 支持 Future 风格 API；

# 设计

## 运行机制

![img](/docs/images/fiber_threadmodel_arch.png)

为了适应 NUMA 架构，Fiber 支持多个调度组（SchedulingGroup）。图中最左边的组显示了完整的运行调用关系。其他的组有所简化，为了展示组间的 Fiber 窃取（Work-Stealing）。

图中数量确定的组件：Main Thread（全局唯一），master Fiber（FiberWorker内唯一），Queue（SchedulingGroup 内唯一），TimerWorker（SchedulingGroup 内唯一）。其他组件数量都只是示意（可配置或者动态改变）。

### 调度组

`M:N` 的线程模型相对于 N:1 以及常见的协程设计，会引入更多的 cache miss，导致性能下降，这是为了保证性能（响应时间等）的平稳性所难以避免的代价。但是这并不是说性能表现不如其他 N:1的框架或协程框架。系统的整体性能实际上可以由多方面进行优化，单纯的调度模型并不会决定一个框架的整体性能，这里重点介绍调度模型相关，框架其他性能优化部分暂时不做展开。

#### 为什么引入调度组?

为改善在多核多线程环境下的伸缩性，所以引入分组管理的思想，将分到同一组的 FiberWorker 称之为一个调度组。将资源按照调度组划分其内部共享数据，可以减少共享数据的竞争、提高伸缩性:

1. 调度逻辑通常避免在调度组间发生 Fiber 偷取，因此通常Fiber工作集会集中在同一个调度组内。这意味着可能需要竞争的共享数据通常也处在这一调度组。因此不同调度组之间通常只会由于被偷取的Fiber访问原调度组导致的的线程间同步（mutex 等）开销，其余情况下不同调度组之间的干扰比较小，明显的改善框架的整体伸缩性；

2. 其次，在 NUMA 环境中，即便发生任务偷取，跨 NUMA 节点的调度组间偷取也会受到相对于同 NUMA 节点内不同调度组间的偷取更强的频率限制。有利于将各个 Fiber 的工作集集中在某个 NUMA 节点内，避免访问远端内存，降低内存访问延迟；

3. 最后，对于多路环境，按照调度组划分工作集还有利于改善跨 CPU 节点的通信消息数量，提高伸缩性；

#### 调度组的设计

在设计调度组时有以下考虑：

1. 任务窃取：允许各个调度组之间以任务偷取的方式迁移 Fiber 但是任务偷取的频率是受到限制的。隶属于不同 NUMA 节点的调度组之间的任务偷取默认被禁用，如果手动启用，其频率也会受到相对于同 NUMA 节点之间而言，更进一步的限制；

2. 调度组内 FiberWorker 个数取决于参数 scheduling_group_size，且不能超过64；

3. 每个调度组内至多允许有 fiber_run_queue_size 个可执行的 Fibers，这一参数必须是2的整数次幂；

所以，一个调度组包含：

1. **队列，用来存放所有待运行的 Fiber**

    为了更好的满足不同场景下对性能的需要，我们目前有两种调度策略，不同的调度策略的一个差异点就是队列的设计:

    * **调度策略 v1 版本(默认策略):同调度组内只有一个全局的队列。这种全局队列方式更通用一些**

    * **调度策略 v2 版本:使用业界主流的全局队列+单生产者多消费者的本地队列。因为可以优先在当前本地队列执行，所以更适合CPU密集型场景**

    这里无论是 v1 版本还是 v2 版本，队列本身的设计都是有界无锁队列。无锁是处于避免频繁竞争影响性能，至于有界的权衡，在最开始对有界队列和无界队列做了分析对比，最终选择有界队列，原因如下：

    | 选型          |  优势 | 劣势 |
    | :----------| :-------------------------------------------------------------------------------|:--------------------------------------------------|
    |有界队列|1. 无锁实现不用考虑内存释放问题，易于实现；<br> 2. 实现方式多，数组、链表等，在性能上更多的优化空间；<br> 3. 性能表现平稳，不会出现内存分配导致的偶发延迟。<br> |需要引入配置项，可能需要手动调参|
    |无界队列|无需引入配置项，简单|1.可同时存在的 Fiber 数量制约因素很多；<br> 2.如果系统中真的存在海量的 Fiber 需要执行，通常意味着系统已经出现了其他方面的瓶颈；<br> |

    因此通常一个足够大的预设值可以满足绝大多数环境，当预设值不足时程序本身往往已经不能正常运行了，此时无界队列自动扩容带来的好处很少有实际场景。因此选择了有界队列作为我们的 run queue。

2. **一组线程 FiberWorker：从 Queue 中获取 Fiber 执行**

3. **一个线程 TimerWorker：按需生产定时任务的 Fiber**

    每个 FiberWorker 维护一个 thread_local 的定时器数组。每个定时器对应一个 uint64 的 ID，也即 Entry 对象地址。TimerWorker 则中维护一个定时器的优先级队列，循环读取各个FiberWorker 的数组并加入优先级队列，然后按照当前时刻依次执行，然后睡眠(阻塞在条件变量)到下一个定时器超时。

    通常来说，定时器的到期回调会启动一个  Fiber 来执行真正的任务(不过并非必须)。这里之所以采用一个单独的系统线程来实现定时器，一方面是保持  FiberWorker 的纯粹性，一方面也避免了阻塞操作长期占用一个 FiberWorker 线程。

### Fiber 运行实体

* 一个 Fiber 用 FiberEntity 表示，这里展示其中比较重要的成员

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

* Fiber 的物理空间布局

  ```mermaid
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

  注意：每个栈可能还会有一个不可访问的页 guard page 用于检测栈溢出。默认启用 guard page，所以意味着每个栈有两个内存段（VMA），而不启用通常只需要一个 VMA（但是有栈溢出检测不到的风险）通过配置项 fiber_stack_enable_guard_page 来标识是否启用。

## 任务调度

### 调度

Fiber 从被创建到被 FiberWorker 执行是一个典型的生产消费的场景，通常我们有如下办法可以调度：

1. 空闲的工作线程轮询队列：有助于改善唤醒延迟，生产者不需要发起 syscall 唤醒工作线程。由于一直在轮询，CPU 负载持续 100%，通常的生产环境中不可接受；

2. 共享队列、条件变量：共享数据结构竞争激烈，共享队列插入时不加锁可能导致 wakeup loss，加锁对整体吞吐影响大。无法避免唤醒工作线程的 syscall

3. 每个工作线程一个队列：容易造成负载不均衡，唤醒工作线程 syscall

4. 每个工作线程一个队列、任务偷取：取决于任务偷取强度，（强度低）可能无法解决不均衡，（强度高）或造成低负载下大量无效偷取导致的各个队列的锁竞争，唤醒工作线程 syscall

由于上述算法均有较为明显的缺陷，因此我们自行设计了唤醒算法。

* **调度策略 v1 版本(默认策略): 折中平衡策略，用至多2个线程并行轮询**

  1. 在获取 Fiber 时，如果取到 Fibe r则直接运行。如果没取到 Fiber 并且轮询线程不足2个，则进入轮询状态

  2. 如果一个轮询线程能取到 Fiber，则在运行此Fiber之前，唤醒另外一个线程。轮询超时则睡眠

  3. 在生产Fiber时，如果存在轮询线程，则直接让他退出轮询去取Fiber。否则唤醒一个线程

  当然，在上述选取轮询线程/睡眠线程中，都按照固定编号排序，选择第一个（LSB）可选项。这是由于多个 Fiber 很可能是协作式任务，将他们集中在固定的线程上运行（FiberWorker 可以配置绑核，而且操作系统也会更倾向把相同的线程运行在固定的 CPU），会提高 CPU Cache 的热度。

* **调度策略 v2 版本: 优先本地队列、同时支持全局和本地队列间任务窃取（参考 taskflow runtime 的实现思想）**

  1. 任务队列由 global(mpmc) + local(spmc) 组成，支持 global 及 local 队列间的任务窃取；

  2. 当执行并行任务时, 本 worker 线程可以往其 local 队列添加任务，这时无需 notify，无系统调用开销；

  3. 线程间的通知使用 taskflow 原生的 notifier, notify开销更小；

  4. taskflow 的任务调度时, task 在执行时不能有堵塞卡住线程的情况, 一旦出现会导致其中一个线程 cpu 100%，而 fiber reactor 执行 epoll_wait 时, 没有网络事件时, 会有堵塞卡住线程的情况, 为了解决这个问题,我们将 reactor fiber 作为 io 类型的 fiber，其单独放到 global io 队列中处理，避免频繁检测到运行队列中有 fiber reactor 任务，造成采用该实现下的 worker 线程无法进入睡眠导致 cpu 100% 空转问题；

  taskflow 相关资料可以参考[taskflow executor](https://github.com/taskflow/taskflow/blob/master/taskflow/core/executor.hpp)

### 窃取

如果 Fiber 创建属性指定不允许窃取，那么 Fiber 只会在初始的调度组内执行，否则可能由其他组窃取。按照性能的不同，窃取分为两种：

1. NUMA Node内：性能与正常调度相同。纯粹为了平衡组间负载，默认开启；

2. NUMA Node间：性能较差。默认不开启；

不论是组内还是组间，其原理是一样的，只是工作线程 acquire 的目标 Queue 不同而已。而且其算法也相同，只是窃取频率系数（steal_every_n，0为禁止窃取，否则越大表示窃取频率越低）不同，可以参数指定。

## 用户接口设计

Fiber 用户态线程定位上跟 pthread 相似，所以在提供能力有一些相似之处：

| 能力描述 | pthread  |   Fiber   |    说明   |  
| :-----------------------------| :-------------------------|:----------------------------|:----------------------------------------------------------|
|创建并运行|std::thread类|Fiber类及StartFiberDetached接口|需要提供自定义运行属性以满足不同场景|
|同步控制之Latch|std::latch类|FiberLatch类|用于多任务协作同步，如创建N个并行Fiber任务并等待所有结果|
|同步控制之互斥锁|std::mutex类|FiberMutex类|互斥访问|
|同步控制之读写锁|std::shared_mutex类|FiberSharedMutex类|读优先，满足读多写少场景|
|同步控制之顺序锁|Seqlock类|FiberSeqLock类|写优先场景|
|条件变量|std::condition_variable类|FiberConditionVariable类|满足条件则通知|

除此之后还有:

1. 同外部线程交互的 FiberEvent 类；

2. 进行 Future 异步编程的工具 BlockingGet 等；
