[中文](../zh/overload_control_concurrency_limiter.md)

# Overview

The tRPC-Cpp framework is applied in high-concurrency scenarios and requires overload protection to prevent unexpected errors in business programs. Although the framework has implemented flow control plugins based on services and methods for rate limiting, these plugins run after the requests are enqueued and cannot provide early overload protection. This article mainly introduces an overload protection plugin based on concurrent request volume, which runs before the requests are enqueued and implements a simple and understandable strategy.

# Principle

## Principle diagram of overload protection based on concurrent requests

The core point in the diagram is the `ConcurrencyLimiter`, which is responsible for comparing the current concurrent request count (`concurrency_reqs`) with the maximum concurrency configured by the user (`max_concurrency`) to determine whether to reject the incoming new request (`curr_req`). The logic is straightforward.

## Implementation code

  The overload protection implementation of the framework is based on [filters](./filter.md), in order to ensure flow control protection as early as possible. This filter is placed before the request is enqueued, and the specific implementation of the filter placement is as follows:

  ```cpp
  std::vector<FilterPoint> ConcurrencyLimiterServerFilter::GetFilterPoint() {
    return {
        FilterPoint::SERVER_PRE_SCHED_RECV_MSG,
        // ...
    };
  }
  ```

  Source code：[concurrency_limiter_server_filter](../../trpc/overload_control/concurrency_limiter/concurrency_limiter_server_filter.cc)

# Usage example

The overload protection filter based on concurrent request volume can currently **only be applied to the server-side, and is not supported for the client-side at the moment**. Users can use it without modifying any code, just by enabling compilation and adding configuration, which is very convenient.

## Compilation options

Add the following line to the `.bazelrc` file.

```sh
build --define trpc_include_overload_control=true
```

## Configuration file

Concurrent request filter configuration. For detailed configuration reference, please refer to the [concurrency_overload_ctrl.yaml](../../trpc/overload_control/concurrency_limiter/concurrency_overload_ctrl.yaml)

```yaml
#Server configuration
server:
  app: test #Business name, such as: COS, CDB.
  server: helloworld #Module name of the business
  admin_port: 21111 # Admin port
  admin_ip: 0.0.0.0 # Admin ip
  service: #Business service, can have multiple.
    - name: trpc.test.helloworld.Greeter #Service name, needs to be filled in according to the format, the first field is default to trpc, the second and third fields are the app and server configurations above, and the fourth field is the user-defined service_name.
      network: tcp #Network listening type: for example: TCP, UDP.
      ip: 0.0.0.0 #Listen ip
      port: 10001 #Listen port
      protocol: trpc #Service application layer protocol, for example: trpc, http.
      accept_thread_num: 1 #Number of threads for binding ports.
      filter:
        - concurrency_limiter

#Plugin configuration.
plugins:
#  metrics:
#    prometheus:
      # ...
  overload_control:
    concurrency_limiter:
      max_concurrency: 10 # Maximum concurrency. It is configured small for unit testing purposes, but users can configure it to be larger.
      is_report: true # Whether to report
```

The key points of the configuration are as follows：

- concurrency_limiter: Name of the concurrent request overload protection filter
- max_concurrency: Maximum concurrent requests configured for the user. When the current concurrent requests are greater than or equal to this value, the requests will be intercepted.
- is_report: Whether to report monitoring data to the monitoring plugin. **Note that this configuration must be used together with a monitoring plugin (for example, configuring plugins->metrics->prometheus will report to Prometheus). If no monitoring plugin is configured, this option is meaningless**. The monitored data includes:
  `max_concurrency`: Reported maximum concurrent request number in the configuration, used to check if the configuration is consistent with the program's execution.
  `current_concurrency`: Current concurrent request number
  `/{callee_name}/{method}`: Monitoring name format, composed of the callee service (`callee_name`) and method name (`method`), such as: `/trpc.test.helloworld.Greeter/SayHello`.

# FAQ

## Q1: Why does the concurrent overload protector still not take effect even after being configured according to the settings？

According to the implementation principle of the concurrent overload protection filter, it compares the current concurrent requests with the maximum concurrent requests. The current concurrent request number is incremented or decremented based on the constructor and destructor of the server context, as shown below (refer to [server_context](../../trpc/server/server_context.cc)).

```cpp
ServerContext::ServerContext() { FrameStats::GetInstance()->GetServerStats().AddReqConcurrency(); }

ServerContext::~ServerContext() {
  // ...
  auto& server_stats = FrameStats::GetInstance()->GetServerStats();

  server_stats.SubReqConcurrency();
  // ...
}
```

Each request corresponds to a `ServerContext`, and `AddReqConcurrency` is executed in its constructor while `SubReqConcurrency` is executed in its destructor. Therefore, users should not store `ServerContext` during usage, as it would prevent the destructor from being executed. This would result in an increase in concurrent requests without any decrease, causing the overload protection to fail.
