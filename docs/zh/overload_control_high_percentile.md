[English](../en/overload_control_high_percentile.md)

# 前言

过载保护算法通常可以分为两类：

- 非自适应过载保护；指的是输出不会影响到系统的输入、事先将跟控制相关的因素/参数都考虑设置好的控制策略，或者说是没有反馈的控制策略
- 自适应过载保护；指的系统的输出会影响到系统的输入（通常来说是负反馈系统）

tRPC-Cpp 框架已支持：基于流量控制限流，并发请求限流，并发 Fiber 数量限流都属于非自适应过载保护；它们存在共同的缺点：**很难给出一个通配的数值，即使是同一个模块，由于业务逻辑的变化，可能随时也需要调整配置，维护起来不方便**。为了解决此问题，框架提供了一种自适应过载保护策略，也即是本文主要介绍的：**基于并发度的自适应过载保护策略**。

*注意：基于并发请求数限流的最大并发数是配置的固定值，基于并发度的自适应过载保护策略的最大并发数是动态变化的；二者都是和并发请求有关，但是差异很大。*

# 基于并发度的自适应过载保护策略

基于并发度的自适应过载保护策略使用的算法比较复杂，本文后续部分会做详细的介绍，简单来说就是根据 QPS 来反应当前系统的负载程度。其有以下特性：

- 适用场景

  **`高 QPS 导致的系统高负载，也即是 IO 密集型适合适用`**；而**低 QPS 高负载的 CPU 密集型则不太适合使用本策略**。

- 支持优先级

  该过载保护策略支持解析**请求携带优先级**，并在过载发生时，丢弃低优先级的请求。在协议上，默认情况下，请求中通过上下文 `transinfo` 中携带优先级，并随着链路透传。在 `transinfo` 中，优先级使用 key 为 `trpc-priority`，value 为一个字节长度的 uint8 来表示，允许请求优先级范围是 `0-255`；若 key 不存在，或 value 长度不是一个字节时，则认为此请求的`默认优先级是 0`。
  插件也**支持自定义优先级协议**，参照：[SetServerGetPriorityFunc](../../trpc/overload_control/common/request_priority.cc)。
  当所有请求都不携带优先级时，过载保护仍然能够工作，并在过载发生时，**随机丢弃**一部分请求。

- 支持 dry-run 模式

  由于对于不同服务类型，**有可能过于灵敏（误限）或不够灵敏（少限）**；所以建议前期**开启 dry-run 模式、开启上报进行足够观察**（参照本文的配置一节），在线上服务上报数据与线下压测表现都符合预期时，再关闭 dry-run 模式，开启保护。

- 限制

  过载保护的服务端插件，支持 **separate 和 fiber 线程模型**。`merge 线程模式`由于其自身的特性，不会入队，所以**默认配置下不支持该策略**，仅支持基于业务函数处理时长的指标，即`max_request_latency` 配置字段，相应的配置字段 `max_schedule_delay` 应设置为 0 （禁用）。

## 基于并发度的自适应过载保护原理图

![high_percenttile_limiter](../images/high_percenttile_limiter.png)
图中核心点就是 `HighPercentileServerFilter` ，它的主要作用是获取框架当前的并发请求数（`concurrency_reqs`）与动态计算的最大并发数（`max_concurrency`）进行比较来判断是否拒绝当前接收的新请求（`curr_req`），`max_concurrency` 是基于 little`s law 原理计算获取的。

## 实现代码

  框架的过载保护实现方式是基于[过滤器](./filter.md)，为了保证尽可能早进行限流保护；该过滤器会埋点在请求入队前，具体埋点实现如下：

  ```cpp
  std::vector<FilterPoint> HighPercentileServerFilter::GetFilterPoint() {
    return {
        FilterPoint::SERVER_PRE_SCHED_RECV_MSG,
        // ...
    };
  }
  ```

  源码参考：[high_percentile_server_filter](../../trpc/overload_control/high_percentile/high_percentile_server_filter.cc)

# 使用示例

基于并发度的自适应的过载保护过滤器，**当前只能应用于服务端，客户端暂时不支持**，用户使用无需修改任何代码，只需开启编译和增加配置即可，非常方便。

## 编译选项

编译选项：在`.bazelrc` 文件中加入下面一行

```sh
build --define trpc_include_overload_control=true
```

## 配置文件

并发请求过滤器配置如下（详细配置参考：[high_percentile.yaml](../../trpc/overload_control/high_percentile/high_percentile.yaml)）：

```yaml
#Server configuration
server:
  app: test #Business name, such as: COS, CDB.
  server: hello #Module name of the business
  service: #Business service, can have multiple.
    - name: trpc.test.hello.Route #Service name, needs to be filled in according to the format, the first field is default to trpc, the second and third fields are the app and server configurations above, and the fourth field is the user-defined service_name.
      protocol: trpc #Service application layer protocol, for example: trpc, http.
      network: tcp #Network listening type: for example: TCP, UDP.
      ip: 127.0.0.1 #Listen ip
      port: 10104 ##Listen port
      filter:
        - high_percentile
#Plugin configuration.
plugins:
#  metrics:
#    prometheus:
      # ...
  overload_control:
    high_percentile:
      # Expected maximum scheduling delay (in ms). If exceeded, it is considered overloaded. A value of 0 means this load metric is not enabled. It is recommended to set it to 15 or 20.
      max_schedule_delay: 10
      # Expected maximum downstream request delay (in ms). If exceeded, it is considered overloaded. A value of 0 means this load metric is not enabled.
      max_request_latency: 0
      # ...
      # Perform metric statistics and overload algorithms without intercepting requests, for experimental observation.
      dry_run: false
      # Whether to report overload protection monitoring metrics.
      is_report: true
```

配置关键点如下：

- high_percentile：基于并发度的自适应过载保护器名称
- max_schedule_delay：期望的最大线程调度延迟（ms），表示请求在队列里面的期望的最大等待时间，算法结果对该参数的改变较为敏感。在 `separate` 和 `fiber` 模式下使用；该值只有大于 0 才有意义。
- max_request_latency：期望的最大业务处理延迟，或称请求下游延迟（ms），该参数只是表示业务耗时，不一定表示当前系统的负载程度，用于解决 `merge` 模式下请求无入队操作导致 `max_schedule_delay` 失效的问题，仅适合 io 密集型场景；该值只有大于 0 才有意义。
- dry_run：启用时，会模拟执行保护策略，并执行上报，但不会真正地拦截请求
- is_report：是否上报监控数据到监控插件，**注意，该配置必须与监控插件一起使用(例如配置：plugins->metrics->prometheus，则会上报到 prometheus 上)，如果没有配置监控插件，该选项无意义**，被监控数据有：
  - `Pass`：单个请求的通过状态，0：拦截；1：通过。每次请求的监控结果在 `Pass`、`Limited`，`LimitedByLower` 三者中只有一个为 1；另外两个为 0
  - `Limited`：单个请求因为过载被拦截状态，1：拦截；0：通过。
  - `LimitedByLower`: 单个请求的拦截状态因为优先级被拦截的状态，1：拦截；0：通过。
  - `/{callee_name}/{method}`: 上报监控的 RPC 方法名，属于固定值；由被调服务(callee_name)和方法名(method)组成，例如：`/trpc.test.helloworld.Greeter/SayHello`。
  - 监控算法执行结果的相关参数：
    - `/{callee_name}/{method}/{strategy}/expected`：上报配置的 `max_schedule_delay` 或者 `max_request_latency`的值（用`expected`表示），例如：`/trpc.test.helloworld.Greeter/SayHello/max_schedule_delay/expected`，表示配置 `max_schedule_delay` 大于 0 时的值，用于校验配置参数是否生效。
    - `/{callee_name}/{method}/{strategy}/measured`：上报算法计算的请求处理耗时的高百分位值（用 `measured` 表示），后续算法介绍的$M_{expect}$值。例如：`/trpc.test.helloworld.Greeter/SayHello/max_schedule_delay/measured`，表示当配置 `max_schedule_delay` 大于 0 时，请求处理耗时的高百分位值。
    - `/{callee_name}/{method}/{strategy}/quotient`：上报 `expected/measured`  的值（用 `quotient` 表示），例如：`/trpc.test.helloworld.Greeter/SayHello/max_schedule_delay/quotient`，表示当配置 `max_schedule_delay` 大于 0 时，`expected/measured` 的比值。
    - `/{callee_name}/{method}/max_measured`：上报最大的 `measured`（用 `max_measured` 表示），例如：`/trpc.test.helloworld.Greeter/SayHello/max_measured`。
    - `/{callee_name}/{method}/max_concurrency`：上报计算当前系统能允许的最大并发度（用 `max_concurrency` 表示），例如：`/trpc.test.helloworld.Greeter/SayHello/max_concurrency`。
    - `/{callee_name}/{method}/cur_concurrency`：上报当前系统的并发度（用 `cur_concurrency` 表示），例如：`/trpc.test.helloworld.Greeter/SayHello/cur_concurrency`。
    - `/{callee_name}/{method}/lower`：上报当前系统算法计算的请求优先级的下阈值，例如：`/trpc.test.helloworld.Greeter/SayHello/lower`。
    - `/{callee_name}/{method}/upper`：上报当前系统算法计算的请求优先级的上阈值，例如：`/trpc.test.helloworld.Greeter/SayHello/upper`。
    - `/{callee_name}/{method}/must`：上报算法计算的请求优先级大于 `upper` 的请求个数，例如：`/trpc.test.helloworld.Greeter/SayHello/must`。
    - `/{callee_name}/{method}/may`：上报算法计算的请求优先级处于 `[lower ,upper)` 区间的请求个数，，例如：`/trpc.test.helloworld.Greeter/SayHello/may`。
    - `/{callee_name}/{method}/ok`：上报算法计算的请求优先级处于 `[lower ,upper)` 区间时，通过的请求数，例如：`/trpc.test.helloworld.Greeter/SayHello/ok`。
    - `/{callee_name}/{method}/no`：上报算法计算的请求优先级小于 `lower` 的请求个数，例如：`/trpc.test.helloworld.Greeter/SayHello/no`。

# 算法基本原理

## 并发数算法

系统当前的并发度 $concurrency_{curr}$ 相对易于得到（使用一个 atomic 变量，每次请求开始 +1，请求结束 -1 即可），如果我们能估计系统的最大并发度 $concurrency_{max}$，那么每次请求前对比 $concurrency_{curr}$ 与 $concurrency_{max}$，即可决定此次请求是否允许通过，从而保护系统并发量不会过大。

并发数控制算法基于 [Little's Law](https://en.wikipedia.org/wiki/Little%27s_law)：

```math
concurrency = cost_\text{avg} \times qps \quad
```

根据上述公式，在一段窗口时间内，有：

```math
concurrency_\text{max} = \text{min}(cost_\text{avg}) \times \text{max}(qps_\text{success}) \quad
```

即当服务恰好满负荷（即将过载，轻微过载）时，可以通过监控过去一小段时间内的最小请求开销 $min(cost_{avg})$ 和最大成功请求的每秒请求数量 $max(qps_{success})$，从而估算系统能够支持的最大并发度。

上述公式仅在服务恰好满负荷时有效，我们希望：

1. 在服务轻载的时候，完全不拦截请求；
2. 在服务中负载时，按照上述公式算出的并发度偏小（ $max(qps_{success})$ 未达到最大），能够乘以某个大于 1 的比例，近似预估最大并发度；
3. 在服务恰好满负载时，按照上述公式，预估最大并发度；
4. 在服务过负载时，按照上述公式算出的并发度偏大（ $min(cost_{avg})$ 因过载偏大），能够乘以某个小于 1 的比例，近似预估最大并发度。

我们可以选定一个监控指标 $M$（默认可以使用线程/协程调度延迟，见下一小节《负载指标》），其有一个期望值 $M_{expect}$（例如，10ms），系统周期性地监控这个指标的实际值 $M_{measured}$，当 $M_{expect}$ 接近 $M_{measured}$ 时，标志着系统接近满负荷，同时  $M_{expect}$ 与 $M_{measured}$ 相差较远时，两者的比值可以表示系统的负载程度，并通过某种算法，计算出一个修正系数 $F(M_{measure}, M_{expect})$，来估算系统正确的最大并发度（对应上述 2. 和 4. 两种情况），见以下公式：

```math
concurrency_\text{max} = \mathcal{F}(\mathcal{M}_\text{measure}, \mathcal{M}_\text{expect}) \times \text{min}(cost_\text{avg}) \times \text{max}(qps_\text{success}) \quad
```

假如 $F(M_{measure}, M_{expect})$ 表现良好，则意味着我们可以在任何负载下，都可以估算系统的最大并发度 $concurrency_{max}$。

经实验，$F(M_{measure}, M_{expect})$ 可以取如下形式：

```math
\begin{equation}
\mathcal{F(M_\text{measure}, M_{expect} )} = \begin{cases}
\infty & \text{if } \mathcal{M}_{measure} < \alpha \times \mathcal{M}_{expect} \\
\mathcal{M}_\text{expect} / \mathcal{M}_\text{measure} & \text{if } \alpha \times \mathcal{M}_{expect} < \mathcal{M}_{measure} < \mathcal{M}_{expect} \\
\sqrt{\frac{\mathcal{M}_{expect}}{\mathcal{M}_{measure}}} & \text{if } \mathcal{M}_{measure} \ge \mathcal{M}_{expect}
\end{cases}
\end{equation}
```

其中，第一种情况表示系统轻载，此时取 $concurrency_{max} = \infty$，表示不需要执行任何拦截保护。

其中 $\alpha$ 通常取 0.5 即可，即：

- $ M_{measure} < 0.5 * M_{expect} $ 表示系统处于轻负载
- $ 0.5 * M_{expect} < M_{measure} < M_{expect} $ 表示系统处于中负载
- $ M_{measure} > M_{expect} $ 表示系统处于过载

## 负载指标

有两个时间指标可用：

- 线程、协程调度延迟
  - Default 线程：SubmitHandleTask 的时间，到 task 被执行的时间，之前的时间差；
  - Fiber 协程：StartFiberDetached 的时间，到 task 被执行的时间，之前的时间差；
- 处理一个请求的耗时
  - 类似于 Filter 从 PRE 到 POST 的时间

由于线程、协程调度延迟不依赖具体服务，只和 CPU 有关，因此更适合作为一个默认的指标。

在 tRPC-Cpp 框架下，这个指标也有两个缺陷：

- 在 `merge` 模式下，直接在读 IO 的线程处理消息，不存在跨线程调度，导致 `merge` 模式下根本没有这个指标；
- 在 `seperate` 模式下，IO 和 Handle 线程独立，由于监控的是提交 Handle 任务的延迟，只有 Handle 忙的情况才能监控到这个指标的变化，对于 IO 忙的情况则监控不到。

在 tRPC-Go 中，$M_{expect}$ 默认设置为 3ms，在 tRPC-Cpp 中，由于协程调度等实现与 goroutine 存在不同，需要测试总结。初步测试下，发现 3ms 可能过于严苛，导致误限，10ms 可能是一个相对合理的配置，handle 任务调度延迟达到 10ms 时，通常表示请求时常通常比空载时多了 20ms（测试表明），值得留意的是，此时 CPU 大概率还并没有满载，如果能承受相对较高的请求时长，也应当可以继续调大这个值，来允许更高的 qps，因此这个值作为配置文件来提供。根据经验该值设置为 20ms 比较合适。

## 优先级算法

系统维护两个优先级阈值：

- lower （下阈值，初始值 0）
- upper（上阈值，初始值是优先级最大值 + 1，在 tRPC 中一般为 255 + 1）

对于一个请求，根据其优先级，被分为三类：

- Must （优先级 >= upper）
- May （ lower <= 优先级 < upper ）
- No （优先级 < lower）

其中，No 类请求，将直接拦截；Must 类请求使用相对宽松的策略，获取请求许可（例如， $concurrency_{curr} < 2 * concurrency_{max}$ ，则允许此请求通过），May 类请求使用一般的策略，获取请求许可（例如， $concurrency_{curr} < concurrency_{max}$，则允许此请求通过），对于 May 类型的请求，根据请求许可结果结果再细分为以下两类：

- May_OK （请求许可成功，后续继续执行请求）
- May_FAIL （请求许可失败，拦截）

在每个窗口周期（例如，200 个请求算作一个窗口周期），统计出 4 类请求的总数量，并依此小幅度调整 lower、upper 阈值，在多个窗口周期后，最终使四种请求的比例有如下特征：

1. $ N_{May\_OK} / N_{Must} $ ~ 0.1 （或某个较小的比例）
2. $ N_{May\_OK} / N_{May} $ ~ 0.5

tRPC 协议中携带的优先级是 0-255 之间的整数，可以为其加上一个 \[0, 1\) 间的随机小数，进行模糊化，使每个请求优先级分布连续，也允许在所有请求优先级相同时，等价于随机拒绝请求。

## 如何计算过去几个周期的最值

算法需要得到过去一段时间的最值，包括 $max(qps_{success})$ 和 $min(cost_{avg})$ 。这里采用 EMA 算法的变种：

```math
s_t = \begin{cases}
x_t \times \alpha + s_{t-1} \times (1-\alpha), & \text{if } x_t > s_{t-1} \\
x_t \times \beta + s_{t-1} \times (1-\beta), & \text{if } x_t \le s_{t-1}
\end{cases}
```

其中，$\alpha \ll 1$，$\beta \ll 1$。

- 对于 $\text{min}(cost_\text{avg})$，选 $\alpha \ll \beta$，如，$\alpha=0.01$，$\beta=0.1$。
- 对于 $\text{max}(qps_\text{success})$，选 $\alpha \gg \beta$，如，$\alpha=0.1$，$\beta=0.01$。

## 如何计算高百分位值

对于监控指标 $M$，每个成功的请求会提供一个值，而我们需要这个指标载过去一段时间内的高百分位值（例如，p99百分位值）来作为 $M_{measured}$。完整的 p99 实现较为复杂，而过载保护的场景并不需要这个统计数据特别精确。

这里使用一个相对简单的算法，每经过 10 个请求，记这个请求编号为 n，则记录 [n-30, n] 这些编号请求的最大值，经过 k 轮请求后拥有 $k/10$ 个最大值，将这些最大值取平均值，再与上一轮的到的历史  $M_{measured}$ 值进行 EMA 求和：

```math
s_t = 0.9 * s_{t-1} + 0.1 * avg(v_{max})
```
