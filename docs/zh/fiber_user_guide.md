
# 1 前言

本文从开发者角度介绍如何使用Fiber，包括配置、常用类及接口、常见问题等几部分。关于Fiber原理可以阅读[Fiber](./fiber.md)。

# 2 配置
目前Fiber的功能需要在Fiber执行环境中运行，有Fiber执行环境第一步就需要正确的Fiber配置，这里只展示Fiber部分的配置。

为简化使用，开发者只需要填写一些必填的配置项就即可，以下是精简Fiber配置:
```yaml
global:
  threadmodel:
    fiber:                              # 使用 Fiber(M:N协程) 线程模型
      - instance_name: fiber_instance   # 线程实例唯一标识，目前暂不支持配置多个Fiber线程实例
        concurrency_hint: 8             # 运行的Fiber Worker物理线程个数，建议配置为实际可用核数个数(否则读取系统配置)
xxx
```
其中推荐手动填入concurrency_hint配置项，避免出现因读取系统配置而导致的问题。


除了精简版配置，还可以自定义高阶配置项，以下是完整Fiber配置:
```yaml
global:
  threadmodel:
    fiber:
      - instance_name: fiber_instance
        concurrency_hint: 8                                       #建议配置，表示共创建多少个fiber worker物理线程来运行fiber任务。如果不配置默认值是读取/proc/cpuinfo个数(容器场景下实际可用核数可能远低于/proc/cpuinfo值，会造成fiber worker线程频繁切换影响性能)，所以建议此值和实际可用核数相同。
        scheduling_group_size: 4                                  #建议配置，表示每个调度组(为了减小竞争，框架引入多调度组来管理fiber worker物理线程)共有多少个fiber worker物理线程。如果不配置默认框架会依据concurrency_hint值和策略自动创建一个或者多个调度组。如果希望当前只有一个调度组，将此值配置同concurrency_hint一样即可。如果希望有多个调度组，可以参考展示配置项:表示每个调度组有4个fiber worker物理线程，共有2个调度组。
        reactor_num_per_scheduling_group: 1                       #表示每个调度组共有多少个reactor模型，如果不配置默认值为1个。针对io比较重的场景，可以适当调大此参数，但也不要过高。 
        reactor_task_queue_size: 65536                            #表示reactor任务队列的大小
        fiber_stack_size: 131072                                  #表示fiber栈大小，如果不配置默认值为128K。如果需要申请的栈资源较大，可以调整此值
        fiber_run_queue_size: 131072                              #表示每个调度组的Fiber运行队列的长度，必须是2幂次，建议和可用Fiber分配的个数相同或稍大。
        fiber_pool_num_by_mmap: 30720                             #表示通过mmap分配fiber stack的个数
        numa_aware: false                                         #表示是否启用numa，如果不配置默认值为false。配置为true表示框架会将调度组绑定到cpu nodes(前提是硬件支持numa)，false表示由操作系统调度线程运行在线程运行在哪个cpu上。
        fiber_worker_accessible_cpus: 0-4,6,7                     #表示需要指定运行在特定的cpu IDs，如果不配置默认值为空。如果希望指定，参考当前展示配置项：表示指定从0到4，还有6,7这几个cpu ID
        fiber_worker_disallow_cpu_migration: false                #表示是否允许fiber_worker在不同cpu上运行，也就是是否绑核，如果不配置默认值为false也就是默认不绑核。
        work_stealing_ratio: 16                                   #表示不同调度组之间任务窃取的比例，如果不配置默认值是16，表示按照16%比例进行任务窃取。
        cross_numa_work_stealing_ratio: 0                         #表示numa架构不同node之间偷取任务频率(v1调度器版本实现支持)，如果不配置默认值为0表示不开启(开启会比较影响效率，建议实际测试后再开启)
        fiber_stack_enable_guard_page: true                       #是否启用fiber栈保护，如果不配置默认值为true，建议启用。
        fiber_scheduling_name: v1                                 #表示fiber运行/切换的调度器实现，目前提供两种调度器机制的实现：v1/v2，如果不配置默认值是v1版本即原来fiber调度的实现，v2版本是参考taskflow的调度实现
```


# 3 常用类及接口
## 1 创建运行Fiber

接口形式：
```cpp
// @brief 创建Fiber并运行，按照默认属性
// @note  注意如果创建太多系统内存则会返回失败false.
bool StartFiberDetached(Function<void()>&& start_proc);

// @brief 创建Fiber并运行，可以自定义属性
bool StartFiberDetached(Fiber::Attributes&& attrs, Function<void()>&& start_proc);
```
创建Fiber默认的运行属性是由框架发起调度(可能会在其他调度组)并执行，当然也可以自定义Fiber运行属性Fiber::Attributes:
```cpp
  struct Attributes {
    // 如何启动一个Fiber, 默认Post会将Fiber入队等待调度执行，可选Dispatch方式(马上执行)
    fiber::Launch launch_policy = fiber::Launch::Post;

    // Fiber用什么策略选择在哪个线程组中执行，默认是选择最近的(即来自同一个主调线程的Fiber在同一个线程组中执行)
    // 可选随机kUnspecifiedSchedulingGroup或者指定(0-线程组个数-1)来选择固定线程组
    std::size_t scheduling_group = kNearestSchedulingGroup;

    // 是否允许Fiber在其他线程组执行，默认是允许的
    bool scheduling_group_local = false;
  };
```


如创建一个默认属性的Fiber：
```cpp
trpc::StartFiberDetached([&] {
  Handle();
});
```


也可以指定Fiber::Attributes，如指定Fiber在某个调度组中运行：
```cpp
trpc::Fiber::Attributes attr;
attr.scheduling_group = 1;

trpc::StartFiberDetached(attr, [&] {
  Handle();
});
```

## 2 Fiber间多任务协作
用于Fiber间多任务之间协作的FiberLatch，示例如:
```cpp
trpc::FiberLatch l(1);
trpc::StartFiberDetached([&] {
  Handle();
      
  // 这次执行单元即将结束，FiberLatch减一，减到0会唤醒同步等待操作
  l.CountDown();
});    
    
 // 同步等待FiberLatch的值减至0，才唤醒
l.Wait();
```


FiberLatch接口:

|接口名称 |  功能 |参数 |返回值|
| :-------------------------------------------------------| :---------------------|:---------------------|:----------------------|
| FiberLatch(std::ptrdiff_t count) |  创建一个内部计数为count的FiberLatch|count 内部计数 | void |
| CountDown(std::ptrdiff_t update = 1) |  内部计数减去update|update 内部计数减去 | void |
| TryWait() |  内部计数实际已经等于0| 无 | bool，如果内部计数等于0返回true，否则返回false |
| Wait() |  阻塞等待直到内部计数等于0| 无  | void |
| ArriveAndWait(std::ptrdiff_t update = 1) |  CountDown()和Wait()的合并接口| | void |
| bool WaitFor(const std::chrono::duration<Rep, Period>& timeout)  | 在超时时间内等待内部计数等于0|timeout 时间点 | void |
| bool WaitUntil(const std::chrono::time_point<Clock, Duration>& timeout)  | 在超时时间内等待内部计数等于0|timeout 时间点 | void |

## 3 Fiber间互斥访问
用于Fiber之间共享数据互斥访问FiberMutex，示例如:
```cpp
trpc::FiberLatch latch(10);
trpc::FiberMutex mu;
for(size_t i=0; i<10; ++i) {
  trpc::StartFiberDetached([&latch, &mu] {
    for(size_t i=0; i<1000; ++i) {
      // FiberMutex互斥访问数据
      std::scoped_lock _(mu)

      Update();
    }

    latch.count_down();
  });
}

latch.wait();
```

注意：

- FiberMutex可以配合std::scoped_lock等RAII工具类使用，避免忘记释放锁资源。

FiberLatch接口:

|接口名称 |  功能 |参数 |返回值|
| :-------------------------------------------------------| :---------------------|:---------------------|:----------------------|
| void wait(std::unique_lock<FiberMutex>& lock)  | 等待直到notify_xxx()  | lock FiberMutex锁| void |
| void lock()  | 加锁保护共享数据 | 无 | void |
| void unlock()  | 释放锁并唤醒对应的Fiber | 无 | void |
| bool try_lock() | 是否可以加锁，即当前已经被加锁过则返回失败 | 无 | bool，加锁过则返回失败，否则返回true |

## 4 共享数据读优先
用于Fiber之间共享数据读优先读写锁:FiberSharedMutex，示例如:
```cpp 
trpc::FiberSharedMutex rwlock_;

// 使用写锁互斥
void Update(){
  rwlock_.lock();
  // ...Update

  rwlock_.unlock();
}
 
// 使用读锁互斥
void Get(){
  rwlock_.lock_shared();
  // ...Get

  rwlock_.unlock_shared();
}
```
## 5 共享数据写优先
用于Fiber之间共享数据写优先读写锁:FiberSeqLoc，示例如:
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
## 6 条件通知
用于Fiber条件通知:FiberConditionVariable，示例如:
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

FiberConditionVariable接口:

|接口名称 |  功能 |参数 |返回值|
| :-------------------------------------------------------| :-------------------------------|:--------------------------|:--------------------------------|
| void wait(std::unique_lock<FiberMutex>& lock)  | 等待直到notify_xxx()  | lock FiberMutex锁| void |
| std::cv_status wait_for(std::unique_lock<FiberMutex>& lock,const std::chrono::duration<Rep, Period>& expires_in)  | 等待直到notify或者超时  | lock FiberMutex锁，expires_in时间 | cv_status ，如果超时内被唤醒则返回std::cv_status::no_timeout，否则返回std::cv_status::timeout |
| std::cv_status wait_until(std::unique_lock<FiberMutex>& lock,const std::chrono::time_point<Clock, Duration>& expires_at)  | 等待直到notify或者超时  | lock FiberMutex锁，expires_at时间 | cv_status ，如果超时内被唤醒则返回std::cv_status::no_timeout，否则返回std::cv_status::timeout |
| notify_one()  | 唤醒其中一个等待waiter | | void |
| notify_all()  | 唤醒所有等待waiter | | void |
| void wait(std::unique_lock<FiberMutex>& lock, Predicate pred)  | 一直到pre符合条件，才开始Wait | lock FiberMutex锁，pred 符合的条件| void |

## 7 定时任务
有需要再Fiber中执行定时任务的需求，目前提供以下几种方式：

- 1.SetFiberTimer创建并启用+KillFiberTimer手动释放:
```cpp
// 起一个fiber timer，到100ms后执行
auto timer_id = SetFiberTimer(trpc::ReadSteadyClock() + 100ms, [&]() {
  Handle();
});
 
// 删除对应timer，回收资源 
KillFiberTimer(timer_id);
```

- 2.SetFiberTimer创建并启用+FiberTimerKiller RAII式释放:
```cpp
// 类中定义一个成员变量FiberTimerKiller
FiberTimerKiller killer;
killer.reset(SetFiberTimer(trpc::ReadSteadyClock() + 100ms, [&]() {
  Handle();
}));

// FiberTimerKiller析构时自动释放对应的timer

```
- 3.CreateFiberTimer创建+EnableFiberTimer启用:
```cpp
// 创建一个fiber timer，到100ms后执行
auto timer_id = CreateFiberTimer(trpc::ReadSteadyClock() + 100ms, [&](auto timer_id) {
  // 注意这里是不是在FiberWorker中运行,所以在cb不能使用Fiber特性
  Handle();
});
 
// 启用
EnableFiberTimer(timer_id);

// 注册的定时回调
void OnTimeout(std::uint64_t timer_id){
  HandleTimeout();

  // 关闭定时器资源
  KillFiberTimer(timer_id);
}
```

注意：
- SetFiberTimer接口会创建一个Fiber运行自定义cb，所以可以在cb使用Fiber特性；
- CreateFiberTimer接口则是在自己的TimerThread中运行自定义cb，并没有运行在FiberWorker中，所以在cb不能使用Fiber特性；

## 8 同非框架线程同步协作
用于同非框架线程的同步协作FiberEvent
```cpp
auto ev = std::make_unique<FiberEvent>();

// 外部线程，非框架FiberWorker
thread_pool.push([&](){
  Handle();

  // 唤醒FiberEvent
  ev->Set();

});

// FiberEvent等待
ev->Wait();
```

## 9 异步编程
也支持在Fiber中使用trpc::Future进行异步编程:
```cpp
trpc::StartFiberDetached([&] {
  auto rpc_future = Async_rpc();
  
  // 通过BlockingGet阻塞等待当前Fiber执行结果
  auto rpc_result = trpc::fiber::BlockingGet(rpc_future);
  
  // 按照trpc::Future编程
  if(rpc_result.IsReady()){
    ...
  }else{
    ...
  }
});

```

# 4 FAQ
查阅[Fiber FAQ](./fiber_faq.md)
