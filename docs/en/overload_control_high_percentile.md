[中文](../zh/overload_control_high_percentile.md)

# Overview

Overload protection algorithms can generally be divided into two categories:

- Non-adaptive overload protection: This refers to control strategies where the output does not affect the system's input. These strategies are pre-configured with control factors/parameters related to the control, or in other words, they are control strategies without feedback.
- Adaptive overload protection: This refers to systems where the output affects the system's input. Typically, these are negative feedback systems.

The tRPC-Cpp framework already supports non-adaptive overload protection, including flow control rate limiting, concurrent request limiting, and concurrent Fiber limiting. They share a common drawback: **it is difficult to determine a universal value. Even for the same module, configuration adjustments may be required at any time due to changes in business logic, making maintenance inconvenient**. To address this issue, the framework provides an adaptive overload protection strategy, which is the main focus of this article: **the adaptive overload protection strategy based on concurrency**.

*Note: The maximum concurrency based on concurrent request rate limiting is a fixed value set in the configuration. On the other hand, the maximum concurrency in the adaptive overload protection strategy based on concurrency is dynamically changing. Both are related to concurrent requests, but there is a significant difference between them.*

# Adaptive overload protection strategy based on concurrency

The adaptive overload protection strategy based on concurrency utilizes a complex algorithm, which will be explained in detail in the subsequent sections of this article. In simple terms, it reflects the current system load based on QPS (Queries Per Second). It has the following characteristics:

- Use Cases

  **`High QPS leading to high system load, i.e., IO-intensive scenarios are suitable for this strategy`**; However, **low QPS and high CPU-intensive scenarios are not well-suited for this strategy.**

- Support for Priority

  This overload protection strategy supports **parsing requests with priority** and discarding low-priority requests when overload occurs. By default, the priority is carried in the request through the context `transinfo` and is propagated along the chain. In `transinfo`, the priority is represented using the key `trpc-priority`, and the value is a single-byte uint8 length. The allowed range for request priority is `0-255`. If the key does not exist or the value length is not one byte, the request is considered to have a `default priority of 0`.
  The plugin also supports **custom priority protocols**, referring to: [SetServerGetPriorityFunc](../../trpc/overload_control/common/request_priority.cc).
  When no requests carry priority, the overload protection can still function and **randomly discard a portion of requests** when overload occurs.

- Support for dry-run mode

  Due to the **possibility of being too sensitive (false positive) or not sensitive enough (false negative) for different service types**, it is recommended to **enable dry-run mode and enable reporting for sufficient observation in the early stages** (refer to the configuration section in this article). Once the reported data from the production environment and the performance observed during offline load testing meet expectations, the dry-run mode can be disabled and the protection can be enabled.

- Limitation

  The server-side plugin for overload protection supports both the **separate and fiber thread models**. The `merge thread model`, due to its own characteristics, `does not enqueue requests`, so **it is not supported by default configuration**. It only supports metrics based on the duration of the business function, which is configured using the `max_request_latency` field. The corresponding configuration field `max_schedule_delay` should be set to 0 (disabled).

## Diagram of Adaptive Overload Protection Based on Concurrency

![high_percenttile_limiter](../images/high_percenttile_limiter.png)
The core component in the diagram is the `HighPercentileServerFilter`, which plays a crucial role in comparing the current concurrency requests (`concurrency_reqs`) with the dynamically calculated maximum concurrency (`max_concurrency`) to determine whether to reject the incoming request (curr_req). The `max_concurrency` is calculated based on the principles of Little's Law.

## Implementation code

  The overload protection implementation in the framework is based on [filters](./filter.md), which are used to ensure early flow control protection. To achieve this, the filter is placed before the request is enqueued. The specific implementation of the filter placement is as follows:

  ```cpp
  std::vector<FilterPoint> HighPercentileServerFilter::GetFilterPoint() {
    return {
        FilterPoint::SERVER_PRE_SCHED_RECV_MSG,
        // ...
    };
  }
  ```

  Source code reference: [high_percentile_server_filter](../../trpc/overload_control/high_percentile/high_percentile_server_filter.cc)

# Usage example

Adaptive overload protection filter based on concurrency, currently only **applicable to the server-side, client-side is not supported at the moment**. Users can utilize this filter without modifying any code; they only need to enable compilation and add the necessary configuration, making it extremely convenient.

## Compilation options

Add the following line to the `.bazelrc` file.

```sh
build --define trpc_include_overload_control=true
```

## Configuration file

The server-side configuration is as follows (for detailed configuration, refer to [high_percentile.yaml](../../trpc/overload_control/high_percentile/high_percentile.yaml)):

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

The key points of the configuration are as follows:

- high_percentile: The name of the flow control overload protector
- max_schedule_delay: The desired maximum thread scheduling delay (in ms) represents the expected maximum waiting time for requests in the queue. The algorithm's results are sensitive to changes in this parameter. This option is used in the `separate` and `fiber` modes. This value is only meaningful if it is greater than 0.
- max_request_latency: The desired maximum business processing latency, also known as downstream request latency (in ms). This parameter represents the time taken for business processing and may not necessarily indicate the current system load level. It is used to address the issue of `max_schedule_delay` becoming ineffective in the `merge mode` when there are no enqueue operations for requests. This option is only suitable for IO-intensive scenarios. This value is only meaningful if it is greater than 0.
- dry_run: When enabled, it simulates the execution of protection strategies, performs reporting, but does not actually intercept requests.
- is_report: Specifies whether to report monitoring data to the monitoring plugin. **Note that this configuration must be used together with a monitoring plugin (e.g., configuration: plugins->metrics->prometheus), otherwise, this option is meaningless**. The monitoring data for different flow control algorithms is as follows:
  - `Pass`: The pass status of an individual request. 0 indicates interception, and 1 indicates passing. For each request, only one of the monitoring results for `Pass`, `Limited`, and `LimitedByLower` will be 1, while the other two will be 0.
  - `Limited`: The interception status of an individual request due to overload. 1 indicates interception, and 0 indicates passing.
  - `LimitedByLower`: The interception status of an individual request due to priority. 1 indicates interception, and 0 indicates passing.
  - `/{callee_name}/{method}`: The format of the monitoring name, composed of the callee service (callee_name) and the method name (method). For example: `/trpc.test.helloworld.Greeter/SayHello`.
  - Parameters related to monitoring algorithm execution results:
    - `/{callee_name}/{method}/{strategy}/expected`: Reports the configured value of `max_schedule_delay` or `max_request_latency` (represented by expected). For example: `/trpc.test.helloworld.Greeter/SayHello/max_schedule_delay/expected` represents the value when `max_schedule_delay` is configured to be greater than 0. This is used to verify if the configuration parameter is effective.
    - `/{callee_name}/{method}/{strategy}/measured`: Reports the high percentile value of request processing time calculated by the algorithm (represented by `measured`). This corresponds to the $M_{expect}$ value mentioned in the subsequent algorithm description. For example: `/trpc.test.helloworld.Greeter/SayHello/max_schedule_delay/measured` represents the high percentile value of request processing time when `max_schedule_delay` is configured to be greater than 0.
    - `/{callee_name}/{method}/{strategy}/quotient`: Reports the value of `expected/measured`(represented by `quotient`)，For example:`/trpc.test.helloworld.Greeter/SayHello/max_schedule_delay/quotient` represents the ratio of `expected/measured` when `max_schedule_delay` is configured to be greater than 0.
    - `/{callee_name}/{method}/max_measured`: Reports the maximum `measured` value (represented by `max_measured` ). For example:  `/trpc.test.helloworld.Greeter/SayHello/max_measured`。
    - `/{callee_name}/{method}/max_concurrency`: Reports the maximum concurrency allowed by the current system (represented by `max_concurrency`). For example: `/trpc.test.helloworld.Greeter/SayHello/max_concurrency`。
    - `/{callee_name}/{method}/cur_concurrency`: Reports the current concurrency of the system (represented by `cur_concurrency`). For example: `/trpc.test.helloworld.Greeter/SayHello/cur_concurrency`。
    - `/{callee_name}/{method}/lower`: Reports the lower threshold of request priority calculated by the current system's algorithm. For example: `/trpc.test.helloworld.Greeter/SayHello/lower`。
    - `/{callee_name}/{method}/upper`: Reports the upper threshold of request priority calculated by the current system's algorithm. For example: `/trpc.test.helloworld.Greeter/SayHello/upper`。
    - `/{callee_name}/{method}/must`: Reports the number of requests with a priority calculated by the algorithm that is greater than `upper`. For example: `/trpc.test.helloworld.Greeter/SayHello/must`。
    - `/{callee_name}/{method}/may`: Reports the number of requests with a priority calculated by the algorithm that falls within the `[lower ,upper)` range. For example:`/trpc.test.helloworld.Greeter/SayHello/may`。
    - `/{callee_name}/{method}/ok`: Reports the number of requests that pass when their priority calculated by the algorithm falls within the `[lower ,upper)` range. For example: `/trpc.test.helloworld.Greeter/SayHello/ok`。
    - `/{callee_name}/{method}/no`: Reports the number of requests with a priority calculated by the algorithm that is less than `lower`. For example: `/trpc.test.helloworld.Greeter/SayHello/no`。

# Algorithm Basic Principles

## Concurrency Control Algorithm

The current concurrency level of the system, $concurrency_{curr}$ , is relatively easy to obtain (using an atomic variable, incrementing by 1 when a request starts and decrementing by 1 when a request ends). If we can estimate the maximum concurrency level of the system, $concurrency_{max}$，we can compare  $concurrency_{curr}$ with $concurrency_{max}$, before each request to determine whether the request should be allowed to proceed. This helps protect the system from excessive concurrency and ensures that the concurrency level remains within acceptable limits.

Concurrency control algorithms are based on [Little's Law](https://en.wikipedia.org/wiki/Little%27s_law)：

```math
concurrency = cost_\text{avg} \times qps \quad
```

According to the above formula, within a window of time, we have:

```math
concurrency_\text{max} = \text{min}(cost_\text{avg}) \times \text{max}(qps_\text{success}) \quad
```

That is, when the service is operating at full capacity (approaching overload or experiencing slight overload), we can estimate the maximum concurrency level that the system can support by monitoring the minimum average request cost, $min(cost_{avg})$ , and the maximum number of successful requests per second, $max(qps_{success})$, over a short period of time.

The above formula is only effective when the service is operating at full capacity. However, we would like to achieve the following:

1. When the service is lightly loaded, we want to allow requests to pass through without any interception.
2. When the service is moderately loaded, we want to approximate the maximum concurrency level by multiplying the result of the above formula (where $max(qps_{success})$ has not reached its maximum) by a scaling factor greater than 1.
3. When the service is operating at full capacity, we want to estimate the maximum concurrency level using the above formula.
4. When the service is overloaded, we want to approximate the maximum concurrency level by multiplying the result of the above formula (where $min(cost_{avg})$ is inflated due to overload) by a scaling factor less than 1.

We can select a monitoring metric, denoted as $M$ (typically the `thread/coroutine` scheduling delay, as described in the next section "Load Metrics"). This metric has an expected value, $M_{expect}$ (e.g., 10ms). The system periodically monitors the actual value of this metric, $M_{measured}$，When $M_{expect}$ is close to $M_{measured}$ , it indicates that the system is approaching full capacity. Additionally, when $M_{expect}$ and $M_{measured}$ differ significantly, their ratio can represent the system's load level. By using an algorithm, we can calculate a correction factor, $F(M_{measure}, M_{expect})$, to estimate the correct maximum concurrency level of the system (corresponding to cases 2 and 4 mentioned earlier). The formula is as follows:

```math
concurrency_\text{max} = \mathcal{F}(\mathcal{M}_\text{measure}, \mathcal{M}_\text{expect}) \times \text{min}(cost_\text{avg}) \times \text{max}(qps_\text{success}) \quad
```

If $F(M_{measure}, M_{expect})$ performs well, it means that we can estimate the maximum concurrency level, $concurrency_{max}$ of the system under any load condition.

Based on experimental results, $F(M_{measure}, M_{expect})$ can take the following form:

```math
\begin{equation}
\mathcal{F(M_\text{measure}, M_{expect} )} = \begin{cases}
\infty & \text{if } \mathcal{M}_{measure} < \alpha \times \mathcal{M}_{expect} \\
\mathcal{M}_\text{expect} / \mathcal{M}_\text{measure} & \text{if } \alpha \times \mathcal{M}_{expect} < \mathcal{M}_{measure} < \mathcal{M}_{expect} \\
\sqrt{\frac{\mathcal{M}_{expect}}{\mathcal{M}_{measure}}} & \text{if } \mathcal{M}_{measure} \ge \mathcal{M}_{expect}
\end{cases}
\end{equation}
```

In the first case, when the system is lightly loaded, we set $concurrency_{max} = \infty$ to infinity, indicating that no interception protection is needed.

In this case, $\alpha$ is typically set to 0.5, meaning:

- $ M_{measure} < 0.5 * M_{expect} $ indicates that the system is lightly loaded.
- $ 0.5 * M_{expect} < M_{measure} < M_{expect} $ indicates that the system is moderately loaded.
- $ M_{measure} > M_{expect} $ indicates that the system is overloaded.

## Load Metrics

There are two time metrics available:

- Thread/Coroutine Scheduling Delay:
  - For default threads: The time difference between when `SubmitHandleTask` is called and when the task is executed.
  - For Fiber coroutines: The time difference between when `StartFiberDetached` is called and when the task is executed.
- Request Processing Time:
  - Similar to the time taken for a filter to process a request from the PRE to POST phase.

Since the thread/coroutine scheduling delay is independent of the specific service and only depends on the CPU, it is more suitable as a default metric.

In the tRPC-Cpp framework, there are two limitations to consider:

- In `merge` mode, messages are processed directly on the IO thread without any cross-thread scheduling, so this metric is not available in `merge` mode
- In `seperate` mode, the IO and Handle threads are separate. Monitoring the delay of submitting Handle tasks can only capture changes in this metric when the Handle thread is busy. It cannot capture changes when the IO thread is busy.

In tRPC-Go, $M_{expect}$ is set to a default value of 3ms. In tRPC-Cpp, due to differences in coroutine scheduling and implementation compared to goroutines, further testing and evaluation are needed to determine an appropriate value. Initial tests suggest that 3ms may be too strict and can lead to false limitations. A value of 10ms may be a more reasonable configuration. When the scheduling delay of handle tasks reaches 10ms, it typically indicates that the request duration is about 20ms longer than in an unloaded scenario (based on test results). It is worth noting that the CPU is likely not fully utilized at this point. If the system can tolerate higher request durations, this value can be increased to allow for higher QPS. Therefore, this value should be provided as a configurable parameter in the configuration file. Based on experience, a value of 20ms is generally suitable.

## Priority Algorithm

The system maintains two priority thresholds:

- lower Threshold (initial value: 0)
- upper Threshold (initial value: maximum priority value + 1, typically 255 + 1 in tRPC)

For a request, it is classified into one of three categories based on its priority:

- Must (priority >= upper)
- May (lower <= priority < upper)
- No (priority < lower)

For No category requests, they are directly intercepted. Must category requests use a relatively lenient strategy to obtain request permission (e.g., if $concurrency_{curr} < 2 * concurrency_{max}$ , the request is allowed to proceed). May category requests use a general strategy to obtain request permission (e.g., if $concurrency_{curr} < concurrency_{max}$, the request is allowed to proceed). For May category requests, based on the result of request permission, they are further divided into two sub-categories:

- May_OK (request permission granted, continue with the request execution)
- May_FAIL (request permission denied, intercepted)

In each window period (e.g., 200 requests considered as one window period), the total number of requests in each of the four categories is counted. Based on these counts, the lower and upper thresholds are slightly adjusted. After multiple window periods, the goal is to achieve the following characteristics in the ratio of the four types of requests:

1. $ N_{May\_OK} / N_{Must} $ ~ 0.1 (or a small ratio)
2. $ N_{May\_OK} / N_{May} $ ~ 0.5

In the tRPC protocol, the priority carried is an integer between 0 and 255. To make the distribution of request priorities continuous, a random decimal number between [0, 1) can be added to the priority, allowing for fuzziness. This also allows for equivalent random rejection of requests when all request priorities are the same.

## How to calculate the maximum or minimum value over the past few periods

The algorithm needs to obtain the maximum value over a certain period of time, including $max(qps_{success})$ and the minimum value of average cost ($min(cost_{avg})$) Here, a variant of the EMA (Exponential Moving Average) algorithm is used.

```math
s_t = \begin{cases}
x_t \times \alpha + s_{t-1} \times (1-\alpha), & \text{if } x_t > s_{t-1} \\
x_t \times \beta + s_{t-1} \times (1-\beta), & \text{if } x_t \le s_{t-1}
\end{cases}
```

Where $\alpha \ll 1$，$\beta \ll 1$。

- For $\text{min}(cost_\text{avg})$，choose $\alpha \ll \beta$，for $\alpha=0.01$，$\beta=0.1$。
- For $\text{max}(qps_\text{success})$，choose $\alpha \gg \beta$，for $\alpha=0.1$，$\beta=0.01$。

## How to calculate high percentile values

For the monitoring metric $M$, each successful request provides a value, and we need to calculate the high percentile value of this metric over a certain period of time (e.g., p99 percentile value) as $M_{measured}$​
 . Implementing the complete p99 calculation can be complex, but in overload protection scenarios, we don't need this statistical data to be extremely accurate.

Here, we can use a relatively simple algorithm. After every 10 requests, we record the maximum value among the requests with the corresponding request numbers [n-30, n]. After k rounds of requests, we will have k/10 maximum values. We take the average of these maximum values and then perform an EMA (Exponential Moving Average) summation with the previously obtained historical $M_{measured}$ value:

```math
s_t = 0.9 * s_{t-1} + 0.1 * avg(v_{max})
```
