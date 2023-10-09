[中文版](/docs/zh/filter.md)

[TOC]

# Customize filters

## Overview

In order to enhance the framework's extensibility, tRPC introduces the concept of filters, which takes inspiration from the Aspect-Oriented Programming (AOP) paradigm in Java. The approach involves setting up filter points before and after specific actions in the framework, and then inserting a series of filters at these points to extend the functionality of the framework, and it also facilitates integration with different ecosystems.

By leveraging the filter mechanism, the framework modularizes and plugins specific logic components related to interface requests, decoupling them from the concrete business logic and achieving reusability. Examples of filters include monitoring filters, logging filters, and authentication filters.

In terms of business logic, users can customize filters to implement functionalities such as service rate limiting and service monitoring. Therefore, it is essential to understand how to customize and use framework filters.

Next, let's first introduce the principles of filters and then explain how to implement and use custom filters.

## Principle

tRPC-Cpp filters are implemented using an array traversal approach. It defines a set of filter points and executes the corresponding filter logic at each step of the invocation process. To understand the principle of implementing filters in the framework, it is important to understand the following key parts:

- [Filter points and execution flow](#filter-points-and-execution-flow)
- [Filter types and execution order](#filter-types-and-execution-order)

### Filter points and execution flow

The framework's filter points are divided into server-side and client-side points, each with 10 filter points defined in [filter_point.h](/trpc/filter/filter_point.h). Typically, users only need to focus on a few filter points below. If you need to use other filter points, you can refer to the rpcz documentation.

```cpp
enum class FilterPoint {
  CLIENT_PRE_RPC_INVOKE = 0,   ///< Before makes client RPC call
  CLIENT_POST_RPC_INVOKE = 1,  ///< After makes client RPC call

  CLIENT_PRE_SEND_MSG = 2,   ///< Before client sends the RPC request message
  CLIENT_POST_RECV_MSG = 3,  ///< After client receives the RPC response message

  SERVER_POST_RECV_MSG = kServerFilterPrefix | 0,  ///< After server receives the RPC request message
  SERVER_PRE_SEND_MSG = kServerFilterPrefix | 1,   ///< Before server sends the RPC response message

  SERVER_PRE_RPC_INVOKE = kServerFilterPrefix | 2,   ///< Before makes server RPC call
  SERVER_POST_RPC_INVOKE = kServerFilterPrefix | 3,  ///< After makes server RPC call
};
```
The filter points and execution flow are as follows:

![img](/docs/images/filter_point.png)

**Description of client-side filter points**

| Filter point | Description |
| ------ | ------- |
| CLIENT_PRE_RPC_INVOKE | This filter point is executed immediately after the user invokes an RPC interface. At this point, the request content has not been serialized yet.|
| CLIENT_PRE_SEND_MSG | At this point, the request content has been serialized, and the next stage involves encoding and sending the request.|
| CLIENT_POST_RECV_MSG | At this point, the response packet has been received and decoded, but the response content has not been deserialized yet.|
| CLIENT_POST_RPC_INVOKE | At this point, the response content has been deserialized, indicating that the RPC invocation is completed.|

**Description of server-side filter points**

| Filter point | Description |
| ------ | ------- |
| SERVER_POST_RECV_MSG | At this point, the request packet has been received and decoded, but the response content has not been deserialized yet.|
| SERVER_PRE_RPC_INVOKE | At this point, the request content has been deserialized, and the next stage involves entering the RPC processing function.|
| SERVER_POST_RPC_INVOKE | At this point, the RPC processing logic has been completed. This filter point is executed immediately after the RPC processing logic finishes. At this time, the response content has not been serialized yet.|
| SERVER_PRE_SEND_MSG | At this point, the response content has been serialized, and the next stage involves encoding and sending the response.|

**Logic executed at filter points**

The framework calls the interceptor function of the `FilterController` class at filter points. The client-side invokes the `RunMessageClientFilters` function, while the server-side invokes the `RunMessageServerFilters` function. At this point, all filters registered with the framework will execute their implementation of the `operator()` function. The `operator()` function is a pure virtual function in the base class of the framework filter. Each specific filter needs to override this function after inheriting from the base class. Then, use the context in the interface parameters to retrieve the necessary information and implement specific functionality.

### Filter types and execution order

The framework supports two types of filters based on their scope:

1. Global-level filters
   
   These filters apply to all services under the server or client. They can be configured as follows:
   
   ```yaml
   server:
     filter:
       - sg1
       - sg2
   client:
     filter:
       - cg1
       - cg2
   ```
2. Service-level filters

   These filters only apply to the current service. They can be configured as follows:

    ```yaml
    server:
      service:
        - name: xxx
          filter:
            - ss1
            - ss2
    client:
      service:
        - name: xxx
          filter:
            - cs1
            - cs2
    ```

Whether a filter is a global-level filter or a service-level filter depends on its configuration. If a filter is configured as both a global and service filter, it will be treated as a global filter.

The configuration and execution order of filters follow the following principles:

- Filter points need to be paired

  To ensure that filters can gracefully exit in case of failure, we require filters at filter points to be paired. The pairs consist of even numbers (N, N+1).

- Filters are executed in the configured order

  In terms of execution order, global filters' pre-filter points are executed in the order specified in the configuration, followed by service filters' pre-filter points. The execution order of post-filter points is the reverse of the pre-filter points, following the "First In Last Out" principle.

- Filters support early exit:

  If a filter fails at the pre-filter point, it will interrupt the execution of the filter chain, and the subsequent process will only execute the post-filter points of the filters that have already been executed (excluding the failed filter).

For more details, please refer to the [filter design principles](/trpc/filter/README.md).

## How to implement and use custom filters

The overall steps are as follows: Implement the filter -> Register the filter to framework -> Use the filter.

### Implement the filter

- Server-side

  Inherit from the `MessageServerFilter` class, override the `GetFilterPoint` and `operator()` function to select the desired filter points and implement the logic of the filter.

  ```cpp
  #include "trpc/filter/server_filter_base.h"

  class CustomServerFilter : public ::trpc::MessageServerFilter {
   public:
     std::string Name() override { return "custom_server"; }

     std::vector<::trpc::FilterPoint> GetFilterPoint() override {
       std::vector<::trpc::FilterPoint> points = {::trpc::FilterPoint::SERVER_PRE_RPC_INVOKE,
                                                  ::trpc::FilterPoint::SERVER_POST_RPC_INVOKE};
       return points;
     }

     void operator()(::trpc::FilterStatus& status, ::trpc::FilterPoint point, 
                     const ::trpc::ServerContextPtr& context) {
       // do something
     }
  };
  ```

- Client-side

  Inherit from the `MessageClientFilter` class, override the `GetFilterPoint` and `operator()` function to select the desired filter points and implement the logic of the filter.

  ```cpp
  #include "trpc/filter/client_filter_base.h"

  class CustomClientFilter : public ::trpc::MessageClientFilter {
   public:
     std::string Name() override { return "custom_client"; }

     std::vector<::trpc::FilterPoint> GetFilterPoint() override {
       std::vector<::trpc::FilterPoint> points = {::trpc::FilterPoint::CLIENT_PRE_RPC_INVOKE,
                                                  ::trpc::FilterPoint::CLIENT_POST_RPC_INVOKE};
       return points;
     }

     void operator()(::trpc::FilterStatus& status, ::trpc::FilterPoint point, 
                     const ::trpc::ClientContextPtr& context) {
       // do something
     }
  };
  ```

### Register the filter to framework

According to the usage pattern of the framework, there are the following two scenarios:

1. Program as a server

   In this case, the program is implemented as a server by inherit `trpc::TrpcApp` of framework. To register filters in this scenario, you need to override the `trpc::TrpcApp::RegisterPlugins` interface and place the filter registration logic inside it.
   
   ```cpp
   #include "trpc/common/trpc_plugin.h"
   
   int xxxServer::RegisterPlugins() {
     // register server-side filter
     auto server_filter = std::make_shared<CustomServerFilter>();
     TrpcPlugin::GetInstance()->RegisterServerFilter(server_filter);

     //  register client-side filter
     auto client_filter = std::make_shared<CustomClientFilter>();
     TrpcPlugin::GetInstance()->RegisterClientFilter(client_filter);
   }
   ```

2. Program as a pure client
   
   In this case, the program only needs to use the client and no need to inherit `trpc::TrpcApp` of framework. To register filters in this scenario, you need to place the filter registration logic before `trpc::RunInTrpcRuntime` is called.

   ```cpp
   #include "trpc/common/trpc_plugin.h"

   int main() {
     ...

     auto client_filter = std::make_shared<CustomClientFilter>();
     TrpcPlugin::GetInstance()->RegisterClientFilter(client_filter);

     return ::trpc::RunInTrpcRuntime([]() {
       // do something
     });
   }
   ```

### Use the filter

Filter can be used by specifying them either through configuration files or code.

Using configuration files provides flexibility as adjustments can be made without modifying the code. The usage can be found in the section [Filter types and execution order](#filter-types-and-execution-order).

Alternatively, filters can be specified through code in the following scenarios:

1. When obtaining a `ServiceProxy` in the client proxy scenario using `GetProxy`, by specifying the desired service filters using the `service_filters` field in `ServiceProxyOption`.

2. When starting a service in the server service scenario using `StartService`, by specifying the service filters using the `service_filters` field in` ServiceConfig`.

Note: Global-level filters can only be specified through configuration files.

## Advanced Usage

### Configure independent filter for service

By default, a filter object is shared among multiple ServiceProxy instances or multiple services. This can make programming inconvenient in some scenarios and also impact performance. For example, in a scenario where rate limiting is based on the calling behavior of a service, using a shared filter would require storing the service names and their calling behavior in a map structure, along with the need for locking when accessing the map. Therefore, one solution is to allow each ServiceProxy or service to have its own independent filter object. This eliminates the need for a map structure and locking, simplifying programming and improving performance.

To achieve this, the framework provides a corresponding solution. Users can override the Create function of the Filter base class to make each service have its own independent filter object.

```cpp
virtual MessageClientFilterPtr Create(const std::any& param) { return nullptr; }
```

When the Create function returns a non-null result, it will be used as the filter object for the service. Otherwise, the shared filter will be used. Additionally, it is also possible to add independent configurations to the newly created filter in this scenario.

As an example of client proxy usage, the approach is as follows:

```cpp
MyFilterConfig config;
trpc::ServiceProxyOption option;
...
option.service_filter_configs["my_filter"] = config;  // "my_filter" is the filter name
```

In this way, when the `Create` method of the filter is called, the associated filter configuration will be passed as a parameter to the `Create` method. Users can then use `std::any_cast` in the `Create` method to retrieve the corresponding configuration.

```cpp
trpc::MessageClientFilterPtr MyFilter::Create(const std::any& param) {
  MyFilterConfig config;
  if (param.has_value()) {
    config = std::any_cast<MyFilterConfig>(param);
  }

  return std::make_shared<MyFilter>(config);
}
```
