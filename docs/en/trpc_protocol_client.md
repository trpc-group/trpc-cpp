[中文](../zh/trpc_protocol_client.md)

# Overview

This document primarily introduces how to use the tRPC-Cpp framework to invoke a service using the tRPC protocol.

## tRPC Protocol

The tRPC protocol is implemented based on ProtoBuf and has the ability to support streaming transmission, connection reuse, end-to-end timeouts, and transparent information transmission. For more details, please refer to the [tRPC Protocol Design](https://github.com/trpc-group/trpc/blob/main/docs/cn/trpc_protocol_design.md).

## Use Cases

**Calling a tRPC Service** refers to initiating an RPC (Remote Procedure Call) using the client module of the tRPC-Cpp framework. The use cases can be categorized into `Proxy mode` and `Pure client mode`.

- `Proxy mode`: the scenario where a service, built using the tRPC-Cpp framework (`Service B`), receives an RPC request from another service (`Service A`) and needs to access downstream services (such as `Service C`). In this case, `Service B` acts as an Proxy, receiving requests from the upstream service and making subsequent calls to the downstream services. The purpose of these calls is to access the downstream services.
  ![trpc_protocol_client-forword](../images/trpc_protocol_client-forword.png)
- `Pure client mode`: client-side calling scenarios refer to the use of the tRPC-Cpp framework to build a client application. This application only makes calls to downstream services and does not receive or process requests itself. This is the fundamental difference between client-side calling and intermediary calling.
  ![trpc_protocol_client-client](../images/trpc_protocol_client-client.png)

## Interface Form

The framework supports generating stub code based on ProtoBuf. For reference, please see [Compiling and Generating Stub Code](./trpc_protocol_service.md#compile-and-generate-stub-code). For the client, the generated code is as follows:
![rpc_interface](../images/trpc_protocol_client-rpc_interface.png)
It can be observed that two types of classes are generated: `GreeterServiceProxy` and `AsyncGreeterServiceProxy`. The `AsyncGreeterServiceProxy` class only has asynchronous interfaces, which is equivalent to `GreeterServiceProxy::AsyncSayHello`. On the other hand, `GreeterServiceProxy` contains three types of interfaces.

- Synchronous calling: the `SayHello` interface requires a request and a response in its parameter list. In terms of code flow, each request must synchronously wait for the response to arrive before proceeding with subsequent logic. This is often used in coroutine mode.
- Asynchronous calling: the `AsyncSayHello` interface (equivalent to `AsyncGreeterServiceProxy::SayHello`) does not have a response in its parameter list. In terms of code flow, after each request is sent, the subsequent logic is executed directly without waiting for a response. When a response is received from the downstream service, it triggers the callback of the [Future](../../trpc/common/future/future.h) to handle the response data. The entire process is asynchronous.
- One-way calling: the `SayHello` interface is similar to the synchronous interface, but the parameter list does not include a response. This means that only the request needs to be sent without expecting a response, and the downstream service will not return a response.

From the generated interfaces, it can be seen that the tRPC-Cpp framework provides three types of RPC interfaces. Users can choose the corresponding interface based on their business scenarios without needing to worry about the underlying implementation details. This is very convenient.

# Calling downstream tRPC protocol services

In this section, different scenarios will be explained using interface-based examples to demonstrate how to access downstream tRPC services using a framework-based client.

## Synchronously interfaces

Calling a synchronous programming interface does not necessarily mean that the downstream service is called using a synchronous blocking approach. It simply follows the synchronous programming paradigm in terms of code flow. Whether the framework's underlying implementation is blocking depends on the thread model chosen by the user. When using the [fiber thread model](../en/fiber_user_guide.md), users can achieve the simplicity of synchronous programming and the high performance of asynchronous I/O. Therefore, the fiber thread model is also the recommended approach in the tRPC-Cpp framework. **When using a non-fiber thread model, synchronous interfaces will be blocked at the framework's underlying level, severely impacting performance. Therefore, it is strongly discouraged to use synchronous interfaces unless necessary**. So this section will only introduce the usage of synchronous interfaces in the fiber thread model. Since the framework implements coroutine features using the fiber thread model, coroutines and the fiber thread model can be considered equivalent in the following context.

The usage of synchronous programming interfaces differs in the proxy mode and the pure client mode. They will be described separately below.

### Synchronous calling by proxy mode

The standard implementation approach is to use an IDL (such as ProtoBuf) to generate a service proxy class called `xxxServiceProxy`, where *xxx* represents the service name defined in the IDL. For example, in the diagram from the previous section, it would be `GreeterServiceProxy`.

- The synchronous calling method by proxy mode, using [forward_service](../../examples/features/fiber_forward/proxy/forward_service.cc) as an example, is as follows in pseudocode:

  ```cpp
  ForwardServiceImpl::ForwardServiceImpl() {
    greeter_proxy_ =
        ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>("trpc.test.helloworld.Greeter");
  }
  
  ::trpc::Status ForwardServiceImpl::Route(::trpc::ServerContextPtr context,
                                           const ::trpc::test::helloworld::HelloRequest* request,
                                           ::trpc::test::helloworld::HelloReply* reply) {
    TRPC_FMT_INFO("Forward request:{}, req id:{}", request->msg(), context->GetRequestId());
  
    auto client_context = ::trpc::MakeClientContext(context, greeter_proxy_);
  
    ::trpc::test::helloworld::HelloRequest route_request;
    route_request.set_msg(request->msg());
  
    ::trpc::test::helloworld::HelloReply route_reply;
  
    // block current fiber, not block current fiber worker thread
    ::trpc::Status status = greeter_proxy_->SayHello(client_context, route_request, &route_reply);
  
    TRPC_FMT_INFO("Forward status:{}, route_reply:{}", status.ToString(), route_reply.msg());
  
    reply->set_msg(route_reply.msg());
  
    return status;
  }
  ```

For the proxy mode, the following points should be noted:

- The service proxy class is used with smart pointers. You can obtain the corresponding smart pointer using the`::trpc::GetTrpcClient()->GetProxy` template interface. The template parameter is the service proxy class generated by the stub code. The interface parameter is the name of the service proxy class, which serves as a unique index for the server-side proxy class object, making it easy for users to quickly obtain it. It also serves as the callee name, which may be used for service discovery, so it is usually required to be consistent with the `client->service->name` in the configuration file.
- The client context must be created using the `MakeClientContext` interface.
- The downstream service is synchronously called through the service proxy object, such as `greeter_proxy_->SayHello`.
- Proxy service not only provides services but also accesses downstream services as a client, a configuration file is required.

- Configuring the fiber thread model

  At the code level, you don't need to worry about the details of the thread model. However, you can configure the fiber thread model in the configuration file. Taking trpc_cpp_fiber.yaml as an example, the key configurations are as follows (for detailed framework configurations, refer to [framework_config_full](./framework_config_full.md)):
  
  ```yaml
  global:
    threadmodel:
      fiber:                            # Use Fiber(m:n coroutine) threadmodel
        - instance_name: fiber_instance # Need to be unique if you config mutiple fiber threadmodel instances
          # Fiber worker thread num
          # If not specified, will use number of cores on the machine.
          # In a Numa architecture, the workers will be automatically grouped (8-15 cores per group),
          # while in Uma architecture, they will not be grouped.
          concurrency_hint: 8
  server:
    app: test
    server: forword
     # ...
    service:
      - name: trpc.test.forword.Forward
        protocol: trpc
        network: tcp
        # ...
  
  client:
    service:
      - name: trpc.test.helloworld.Greeter
        target: 0.0.0.0:12345
        protocol: trpc
        timeout: 1000
        network: tcp
        # ...
  ```

From the above configuration, it can be noted that there are a few points to consider when configuring the fiber thread model:

- fiber: Indicates the use of the fiber thread model and is a required configuration. Configuring it under `global->threadmodel` means that the thread model is globally shared, making it accessible to all services under both `server` and `client`.
- instance_name: Represents the name of the thread model instance and is a required configuration. It does not have to be specifically set as `fiber_instance`. **Currently, the framework only allows for configuring one instance of the fiber thread model**, so there can only be one `instance_name`.
- concurrency_hint: Represents the number of worker threads for fibers and is an optional configuration. If this field is not specified, the framework will set the number of worker threads based on the number of cores in the system.

### Synchronous calling by pure client

In a pure client calling scenario, tRPC-Cpp framework is used to build a client application that only makes calls to downstream services without responding to requests. The client application is not embedded within the framework, so the initialization of the `runtime` (including thread model, plugins, and other components) needs to be done by the user before initiating the RPC calls.

- In the pure client calling approach, let's take the [fiber_client](../../examples/helloworld/test/fiber_client.cc) as an example.

  ```cpp
  int DoRpcCall(const std::shared_ptr<::trpc::test::helloworld::GreeterServiceProxy>& proxy) {
    ::trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(proxy);
    ::trpc::test::helloworld::HelloRequest req;
    req.set_msg("fiber");
    ::trpc::test::helloworld::HelloReply rsp;
    ::trpc::Status status = proxy->SayHello(client_ctx, req, &rsp);
    if (!status.OK()) {
      std::cerr << "get rpc error: " << status.ErrorMessage() << std::endl;
      return -1;
    }
    std::cout << "get rsp msg: " << rsp.msg() << std::endl;
    return 0;
  }
  
  int Run() {
    auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>(FLAGS_service_name);
  
    return DoRpcCall(proxy);
  }

  int main(int argc, char* argv[]) {
  ParseClientConfig(argc, argv);

  // If the business code is running in trpc pure client mode,
  // the business code needs to be running in the `RunInTrpcRuntime` function
  return ::trpc::RunInTrpcRuntime([]() { return Run(); });
  }
  ```

  In comparison to synchronous calls in the proxy mode, there are a few differences:

  - ::trpc::RunInTrpcRuntime: This is a required call to initialize the framework's runtime, which includes the thread model, plugins, and other components.
  - The construction of the context must be done using the `MakeClientContext` interface. However, the parameter list is different from that in the intermediate service, as there is no server-side context. This is an overloaded function with the same name.

- Configuration file for pure client
  since it does not provide any services, there is no need for a `server` configuration. However, you still need to configure the thread model, and the other configurations are similar to those in the intermediate service configuration. You can refer to the [trpc_cpp_fiber.yaml](../../examples/helloworld/test/conf/trpc_cpp_fiber.yaml) file for reference.

### Concurrent synchronous calling

In many scenarios, users may need to make multiple synchronous API calls simultaneously. Whether it is in a proxy service or a pure client application, the approach for concurrent synchronous calls remains the same. Here, we will use concurrent synchronous calls in a proxy service as an example.

- The code for concurrent synchronous calls using fibers. We will use the forward_service as an example

  ```cpp
  ::trpc::Status ForwardServiceImpl::ParallelRoute(::trpc::ServerContextPtr context,
                                                   const ::trpc::test::helloworld::HelloRequest* request,
                                                   ::trpc::test::helloworld::HelloReply* reply) {
  
    // send two requests in parallel to helloworldserver
  
    int exe_count = 2;
    ::trpc::FiberLatch l(exe_count);
  
    std::vector<::trpc::test::helloworld::HelloReply> vec_final_reply;
    vec_final_reply.resize(exe_count);
  
    int i = 0;
    while (i < exe_count) {
      bool ret = ::trpc::StartFiberDetached([this, &l, &context, &request, i, &vec_final_reply] {
  
        trpc::test::helloworld::HelloRequest request;
        std::string msg = request->msg();
        // ...
  
        auto client_context = ::trpc::MakeClientContext(context, greeter_proxy_);
        ::trpc::Status status = greeter_proxy_->SayHello(client_context, request, &vec_final_reply[i]);
  
        // ...
        // when reduced to 0, the FiberLatch `Wait` operation will be notified
        l.CountDown();
      });
  
      // ...
    
      i += 1;
    }
  
    // wait for two requests to return
    // block current fiber, not block current fiber worker thread
    l.Wait();
  
    // ...
  
    return ::trpc::kSuccStatus;
  }
  ```

  For multiple synchronous calls, the [StartFiberDetached](../../trpc/coroutine/fiber.h) interface is used. This interface creates a coroutine to handle the incoming callback function. In the above code, the anonymous callback function calls the synchronous interface `SayHello`. Therefore, calling `StartFiberDetached` multiple times will create multiple coroutines, and these coroutines will execute concurrently. As a result, multiple synchronous interfaces will also be executed concurrently.

  To wait for all coroutines to finish, we can use trpc::FiberLatch. This class has the same semantics as `std::latch` in C++, both used for thread synchronization. In the code, the `l.Wait()` operation waits for all coroutines to execute. Each coroutine calls `l.CountDown()` after execution. When all coroutines have finished executing, `l` becomes 0, and `l.Wait()` stops blocking.

  Since concurrent calls to synchronous interfaces have no difference in configuration for individual calls, we will not continue to discuss the configuration.

## Asynchronously interfaces

The asynchronous programming interface uses the `Future/Promise` semantics. You can refer to the [future introduction](./future_promise_guide.md) for more information. The client's asynchronous API has two forms. For example, the asynchronous interface generated by the `Greeter` service stub code is `GreeterServiceProxy::AsyncSayHello` and `AsyncGreeterServiceProxy::SayHello`. Both are equivalent, you can choose either one. For asynchronous interfaces, the framework supports all built-in thread models. Since the `fiber thread model` was introduced in the synchronous interface section, here we will only discuss the `non-fiber thread models`.

### Asynchronously calling by proxy mode

- Here is the code for asynchronous calls in the proxy service. We will use the [forward_service](../../examples/features/future_forward/proxy/forward_service.cc) as an example.

  ```cpp
  ForwardServiceImpl::ForwardServiceImpl() {
    greeter_proxy_ =
        ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>("trpc.test.helloworld.Greeter");
  }
  
  ::trpc::Status ForwardServiceImpl::Route(::trpc::ServerContextPtr context,
                                           const ::trpc::test::helloworld::HelloRequest* request,
                                           ::trpc::test::helloworld::HelloReply* reply) {
    TRPC_FMT_INFO("Forward request:{}, req id:{}", request->msg(), context->GetRequestId());
  
    // use asynchronous response mode
    context->SetResponse(false);
  
    auto client_context = ::trpc::MakeClientContext(context, greeter_proxy_);
  
    // ...
  
    ::trpc::test::helloworld::HelloReply route_reply;
  
    greeter_proxy_->AsyncSayHello(client_context, route_request)
        .Then([context](::trpc::Future<::trpc::test::helloworld::HelloReply>&& fut) {
          ::trpc::Status status;
          ::trpc::test::helloworld::HelloReply reply;
  
          if (fut.IsReady()) {
            std::string msg = fut.GetValue0().msg();
            reply.set_msg(msg);
            TRPC_FMT_INFO("Invoke success, route_reply:{}", msg);
          } else {
            auto exception = fut.GetException();
            status.SetErrorMessage(exception.what());
            status.SetFrameworkRetCode(exception.GetExceptionCode());
            TRPC_FMT_ERROR("Invoke failed, reason:{}", exception.what());
            reply.set_msg(exception.what());
          }
  
          context->SendUnaryResponse(status, reply);
          return ::trpc::MakeReadyFuture<>();
        });
  
    return ::trpc::kSuccStatus;
  }
  ```

  Here are some important points to consider when making asynchronous calls in the proxy service:

  - Set the asynchronous response using `context->SetResponse(false)`. Here, **false** indicates an asynchronous response. It means that after calling `AsyncSayHello`, the function `Route` will not wait for the response to arrive and will directly exit the function stack. If the framework detects that the downstream response has arrived, it will be handled through the callback specified in the `Then` function.
  - Other aspects of making asynchronous calls in the proxy service are consistent with synchronous calls.

- Configuration for the non-fiber thread model in the [trpc_cpp_future.yaml](../../examples/features/future_forward/proxy/trpc_cpp_future.yaml) file

  ```yaml
  global:
  threadmodel:
    default:
      - instance_name: default_instance
        io_handle_type: separate
        io_thread_num: 2
        handle_thread_num: 2
  server:
    app: test
    server: forword
    admin_port: 8889
    admin_ip: 0.0.0.0
    service:
      - name: trpc.test.forword.Forward
        protocol: trpc
        network: tcp
        ip: 0.0.0.0
        port: 12346
  
  client:
    service:
      - name: trpc.test.helloworld.Greeter
        target: 0.0.0.0:12345      
        protocol: trpc
        timeout: 1000
        network: tcp
        conn_type: long
        is_conn_complex: true
        selector_name: direct
  ```

  The only difference in configuration with [trpc_cpp_fiber.yaml](../../examples/features/fiber_forward/proxy/trpc_cpp_fiber.yaml) is the thread model. Here the thread model type is set to `default`. More configuration fields are as follows:

  - instance_name: The name of the thread model instance. Similar to the fiber thread model, multiple instances can be configured.
  - io_handle_type: Indicates the instance type. Currently, there are two types: `separate` and `merge`.
    `separate`: Indicates that I/O handling and business processing are performed in different threads. Suitable for CPU-intensive tasks.
    `merge`: Indicates that I/O handling and business processing threads are shared. Suitable for I/O-intensive tasks.
  - io_thread_num: The number of threads for I/O handling.
  - handle_thread_num: The number of threads for business processing. Only valid in the `separate` mode.
  - Based on experience, in the `separate` mode, it is recommended to ensure the following conditions: io_thread_num + handle_thread_num < number of CPU cores * 2, and the ratio of io_thread_num to handle_thread_num is 1:3.

### Asynchronously calling by pure client mode

To make asynchronous calls in the client, you need to initialize the tRPC-Cpp framework `runtime`, just like synchronous calls.

- Here is an example of how to make pure client-side asynchronous calls using the [client](../../examples/features/future_forward/client/client.cc)

  ```cpp
  
  void DoRoute(const std::shared_ptr<::trpc::test::route::ForwardServiceProxy>& prx) {
    ::trpc::test::helloworld::HelloRequest request;
    request.set_msg("future");
  
    auto context = ::trpc::MakeClientContext(prx);
    context->SetTimeout(1000);
  
    if (!FLAGS_sync_call) {
      ::trpc::Latch latch(1);
      prx->AsyncRoute(context, request).Then([&latch](::trpc::Future<::trpc::test::helloworld::HelloReply>&& rsp_fut) {
        // ...
        auto reply = rsp_fut.GetValue0();
  
        latch.count_down();
        return ::trpc::MakeReadyFuture<>();
      });
      latch.wait();
    } 
    // ...
  }
  
  int Run() {
    ::trpc::ServiceProxyOption option;
  
    option.name = FLAGS_target;
    // ...
  
    auto prx = ::trpc::GetTrpcClient()->GetProxy<::trpc::test::route::ForwardServiceProxy>(FLAGS_target, option);
  
    //...
    
    DoRoute(prx);
  
    // ...
    return 0;
  }
  
  int main(int argc, char* argv[]) {
    ParseClientConfig(argc, argv);
  
    // If the business code is running in trpc pure client mode,
    // the business code needs to be running in the `RunInTrpcRuntime` function
    return ::trpc::RunInTrpcRuntime([]() { return Run(); });
  }
  ```

  Differences compared to asynchronous calls in the proxy service:

  - ::trpc::RunInTrpcRuntime: Initializes the `runtime` (including thread model, plugins, and other components) and is a required call.
  - The client context is constructed using the `MakeClientContext` interface, but the parameter list is different from the forward service. There is no server-side context, and it belongs to the same overloaded function.
  - Waiting for the asynchronous call to complete is achieved using `::trpc::Latch` which is like `std::latch` of c++.

### Concurrent asynchronous calling

The framework also supports concurrent asynchronous calls, similar to concurrent synchronous calls.

- Concurrent asynchronous calls can be made using the following code, taking the [forward_service](../../examples/features/future_forward/proxy/forward_service.cc) file as an example:

  ```cpp
  ::trpc::Status ForwardServiceImpl::ParallelRoute(::trpc::ServerContextPtr context,
                                                   const ::trpc::test::helloworld::HelloRequest* request,
                                                   ::trpc::test::helloworld::HelloReply* reply) {
    // ...
    // use asynchronous response mode
    context->SetResponse(false);
    // ...
    int exec_count = 2;
    std::vector<::trpc::Future<::trpc::test::helloworld::HelloReply>> results;
    results.reserve(exec_count);

    for (int i = 0; i < exec_count; i++) {
      std::string msg = request->msg();
      msg += ", index";
      msg += std::to_string(i);
  
      trpc::test::helloworld::HelloRequest request;
      request.set_msg(msg);
  
      auto client_context = ::trpc::MakeClientContext(context, greeter_proxy_);
      results.emplace_back(greeter_proxy_->AsyncSayHello(client_context, request));
    }
  
    ::trpc::WhenAll(results.begin(), results.end())
        .Then([context](std::vector<trpc::Future<trpc::test::helloworld::HelloReply>>&& futs) {
          trpc::Status status;
          trpc::test::helloworld::HelloReply reply;
          std::string rsp = "parallel result:";
          int i = 0;
          for (auto&& fut : futs) {
            // ...
            auto msg = fut.GetValue0().msg();
            i++;
          }
          // ...
          context->SendUnaryResponse(status, reply);
          return trpc::MakeReadyFuture<>();
        });

      return trpc::kSuccStatus;
  }
  ```

  By using `results.emplace_back(greeter_proxy_->AsyncSayHello(client_context, request))`, we can store the `Future` values returned by each asynchronous API call. Then, we can use `::trpc::WhenAll` to wait for the completion of each asynchronous call and process the corresponding response values.

## One-way calling

The framework provides one-way call interfaces, which are convenient for scenarios where only requests are sent without receiving responses. The form of the interface is similar to synchronous interfaces, but with fewer parameters as there is no response data pointer. One-way calls are supported in both coroutine and non-coroutine models.

# Special scenarios and considerations

## Connection reuse and connection pool

- Connection reuse
  Multiple requests sharing a single connection can save connection establishment overhead and provide performance advantages. However, it requires the transmission protocol to have a unique identifier to match requests and responses. Currently, protocols such as trpc support connection reuse. Users do not need to set any special configurations as the framework will prioritize using connection reuse mode based on the protocol. **When using the fiber coroutine mode with connection reuse, do not set the maximum number of connections. The system defaults to 1, which is sufficient for most scenarios.**
- Connection pool
  Support for all protocols, each time a request is sent, a connection is obtained from the pool. If there are no available connections and the number of alive connections is less than the maximum, a new connection is created. After receiving a response, the connection is returned to the pool for subsequent requests. If an exception occurs, the connection is closed. To improve performance, you can increase the maximum number of connections, which is set to 64 by default, but it is not recommended to increase it without limit.

## tcp/udp

The framework supports both TCP and UDP transport protocols. Users can configure the desired protocol type to be used.

```yaml
  service:
    - name: trpc.test.forword.Forward
      # ...
      network: tcp
      # ...
```

## 长/短连接

```yaml
  service:
    - name: trpc.test.forword.Forward
      # ...
      conn_type: long # short
      # ...
```

In the coroutine model, both TCP and UDP protocols are supported. However, in the non-coroutine thread model, only long connections are currently supported.

## backup-request

Sometimes, to ensure availability, it is necessary to access two services simultaneously, and the response from whichever service arrives first is used. In the tRPC-Cpp framework, when the user sets to use `backup-request`, it selects a specified number of node IPs (usually 2) simultaneously based on the settings. If the first request does not receive a response within the `delay` time, the same request is sent to the remaining nodes simultaneously, and the earliest response is used. For more information, please refer to [backup-request](./backup-request.md).

## Support for requests that accept non-contiguous buffers serialized in ProtoBuf

Currently, there is a business requirement to allow requests to accept non-contiguous buffers serialized in ProtoBuf. This scenario is commonly seen when there is a need to reuse a request body and make multiple adjustments to its content. To avoid unnecessary copying of the request body, the modified ProtoBuf request can be serialized and saved before making downstream calls. You can refer to the following example for more information:

```c++

trpc::Status ForwardServiceImpl::SayHelloUseNoncontiguousBufferReq(
    trpc::ServerContextPtr context, const trpc::test::helloworld::HelloRequest* request,
    trpc::test::helloworld::HelloReply* reply) {

  int exe_count = 10;

  // fiber latch for synchronous
  trpc::FiberLatch l(exe_count);

  // Save results from downstream
  std::vector<trpc::test::helloworld::HelloReply> vec_final_reply;
  vec_final_reply.resize(exe_count);

  // Serialize the protobuf request into a non-contiguous buffer by the framework's `pb_serialization`.
  trpc::serialization::SerializationPtr pb_serialization =
      trpc::serialization::SerializationFactory::GetInstance()->Get(trpc::EncodeType::PB);

  // An array of serialized trpc::NoncontiguousBuffer.
  std::vector<trpc::NoncontiguousBuffer> vec_req_buffer;
  vec_req_buffer.resize(exe_count);

  // ProtoBuf data that needs to be reused.
  trpc::test::helloworld::HelloRequest reuse_request;

  for (size_t i = 0; i < exe_count; i++) {
    // Full request
    reuse_request.set_msg(" modify as you like,i:" + std::to_string(i));

    // Call the ProtoBuf serialization tool to serialize.
    trpc::NoncontiguousBuffer req_buffer;
    bool encode_ret = pb_serialization->Serialize(
        trpc::serialization::kPbMessage,
        reinterpret_cast<void*>(const_cast<trpc::test::helloworld::HelloRequest*>(&reuse_request)),
        &req_buffer);
    if (TRPC_UNLIKELY(!encode_ret)) {
      // Error
      context->SetStatus(trpc::Status(
          trpc::codec::GetDefaultClientRetCode(trpc::codec::ClientRetCode::ENCODE_ERROR),
          "encode failed."));
      return trpc::Status(-1, "");
    }
    // Save req_buffer
    vec_req_buffer[i] = std::move(req_buffer);
  }

  int i = 0;
  while (i < exe_count) {
    trpc::StartFiberDetached([this, &l, &context, i, &vec_req_buffer, &vec_final_reply] {
      auto client_context = trpc::MakeClientContext(context, route_proxy_);
      // Set func name
      client_context->SetFuncName("/trpc.test.helloworld.Greeter/SayHello");

      // Directly invoke `PbSerializedReqUnaryInvoke` with the request type as a serialized trpc::NoncontiguousBuffer and the response type as a ProtoBuf.
      trpc::Status status =
          route_proxy_->PbSerializedReqUnaryInvoke<trpc::test::helloworld::HelloReply>(
              client_context, vec_req_buffer[i], &vec_final_reply[i]);

      // Sub `FiberLatch`，the `Wait` operation will be awakened when it is reduced to 0.
      l.CountDown();
    });
    i += 1;
  }

  // Synchronously wait for awakening, which will only block the current fiber without affecting the current thread.
  l.Wait();

  // ...  omit some code

  return trpc::kSuccStatus;
}
```

Here is a simple description of the pseudocode flow:

- The user serializes the request of the protobuf type and stores it in `vec_req_buffer`.
- Using `StartFiberDetached`, a fiber is created to execute parallel access to downstream logic, and the result is awaited.
- Since it is not using the framework's stub code, the downstream function name needs to be set manually by `client_context->SetFuncName("/trpc.test.helloworld.Greeter/SayHello");`. You can refer to the function names in the stub code.
- The downstream is accessed using `RpcServiceProxy::PbSerializedReqUnaryInvoke`. The request parameter type is expected to be `trpc::NoncontiguousBuffer`, for example: `vec_req_buffer[i]`. The response parameter is expected to be a protobuf structure, for example: `trpc::test::helloworld::HelloReply`.
- The remaining part is the logic for waiting and responding, which is not described here in detail.
