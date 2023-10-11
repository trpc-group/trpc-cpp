
# tRPC-Cpp Runtime

为了满足多样化的业务场景需求：有重网络IO的，如接入层服务，其要求QPS尽量高；有重CPU计算的，如推荐搜索服务，其一般采用同步编程方式、关注长尾延时。
因此tRPC-Cpp框架以插件化方式支持了多种runtime类型，以不同线程模型来解决不同业务场景的需求。目前框架支持以下三种runtime: 

1. fiber m:n 协程模型
2. IO/Handle 合并线程模型
3. IO/Handle 分离线程模型

在使用框架时，用户需要根据自己的应用场景选择合适的runtime，以便获得更好的性能(QPS和延时)。

下面我们对如何选择和配置runtime以及各种runtime的具体实现进行介绍。

## 如何选择及配置runtime

### 如何选择runtime

| 类型 | 编程方式           | 优缺点                                                       | 适用场景                                                     |
| ----------- | -------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| fiber m:n 协程 | 协程同步编程          | 优点：采用协程同步编程方式，方便编写逻辑复杂的业务代码；网络IO和业务处理逻辑可多核并行化，能充分利用多核，做到很低的长尾延时。<br>缺点：为了不阻塞线程，可能需要使用特定的协程同步原语进行同步，代码侵入性较强；由于协程数受系统限制，且协程调度存在额外的开销，在QPS或连接数较大场景下性能表现不够好。| 适合CPU密集型、对长尾延时要求高的业务场景，如推荐/搜索/广告类服务 |
| IO/Handle 合并 | future/promise异步编程 | 优点：请求处理不跨线程，平均延时低。<br>缺点：业务处理逻辑无法多核并行化，不适合重CPU计算、有阻塞逻辑的场景。另外，在连接少或请求的计算量不均时容易出现worker线程负载不均的问题。 | 适合网络IO密集型，对延时要求低、QPS要求高的业务场景，如接入层网关服务 |
| IO/Handle 分离 | future/promise异步编程 | 优点：比较通用，允许业务逻辑有部分阻塞操作。<br>缺点：请求处理需跨两个线程，存在额外的调度延时和cache miss，影响平均延时；请求处理逻辑默认只在当前handle线程执行，在需要多核cpu并行计算场景在编程上不太便利(需增加将任务放到全局队列的逻辑)。 | 对QPS/延时没有极致要求的业务场景 |

使用时根据业务场景选择一种runtime即可，最好不要混用多种runtime。

### 配置Runtime

选好runtime后，需要在框架配置文件中配上对应的配置项：

1. fiber m:n协程模型

```yaml
global:
  threadmodel:
    fiber:
      - instance_name: fiber_instance  # thread model instance name, required
        concurrency_hint: 8            # number of fiber worker threads, if not configured, it will adapt based on machine information automatically
```

2. IO/Handle合并线程模型

```yaml
global:
  threadmodel:
    default:
      - instance_name: merge_instance  # thread model instance name, required
        io_handle_type: merge          # running mode, separate or merge, required
        io_thread_num: 8               # number of io thread, required
```

3. IO/Handle分离线程模型

```yaml
global:
  threadmodel:
    default:
      - instance_name: separate_instance  # thread model instance name, required
        io_handle_type: separate          # running mode, separate or merge, required
        io_thread_num: 2                  # number of io thread, required
        handle_thread_num: 6              # number of handle thread, required in separate model, no need in merge model
```

更多配置项说明，详见[框架配置](framework_config_full.md)

## 各种runtime的具体实现

### fiber m:n 协程模型
![img](/docs/images/fiber_threadmodel_arch.png)

类似golang的goroutine，通过将网络IO任务和业务处理逻辑封装成协程任务，由用户态的协程调度器调度到多核并行执行。

实现上采用了多调度组的设计，且支持numa架构，这样numa架构下不同cpu node可以跑不同的调度组，从而减少交互跨node交互。

一个调度组内所有的worker线程共享一个global queue，并支持调度组间的任务窃取，这样任务可多核并行化处理。另外，在v2调度器中，每个worker线程有自己的local queue(单生产多消费)，这样可以做到任务的生产和消费尽可能在同一线程，进一步减少cache miss。

### IO/Handle 合并线程模型

![img](/docs/images/merge_threadmodel_arch.png)

借鉴了shared-nothing的思想，每个Worker线程都有自己的任务队列(多生产者单消费者)和reactor，将某个连接上的网络IO和业务逻辑固定在同一个线程处理。这样该连接上的请求在固定线程处理，无线程切换开销，cache miss最少，因此可以做到很低的平均延时，从而达到很高的QPS。

但也因为每个线程上的请求需要排队进行处理，不能多核并行化，如果某个请求的处理逻辑比较重，则容易影响后续请求的处理，故不适合重CPU计算场景。

### IO/Handle 分离线程模型

![img](/docs/images/separate_threadmodel_arch.png)

IO线程负责处理网络收发逻辑，Handle线程负责处理业务逻辑，使用时用户需要根据应用场景自行调整IO和Handle线程配比。

实现上每个IO线程有自己的任务队列，Handle线程间共享global queue，可并行执行global queue中的任务，因此少量的阻塞业务逻辑不影响请求的处理，普适性较强。另外，每个Handle线程都有自己的local queue(多生产者单消费者)，请求处理可以在指定的线程执行。

但由于请求处理跨两个线程，在网络IO密集型场景下平均延时和QPS不如合并线程模型；某个reactor上的网络IO事件无法多核并行处理，且请求处理逻辑默认只在当前线程执行，在CPU密集场景下长尾延时不如fiber m:n协程模型。
