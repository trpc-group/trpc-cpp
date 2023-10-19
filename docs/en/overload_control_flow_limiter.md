[中文](../zh/overload_control_flow_limiter.md)

# Overview

The capacity of a single node to handle requests is limited. In order to ensure high availability of the service, we must impose some restrictions on the number of requests. The need for flow control arises in this context. The current tRPC-Cpp framework supports flow control configuration at two levels: service and function. The measurement dimension is QPS.

- Two flow control algorithms are used:
  - Fixed window (counting method), corresponding to the flow control controller: `seconds` (fixed window), also known as `default`.
  - Sliding window, corresponding to the flow control controller: `smooth` (sliding window).
- Throttling trigger conditions

  First, the service flow limit is checked. If the service flow limit is exceeded, a flow control error is returned. Then, the interface flow limit is checked. If it exceeds the limit, a flow control error is returned.

*Note: The algorithm principles behind flow control are quite complex. A dedicated section will be provided to explain them in detail.*

# Principle

## Principle diagram of overload protection based on flow control

![flow_control_limiter](../images/flow_control_limiter.png)
The core point in the diagram is the `FlowControlLimiter`, which is primarily responsible for calculating the current QPS (requests per second) using traffic control algorithms. It compares the number of requests (`reqs_counter`) within a 1-second time window to the maximum number of requests configured in the window (`win_limit`) to determine whether to reject the new incoming request (`curr_req`). The logic is straightforward.

## Implementation code

The overload protection implementation of the framework is based on [filters](./filter.md), in order to ensure flow control protection as early as possible. This filter is placed before the request is enqueued, and the specific implementation of the filter placement is as follows:

```cpp
std::vector<FilterPoint> FlowControlServerFilter::GetFilterPoint() {
  return {
      FilterPoint::SERVER_PRE_SCHED_RECV_MSG,
      // ...
  };
}
```

 Source code: [flow_control_server_filter](../../trpc/overload_control/flow_control/flow_controller_server_filter.cc)

# Usage example

The overload protection filter based on flow control can currently **only be applied to the server-side, and client-side support is not available at the moment**. Users can use it without modifying any code by simply enabling compilation and adding configurations, making it very convenient.

## Compilation options

Add the following line to the `.bazelrc` file.

```sh
build --define trpc_include_overload_control=true
```

## Configuration file

The server-side flow control configuration is as follows (for detailed configuration, refer to [flow_test.yaml](../../trpc/overload_control/flow_control/flow_test.yaml)):

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
        - flow_control

#Plugin configuration.
plugins:
#  metrics:
#    prometheus:
      # ...
  overload_control:
    flow_control:
      - service_name: trpc.test.helloworld.Greeter #service name.
        is_report: true # Whether to report monitoring data.
        service_limiter: default(100000) # Service-level flow control limiter, standard format: name (maximum limit per second), empty for no limit.
        func_limiter: #Interface-level flow control.
          - name: SayHello ##Method name
            limiter: seconds(50000) #Interface-level flow control limiter, standard format: name (maximum limit per second), empty for no limit.
          - name: Route #Method name
            limiter: smooth(80000) #Interface-level flow control limiter, standard format: name (maximum limit per second), empty for no limit.
```

The key points of the configuration are as follows:

- flow_control: The name of the flow control overload protector
- service_name: The name of the service associated with flow control.
- service_limiter: Specifies the service-level flow control algorithm for the service name `service_name`. Here, the configuration `default(100000)` is explained as follows:
  - `default` (equivalent to `seconds`): Indicates that the fixed window flow control algorithm is selected.
  - `100000`: Represents the maximum number of requests per second, i.e., the maximum QPS is 100000.
- func_limiter: Specifies the interface-level flow control strategy for the service name `service_name`. Each service can have multiple interfaces, and each interface can choose a different flow control algorithm. The details are as follows:
  - `name`: The name of the interface, such as `SayHello` and `Route`.
  - `limiter`: Specifies the flow control algorithm for the interface. For example, `Route` is configured with the sliding window (smooth) algorithm, with a maximum QPS of 80000.
- is_report: Specifies whether to report monitoring data to the monitoring plugin. **Note that this configuration must be used together with a monitoring plugin (e.g., configuration: plugins->metrics->prometheus), otherwise, this option is meaningless**. The monitoring data for different flow control algorithms is as follows:
  - seconds：
    - `SecondsLimiter`: The name of the flow control algorithm used to check if the configuration matches the one executed in the program.
    - `/{callee_name}/{method}`: The format of the monitoring name, composed of the callee service (callee_name) and the method name (method). For example: `/trpc.test.helloworld.Greeter/SayHello`.
    - `current_qps`: The current concurrent QPS value.
    - `max_qps`: The configured maximum QPS, used to check if the maximum QPS in the configuration matches the one executed in the program.
    - `window_size`: The number of sampling windows. Users may not need to pay attention to this.
    - `Pass`: The pass status of a single request, where 0 means intercepted and 1 means passed.
    - `Limited`: The interception status of a single request, where 1 means intercepted and 0 means passed. This is the opposite of the `Pass` monitoring attribute mentioned above.
  - smooth：
    - `SmoothLimiter`: The name of the flow control algorithm used to check if the configuration matches the one executed in the program.
    - `/{callee_name}/{method}`: The format of the monitoring name, composed of the callee service (callee_name) and the method name (method). For example: `/trpc.test.helloworld.Greeter/SayHello`.
    - `active_sum`: The sum of all requests in the hit time slots, representing the QPS value at the moment after excluding the current request. If it exceeds the maximum QPS, the `hit_num` below will be 0.
    - `hit_num`: The QPS value after adding one request to the hit time slot.
    - `max_qps`: The configured maximum QPS, used to check if the maximum QPS in the configuration matches the one executed in the program.
    - `window_size`: The number of sampling windows. Users may not need to pay attention to this.
    - `Pass`: The pass status of a single request, where 0 means intercepted and 1 means passed.
    - `Limited`: The interception status of a single request, where 1 means intercepted and 0 means passed. This is the opposite of the `Pass` monitoring attribute mentioned above.

## Implementing flow control using code

In addition to configuring flow control, flow control functionality can also be implemented directly through code (note the difference between service-level and interface-level flow control code). The registration methods for service-level and interface-level flow controllers for business users are as follows:

```cpp
class HelloWorldServer : public ::trpc::TrpcApp {
 public:
  // ...
  int RegisterPlugins() { 
    
    // ...

    // Registering a service-level flow controller.
    trpc::FlowControllerPtr service_controller(new trpc::SmoothLimter(200000, true, 100));
    trpc::FlowControllerFactory::GetInstance()->Register( "trpc.test.helloworld.Greeter", service_controller); 

    // Registering an interface-level flow controller.
    trpc::FlowControllerPtr say_hello_controller(new trpc::SecondsLimiter(10000, true, 100)); 
    trpc::FlowControllerFactory::GetInstance()->Register( "/trpc.test.helloworld.Greeter/SayHello", say_hello_controller); 
    return 0;
  }

};
```

The registration logic for the flow controller must be executed in the `RegisterPlugins` function within the `trpc::TrpcApp` interface.

- For detailed information about the `trpc::SmoothLimiter` class, refer to: [SmoothLimiter](../../trpc/overload_control/flow_control/smooth_limiter.h).
- For detailed information about the `trpc::SecondsLimiter` class, refer to: [SecondsLimiter](../../trpc/overload_control/flow_control/seconds_limiter.h).

# Principles of flow control algorithms

Below is a brief explanation of the implementation algorithms for fixed window traffic control and sliding window traffic control.

## Fixed window traffic control algorithm

The fixed window algorithm, also known as the fixed window counter algorithm, divides time into fixed-sized windows along the timeline. It limits the maximum number of requests allowed within each time window to a predefined threshold. At any given moment, the system operates within a specific time window. When a new request arrives within the current time window, the counter for that window is atomically incremented. Additionally, each new request checks the total number of requests allowed within the current time window against the threshold. If the counter exceeds the threshold, the remaining requests are rejected. It is important to note that as time progresses and the time window shifts, the counter within each window is reset to 0.

![flow_control_limiter_second_1](../images/flow_control_limiter_second_1.png)

The fixed window algorithm is the simplest traffic control algorithm in terms of principles and implementation. It typically involves setting a global counter to track the number of requests allowed within the current time window and making decisions for incoming requests. Additionally, the counter is reset whenever the time reaches the start of a new time window. The specific steps are as follows:

- Divide time into multiple windows.
- Increment the counter for each request within the window.
- If the counter exceeds the limit, reject the remaining requests within the current time window. Reset the counter when the time reaches the next window. ![flow_control_limiter_second_2](../images/flow_control_limiter_second_2.png) However, the fixed window algorithm also has a **boundary problem** due to the fixed position of the windows. This means that there can be situations where the actual number of requests passing through exceeds the limit at the boundary positions of the windows. While the number of requests allowed within each individual time window is below the threshold, the combination of two windows at the boundary position may result in a window size that still satisfies the 1-second setting, but allows twice the number of requests as the threshold.

## Sliding window traffic control algorithm

To address the limitations of the fixed window algorithm described above, researchers have developed the sliding window counter algorithm. Instead of using fixed windows, this algorithm uses **sliding** windows that move with time. This ensures that within each sliding window, the condition `counter <= limit` is always satisfied. The sliding windows overlap with each other due to the sliding algorithm, eliminating the occurrence of boundary effects. The specific steps of the sliding window algorithm are as follows:

- Divide time into multiple intervals.
- Within each time interval, divide time into smaller slices. The sliding window moves one slice at a time according to the time rules.
- Increment the counter for each request within each window.
- Check the total count within the latest sliding window against the threshold to decide whether to accept or reject the service. Services that exceed the threshold are rejected.
- When the sliding window moves, the counts from the slices that move out of the sliding window are discarded, and the counts within the new slices are reset to zero.
![flow_control_limiter_smooth](../images/flow_control_limiter_smooth.png) The sliding window algorithm is an upgraded version of the fixed window algorithm. By subdividing the windows and **sliding** them according to time, it improves accuracy and eliminates **boundary effects**. However, this improvement comes at the cost of increased computational overhead.
