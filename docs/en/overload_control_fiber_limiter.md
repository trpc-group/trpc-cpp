[中文](../zh/overload_control_fiber_limiter.md)

# Overview

In the tRPC-Cpp framework, under the `Fiber (M:N)` thread model (refer to the [Fiber User Guide](./fiber_user_guide.md)), when resource exhaustion occurs in high-concurrency scenarios, it can lead to program freezing. Therefore, it is necessary to impose limits on the number of Fibers to prevent program freezing. This article primarily introduces an overload protection plugin based on the number of Fibers in concurrency.

# Principle

## Principle diagram of overload protection based on the number of Fibers in concurrency

![fiber_limiter](../images/fiber_limiter.png)
The core point in the diagram is the `FiberLimiter`, which is primarily responsible for comparing the current number of concurrent Fibers (`curr_fiber_count`) in the framework with the maximum configured number of concurrent Fibers (`max_fiber_count`). Its main purpose is to determine whether to reject processing the current request (`curr_req`).

## Implementation code

The current `FiberLimiter` supports **both the client and the server**. Since the framework's overload protection is implemented based on filters (refer to [filter]((./filter.md))), in order to ensure flow control protection as early as possible, this filter is placed before the request is enqueued. The specific implementation of this filter is as follows:

```cpp
// Client side
std::vector<FilterPoint> FiberLimiterClientFilter::GetFilterPoint() {
  return {
      FilterPoint::CLIENT_PRE_SEND_MSG,
      ...
  };
}

// Server side
std::vector<FilterPoint> FiberLimiterServerFilter::GetFilterPoint() {
  return {
      FilterPoint::SERVER_PRE_SCHED_RECV_MSG,
      ...
  };
}
```

Source code reference: [fiber_limiter](../../trpc/overload_control/fiber_limiter/)

# Usage example

The overload protection filter based on the number of Fibers in concurrency supports both the client and the server. It must be used in the Fiber thread model. Users do not need to modify any code; they only need to modify the configuration, making it very convenient to use.

## Compilation options

Add the following line to the `.bazelrc` file.

```sh
build --define trpc_include_overload_control=true
```

## Configuration file

The configuration for the Fiber concurrency filter is as follows (for detailed configuration, refer to [fibers_overload_ctrl.yaml](../../trpc/overload_control/fiber_limiter/fibers_overload_ctrl.yaml))

```yaml
#Global configuration (required)
global:
  local_ip: 0.0.0.0 #Local IP, used for: not affecting the normal operation of the framework, used to obtain the local IP from the framework configuration.
  coroutine: #Coroutine configuration.
    enable: true #false: means not using coroutine; true: means using coroutine.
  threadmodel:
    fiber:
      - instance_name: fiber_instance
        concurrency_hint: 1
        scheduling_group_size: 1
        reactor_num_per_scheduling_group: 1

#Server configuration
server:
  app: test #Business name
  server: route #Module name of the business
  admin_port: 18888 # Admin port
  admin_ip: 0.0.0.0 #Admin ip
  service: #Business service, can have multiple.
    - name: trpc.test.route.Forward #Service name, needs to be filled in according to the format, the first field is default to trpc, the second and third fields are the app and server configurations above, and the fourth field is the user-defined service_name.
      network: tcp  #Network listening type: for example: TCP, UDP.
      ip: 0.0.0.0 #Listen ip
      port: 11112 #Listen port
      protocol: trpc #Service application layer protocol, for example: trpc, http.
      accept_thread_num: 1 #Number of threads for binding ports.
      filter:
        - fiber_limiter # Name of filter for this service

#Client configuration.
client:
  service:
    - name: trpc.test.helloworld.Greeter
      target: 127.0.0.1:32345
      protocol: trpc #Service application layer protocol, for example: trpc, http.
      network: tcp #Network listening type: for example: TCP, UDP.
      selector_name: direct #Name service used for route selection, "direct" for direct connection.
      filter:
        - fiber_limiter # Name of filter for this client

#Plugin configuration.
plugins: 
#  metrics:
#    prometheus:
      # ...
  overload_control:
    fiber_limiter:
      max_fiber_count: 20 # Maximum number of fibers. Here, for unit testing purposes, it is set relatively small. In normal business scenarios, this value should be much larger than 20.
      is_report: true # Whether to report overload information.
  # ...
```

The key points of the configuration are as follows：

- fiber_limiter: The name of the overload protection for Fiber concurrency.
- max_fiber_count: The maximum number of concurrent requests configured by the user. When the current concurrent requests are greater than or equal to this value, the requests will be intercepted.
- is_report: Whether to report monitoring data to the monitoring plugin. **Note that this configuration must be used together with a monitoring plugin (for example, configuring plugins->metrics->prometheus will report to Prometheus). If no monitoring plugin is configured, this option is meaningless**. The monitored data includes:
  - `max_fiber_count`: Reports the maximum Fiber concurrency configured by the user. It is a fixed value used to check if the configured maximum request concurrency is effective in the program.
  - `fibers_count`: Reports the current number of concurrent Fibers in the system.
  - `/{callee_name}/{method}`: The format for reporting RPC method monitoring name. It is a fixed value and consists of the callee service (callee_name) and the method name (method). For example: /trpc.test.helloworld.Greeter/SayHello. This format is **only effective for server-side rate limiting**.
  - `/{ip}/{port}`: The format for reporting monitoring items based on the IP and port of the callee service. For example: /127.0.0.1/22345. This format is **only effective for client-side rate limiting**. In the case of a client, when there are multiple nodes for the downstream service, it is not possible to differentiate the monitoring information for different nodes based on the callee service RPC. Therefore, the monitoring item is based on the combination of IP and port.
  - `Pass`：The pass status for an individual request: 0 means intercepted, and 1 means passed.
  - `Limited`：The interception status for an individual request: 1 means intercepted, and 0 means passed. It is the opposite of the `Pass` monitoring attribute mentioned above.

# FAQ

## Why is the Fiber overload protection filter not effective even after configuration?

Check if the Fiber thread model is being used.

## What are differences between overload protectors based on concurrent requests and overload protectors based on Fiber concurrency count?

In the tRPC-Cpp framework, the Fiber scheduling group is a global configuration. Therefore, in a relay scenario, whether acting as a client or a server, the same Fiber concurrency count can be obtained, allowing for rate limiting. However, the overload protector based on concurrent requests calculates the current concurrent requests based on the received requests. It is not reasonable to use this value when accessing backend nodes as a client. Therefore, the Fiber concurrency count overload protector supports both server-side and client-side, while the request concurrency overload protector is only applied to the server-side.

## Why is the concurrency still limited even when the number of concurrent requests is less than the maximum parallelism of Fiber?

The number of concurrent requests is not equivalent to the current parallelism of Fiber because some requests may be processed using multiple Fibers.
