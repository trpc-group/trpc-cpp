[中文版](../zh/server_guide.md)

[TOC]

# Server development guide

The [Quick-Start guide](quick_start.md) provides an introduction to developing a simple tRPC service. This article is an advanced piece that will delve into the considerations and tasks involved in server-side program development. If your service requires downstream calls, please read the [Client Development Guide](client_guide.md) after going through this article.

## Runtime selection

See [How to choose and configure the runtime](runtime.md#how-to-choose-and-configure-the-runtime)

## Define Proto Service

A Proto Service is a logical combination of a set of interfaces. It requires defining a package, Proto Service, RPC name, and the data types for interface requests and responses.

### IDL protocol types

The [IDL](https://en.wikipedia.org/wiki/Interface_description_language) language can describe interfaces in a programming language-independent manner, and use tools to convert IDL files into stub code in a specific language, allowing developers to focus on business logic development. For services using IDL protocol types, the definition of Proto Service typically involves the following five steps (using tRPC protocol as an example):

Step 1: Define services through proto IDL file

```protobuf
syntax = "proto3";
package trpc.test.helloworld;

service Greeter {
    rpc SayHello (HelloRequest) returns (HelloReply) {}
}

message HelloRequest {
    string msg = 1;
}

message HelloReply {
    string msg = 1;
}
```

Step 2: Generate project code based on proto IDL file

Refer to the steps in the [Quick Start Guide](quick_start.md).

Step 3: Implement server-side logic

Refer to the steps in the [Quick Start Guide](quick_start.md).

Step 4：Register the Proto Service to the Server

This step has already been done in the generated stub code, and the specific code can be found in "helloworld_server.cc":

```cpp
int helloworldServer::Initialize() {
 
  std::string service_name("trpc.");
  service_name += trpc::TrpcConfig::GetInstance()->GetServerConfig().app;
  service_name += ".";
  service_name += trpc::TrpcConfig::GetInstance()->GetServerConfig().server;
  service_name += ".Greeter";
 
  TRPC_LOG_INFO("service name1:" << service_name);
 
  trpc::ServicePtr my_service(new GreeterServiceImpl());
 
  // The service_name needs to correspond to the service/name in the configuration file to associate the service configuration.
  RegisterService(service_name, my_service);
 
  return 0;
}
```

Next, the user needs to implement the specific interfaces in greeter_service.cc, referring to the steps in the [Quick Start Guide](quick_start.md).

### Non-IDL protocol types

One of the common protocols is the HTTP protocol. For detailed information, please refer to the 
[HTTP Service Development Guide](http_protocol_service.md).

## Provide the corresponding framework configuration file

As a server, the framework configuration file needs to provide configurations for both the 'global' and 'server' sections. The 'plugins' section should be configured based on the plugins being used.

Here is an example configuration for using the fiber m:n coroutine runtime:

```yaml
global:
  local_ip: xxx.xxx.xxx.xxx  # local ip of application, optional
  threadmodel:
    fiber:
      - instance_name: fiber_instance
        concurrency_hint: 8
server:
  app: test                               # application name, optional
  server: helloworld                      # module name, optional
  service:                                # services associated with the server, required
    - name: trpc.test.helloworld.Greeter  # service name, required
      ip: xxx.xxx.xxx.xxx                 # bind ip, required
      port: 10001                         # bind port, required
      protocol: trpc                      # the application layer protocol used
plugins:
  log:
    default:
      - name: default
        sinks:
          local_file:
            filename: /usr/local/trpc/bin/server.log
```

## Asynchronous response

Sometimes, when the server receives a request, we need to asynchronously execute certain tasks and then send the response after the tasks are completed. This is where the framework's asynchronous response feature comes into play. The usage is as follows:

```cpp
::trpc::Status GreeterServiceImpl::SayHello(::trpc::ServerContextPtr context,
                                            const ::trpc::demo::helloworld::HelloRequest* request,
                                            ::trpc::demo::helloworld::HelloReply* reply) {
  // 1. Set to use asynchronous response mode.
  context->SetResponse(false);

  // 2. Do async work. Here, `DoAsyncWork` returns a Future.
  DoAsyncWork(...).Then([context](){
    ::trpc::test::helloworld::HelloReply rsp;
    rsp.set_msg("xxx");
    // 3. Call `SendUnaryResponse` to send response when the asynchronous task is completed.
    context->SendUnaryResponse(::trpc::kSuccStatus, rsp);
    return ::trpc::MakeReadyFuture<>();
  });

  return ::trpc::kSuccStatus;
}
```

Note: If you don't need to set a response body and only want to return the status, you can use the interface with only the Status parameter below:

```cpp
void SendUnaryResponse(const Status& status);
```

## Constraint

### The default maximum length for request packets is 10MB

For the built-in trpc and HTTP protocols in the framework, the maximum length of request packets is limited to 10MB. This limitation is at the service level, users can modify it by increasing the 'max_packet_size' configuration option for the service. The procedure is as follows:

```yaml
server:
  service:
    - name: trpc.test.helloworld.Greeter
      max_packet_size: 10000000  # max packet size limited
```

### Idle connection timeout

The default connection timeout for the server is 60 seconds. If the caller does not send any data for 60 seconds continuously, the connection will be disconnected. This limitation is at the service level and can be modified using the 'idle_time' configuration option:

```yaml
server:
    service:
      - name: trpc.test.helloworld.Greeter
        idle_time: 60000  # connection idle timeout
```

### Self-registration of service

By default, the framework does not automatically register or unregister service instances to the name service system. To enable it, the following configuration is required:

```yaml
server:
  registry_name: xxx   # the specified name service system
  enable_self_register: true  # Enable framework's self-registration
```

### Service heartbeat reporting and dead detection

The framework supports reporting the heartbeat of service instances to the name service system to indicate the availability status of the service itself. To use this feature, the following configuration needs to be added:

```yaml
server:
  registry_name: xxx  # the specified name service system
```

For the IO/Handle separation and merging thread models, the framework supports dead detection of worker threads (IO and Handle threads). The principle is as follows: the worker threads of framework will mark their own active status every 3 seconds. If a thread has not marked itself as active for a threshold period (60 seconds), it is considered dead.

If all Handle threads are detected as dead in the separation mode or all worker threads are detected as dead in the merging mode, the service is considered unavailable. At this point, the heartbeat reporting to the name service system will be stopped. This allows the caller to promptly perceive the service status through the name service system when the service is unavailable, to avoid calling unavailable service nodes, providing a protective effect.

For services such as offline computing, the request processing time may exceed 60 seconds, leading to false positives. To address this, the timeout threshold can be increased:

```yaml
global:
  heartbeat:
    thread_heartbeat_time_out: 60000 # Threshold in milliseconds to determine the dead state of Handle threads.
```

Additionally, if heartbeat reporting is not required, it can be disabled using the following method:

```yaml
heartbeat:
  enable_heartbeat: false  # Heartbeat reporting switch, default value is true.
```

### About fork

The 'fork' function has a historical pitfall in multi-threaded environments. Please read the [related article](https://pubs.opengroup.org/onlinepubs/009695399/functions/fork.html) to understand this premise.

tRPC-Cpp is a multi-threaded framework that only supports one usage of 'fork': 'fork' with 'exec', meaning that 'exec' functions are immediately called after 'fork'.

It does not support 'fork' without 'exec'. If you must use it in this way, please use 'fork' as early as possible (before framework initialization).

When using 'fork' with 'exec', if 'exec' returns, it indicates a failed execution. Please call '_exit()' immediately to exit the child process instead of using 'exit()'. This is done to avoid undefined behavior caused by the destruction of certain singletons or global variables (such as handling threads created by the parent process in the child process).

## Service Development for Common Protocol Types

See [Service Development for tRPC Protocol Type](trpc_service_guide.md).

See [Service Development for Http Protocol Type](http_protocol_service.md).

See [Service Development for gRPC Protocol Type](grpc_service_guide.md).

## Error Code

See [Error code of framework]().

## Advanced Usage

### Specifying the processing thread for a request

See [Fixed thread](../../examples/features/request_dispatch/README.md).

### Timeout Control

See [Timeout control]().

### Transparent proxy

See [Transparent proxy](transparent_service.md).

### Timer

See [Timer]().

### Flow-Control & Overload-Protect

See [Flow-Control & Overload-Protect]().
