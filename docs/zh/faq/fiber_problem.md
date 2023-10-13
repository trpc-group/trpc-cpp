
[English](../../en/fiber_faq.md)

# 前言

我们发现，在实际Fiber使用时，并非是对Fiber的特性不了解或是不会使用，而更多的是易出错，概括起来有两大类问题，一类是性能相关，一类是Coredump相关。

# 性能相关

## 性能表现较差，怎么解决？

针对Fiber的性能问题，可以按照以下思路排查：

1. 如果做性能对比，Fiber模式下相对于原版本相差较大：检查是否启用编译优化比如O2级别；检查对比的库是否是完全一样，比如原版本用cmake编译依赖第三方库版本，跟Fiber用bazel编译依赖的依赖第三方库版本不一致；

2. 如果机器负载较低，且一直机器负载压不上去，检查：是否存在比较大的临界区？有的话，需要拆小锁粒度；是否有同步阻塞逻辑？有的话，调整为异步操作，比如投递到线程池中处理；

3. 如果机器负载能跑上去，QPS压力大时比较容易出现性能差，可以再试下适当调整Fiber配置：比如配置多个FiberWorker(推荐跟实际可用核数相等)、需要任务相互隔离的尝试设置多个调度组相互隔离、禁用NUMA窃取等；

4. 如果有使用ServiceProxy访问下游时，需要选择合适的连接模式：优选连接复用(前提是协议有关联id)，其次选择Pipeline模式(前提是协议支持Pipeline也就是响应按照请求顺序回复，如原生Redis Server)，再次连接池模式(如http协议，最大连接数max_conn_number最好配置大一些)，尽量不要使用短连接模式(短连接会每次请求时创建连接请求结束销毁连接，较为消耗资源)；

## 压测后端Fiber开发的Server，整体QPS相同，上游压测连接较多时，后端Fiber Server性能下降比较明显？

这个是符合预期的。Fiber模式下处理RPC的P99指标延时较好本身就是借助于不同连接的高并行、同一连接的高并行(以可读事件为例，同一连接的不同可读事件都是并行处理的)实现的，这一实现本身是需要创建Fiber来处理的。所以QPS相同时，上游连接越多，分散到后端Fiber Server的连接上也就越多，连接上需要创建的Fiber也就越多，Fiber创建的越多，Fiber本身的创建和执行时需要消耗资源的，也就是性能损耗也就越多，所以在连接较多时，使用Fiber性能下降明显。

## 发起调用时间和实际执行时间相差较大？

从发起调用到最终实际执行，需要经历构造->入队列->等待有空闲FiberWorker来调度该Fiber。如果中间差距比较大，可能是创建的fiber在队列里进行排队等待处理，排队等待的原因可能是Fiber工作线程比较忙，需要增加fiber工作线程个数，也可能是在fiber工作线程上运行的fiber任务太慢；

所以可以尝试减少当前创建的Fiber个数、优化占用较多CPU资源的Fiber任务、调大消费者FiberWorker线程个数、设置多个调度组相互隔离等手段。

## 是否可以在Fiber中使用线程级别同步原语(如std::mutex)？

看情况。如果临界区很小或者竞争较少，可以使用线程级别同步原语，因为如果使用Fiber级别同步原语可能会导致协程重新调度会影响性能表现，虽然线程级别同步原语会阻塞当前Fiber运行的线程，但是如果粒度小的话，影响也较小；

当然如果临界区较大或者竞争较多，还是推荐使用Fiber级别同步原语。

## 内存占用较高，怎么解决？

如果有使用ServiceProxy客户端访问下游，且客户端使用了Pipeline，且后端节点较多(比如访问Redis Server，节点成百甚至上千)，可以适当调小fiber_pipeline_connector_queue_size及max_conn_num；

如果有使用Fiber同步原语，检查是否存在同步原语不对(比如FiberLatch一直没有CountDown)导致对应的Fiber无法释放，建议加上异常处理保证Fiber能被执行完毕。

## 如何获得当前堆积的Fiber任务个数，如何获取当前所有Fiber任务的个数？

获得当前堆积的Fiber任务个数：通过trpc::fiber::GetFiberQueueSize()接口，这个值本身也会trpc.FiberTaskQueueSize作为指标上报。如果这个指标长时间都比较大，说明可能当前系统存在Fiber任务堆积。

获取当前所有Fiber任务的个数：通过trpc::GetFiberCount()接口，表示在生命周期内的Fiber任务总个数。实现层面：在每次创建Fiber时总计算自加一，在每个Fiber执行完毕释放资源时总计数自减一。

## 使用默认Fiber调度策略(v1)，出现FiberWorker负载不均匀，这种符合预期吗？

如果当前系统负载较低，比如Fiber个数较少，此时是有可能负载不均的，体现在调度组内某两个FiberWorker线程负载偏高，此时是符合预期的。
具体原因是：默认Fiber调度策略会优先调度某两个FiberWorker，所以就会在系统负载较低时，同调度组内某两个FiberWorker线程负载比其他的负载高。

此时可以尝试调大QPS，加大系统负载，此时就不应该出现负载不均现象了。

## vm.max_map_count 过小导致 Fiber 创建失败？

每分配一个Fiber栈会引入两个内存段（/proc/self/maps中表现为一行），它们分别作为Fiber栈和用于检测栈溢出的guard page。

Linux系统会限制每个进程所允许的最多的内存段的个数，默认值为65536。对于QPS高且单个请求处理时间长（并发请求量大）的服务，可能会达到这一上限（32768，考虑到对象池导致的线程局部缓存的栈对象以及其他代码、文件映射占用的内存段，上限大约是30K个Fiber）。

这一参数可以通过修改vm.max_map_count来解决。具体大小可以视业务需要修改。

修改方式：

```sh
echo 1048576 > /proc/sys/vm/max_map_count
```

这一选项的副作用可以参考Side effects when increasing vm.max_map_count，通常不会对服务及机器产生影响。

## Fiber 队列溢出怎么处理（Run queue overflow. Too many ready fibers to run.）？

如果Fiber数量过多，会出现Fiber 队列溢出现象，有类似日志:

```txt
Run queue overflow. Too many ready fibers to run. If you're still not overloaded, consider increasing run_queue_size
```

可考虑适当调大fiber配置中的run_queue_size配置项。

# Coredump 相关

## Coredump，如何排查？

1. 启动时Core掉，检查是否配置合法的concurrency_hint？解法办法：如果不手动配置concurrency_hint，框架会读取系统中的cpu信息(比如/proc/cpuinfo)，如果在容器状态下，可能存在读取系统cpu信息和实际可用核数不准确现象；Fiber环境初始化会申请资源，一旦concurrency_hint较大，可能会导致资源不够申请失败而Core掉。所以建议手动调整为实际可用核数1-2倍(经验值)。

2. 检查是否存在Fiber创建过多？解法办法：通过观察trpc.FiberTaskQueueSize指标验证是否存在长时间堆积，如果有的话考虑减小QPS、增大FiberWorker个数、再或者使用Fiber限流保证有损服务；

3. 检查是否有多线程竞争不安全问题？解法办法：加锁保护或者源头上避免竞争；

4. 检查是否存在栈变量较大，比如大于默认Fiber栈大小128K?解法办法：调大配置项fiber_stack_size的默认栈大小；或者尽量不要定义比较大的栈内存变量，换成堆内存；

5. 检查是否存在捕获时和执行时的生命周期问题? 解法办法：因为Fiber执行是异步的，而捕获是实时的，所以可能存在执行时的变量生命周期已经失效的场景这种情况很容易出现Coredump，所以推荐使用智能指针引用捕获、std::move值、再或者直接值捕获拷贝等方式；

6. 检查是否未捕获异常，tRPC-Cpp编程不使用异常，所以需要自行处理异常，如果Fiber中出现为捕获的异常，会导致Crash；

# 其他

## Fiber 下能否使用异步 RPC 接口访问下游？

支持。

## 是否可以配置多个 Fiber 线程实例？

目前 Fiber 线程实例只支持配置一个，不支持多个。

## 如果需要编写 Fiber 下的单元测试？

可以考虑使用RunAsFiber快捷工具：

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

## 能否在非 FiberWorker 线程运行 Fiber 特性比如 Fiber 同步原语？

目前不支持，如果使用会Check "IsFiberContextPresent" 失败并Coredump；

后续会支持，敬请期待。
