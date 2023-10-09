[中文](../zh/basic_tutorial.md)

[TOC]

# Basic Tutorial

## Overview

This tutorial provides a basic C++ programmer’s introduction to working with tRPC-Cpp.

In [Quick Start](quick_start.md), you have successfully run HelloWorld demo. Next, we will show an example of proxy service for forwarding request, through this example, you will understand how to:

- Define the service in a .proto file.
- Use trpc-cmdline tool to generate server and client code.
- Use synchronous and asynchronous APIs to access backend services.

The calling relationship of proxy service (we call it Forward for short) is as follows:

Client <---> Forward <---> HelloWorld

- `HelloWorld`: service in [Quick Start](quick_start.md)
- `Forward`: receive the client's request, and then call the HelloWorld service
- `Client`: client test code

## Install Environment

Before compiling and running the Forward example in this article, you need to: 

1. Follow [Quick Start](quick_start.md) to install the development environment and compile the framework code.
2. Install [trpc-cmdline](https://github.com/trpc-group/trpc-cmdline#install-from-release) tools.

## Defining the service

Our first step is to define the tRPC-Cpp service and the method request and response types using protocol buffers.

To define a service, you can define a `service` with a specific name in `.proto` file.

```protobuf
service Forward {
   ...
}
```

Then, you can defines `rpc` methods in `service`, specifying their request and response types. tRPC-Cpp allows you to define a one-response-one-response (unary) rpc method and a streaming (stream) rpc. Only the one-response-one-response (unary) rpc method is used here, and the stream (stream) rpc can refer to [trpc stream](trpc_protocol_streaming_service.md) . The content of our `forward.proto` file is defined as follows, it dependes on `message.proto` which can be located at `deps` directory start from `forward.proto` location():

```protobuf
syntax = "proto3";

package trpc.demo.forward;

import "deps/message.proto";

service ForwardService {
  rpc Route (ForwardRequest) returns (ForwardReply) {}
}
```

CD into the location where you place `forward.proto`, create a directory `deps` and place `message.proto` inner.

```protobuf
syntax = "proto3";

package trpc.demo.forward;

message ForwardRequest {
   string msg = 1;
}

message ForwardReply {
   string msg = 1;
}
```

Here it is recommended to refer to [terminology](https://github.com/trpc-group/trpc/blob/main/docs/en/terminology.md) for the name of the package of both proto files. Here we use the three-segment name trpc.{ app}.{server}, {app} here is demo, {server} is forward.

## Generate code based on proto file

### Generate service code

First, create a directory by executing the following command, which is used to experience the use of tRPC-Cpp following this tutorial.

```shell
mkdir trpc-cpp-quickstart
cd trpc-cpp-quickstart
```

Put the `forward.proto` file and `deps` directory in the trpc-cpp-tutorial directory, execute `tree .`, you can check below file structure:

```shell
.
├── deps
│   └── message.proto
└── forward.proto
```

If you have proto files with complex dependency, they must be organized at a directory before use trpc-cmdline tool. For examples, if proto files have below dependency:

```text
test3/test.proto ---> test2/test.proto --->
                                            mytest/helloworld.proto
                      test/test.proto  --->
```

You should organize them as below:

```shell
.
├── mytest
│   └── helloworld2.proto
├── test
│   └── test.proto
├── test2
│   └── test.proto
└── test3
    └── test.proto
```

Execute the following command in the trpc-cpp-tutorial directory:

```shell
trpc create -l cpp --protofile=forward.proto
# The complex example above should use command: trpc create -l cpp --protofile=mytest/helloworld2.proto
```

Among them, `trpc` is the trpc-cmdline tool you install at `Install Environment` step, the two parameters of this tool are the -l specified the tool should generate trpc-cpp project and --protofile specified the tool should generate project based on which proto file.

After the command runs successfully, a project will be generated under trpc-cpp-tutorial. The project name is the server name is the same as proto file. For example: server name here is forward，you can execute `tree forward` to  check the following file structure:

```shell
forward
├── build.sh
├── clean.sh
├── client
│   ├── BUILD
│   ├── conf
│   │   ├── trpc_cpp_fiber.yaml
│   │   └── trpc_cpp_future.yaml
│   ├── fiber_client.cc
│   └── future_client.cc
├── proto
│   ├── BUILD
│   ├── deps
│   │   └── message.proto
│   ├── forward.proto
│   └── WORKSPACE
├── README.md
├── run_client.sh
├── run_server.sh
├── server
│   ├── BUILD
│   ├── conf
│   │   ├── trpc_cpp_fiber.yaml
│   │   └── trpc_cpp.yaml
│   ├── server.cc
│   ├── server.h
│   ├── service.cc
│   └── service.h
└── WORKSPACE
```

*PS: the genearted project will use bazel to build. Genearting a project using cmake to build is not supported currently.*

The code in the server directory is the code related to the implementation of this service, the code in the client directory is the client code for the local testing, and the code in the proto directory is a project that manages proto file with it's dependency.

The server directory contains a framework configuration file directory, 4 code files, and a bazel BUILD file.
- Configuration directory: It contains two framework configuration files, one is the framework configuration file of fiber (M:N coroutine), which means that the current service will use the fiber runtime, and users will uses synchronous api to writing business code, and the other is the framework configuration file of ordinary thread runtime, which means that the current service will use the merged or separated operating environment of the framework, and the business code will use the asynchronous future-related api. **When the business writes tRPC-Cpp services, it must first choose a runtime environment of the framework**. Refer to [runtime](to do) for the introduction of the fiber runtime environment, merge or separate runtime environment, and how to choose.
- BUILD file: indicates that the program is built using bazel rule which depends on relative bazel rule of forward.proto in proto directory.
- Code file: Contains two types of files, one is the service process level code file (server.h and server.cc), which defines a ForwardServer class, which inherits the `TrpcApp` class of the framework, and the business needs to override its `Initialize` and `Destroy` methods to complete the business-related initialization operations during the startup process of the service process, and the business destruction operations during the shutdown process; one type is the service-level code file (service .h and service.cc), these two files are the concrete implementation of the proto interface.

The specific implementation of the forward service can be divided into the following steps:
1. Choose the runtime environment used by the forward service (we also call it the thread model), and set it in the global configuration item of the framework configuration;
2. Define a process-level implementation class (here ForwardServer), inherit the `TrpcApp` class of the framework, and override its `RegisterPlugins`, `Initialize` and `Destroy` methods;
3. Define a service-level implementation class (here ForwardServiceServiceImpl), inherit the service class defined by the proto file, and override the RPC method, where the first parameter of each RPC method is the context of the current RPC execution `ServerContext `;
4. If necessary, complete the registration of the custom plug-in in the `RegisterPlugins` method in 1, for example: register the custom protocol `Codec`;
5. Complete the process-level business initialization operation in the `Initialize` method in 1;
    1. First initialize the business-related logical operations that depend on the framework, such as: pull remote configuration, create and start a thread pool;
    2. After the business-related operations are completed, the Service registration operation is usually performed last. Here, the ForwardServiceServiceImpl object is created, and then call `RegisterService` to register it in the ForwardServer. Note that there will only be one ForwardServiceServiceImpl object in the process.
6. Complete the process-level business destruction operation in the `Destroy` method in 1;
    1. Here generally stop the dynamic resources created in `Initialize`, for example: stop running the thread pool;
7. Define the instance of the process-level implementation class at the entry function `main` of the program, and then call its `Main` and `Wait` methods to start the program;

The client directory includes a framework configuration file directory, 2 code files, and a bazel BUILD file.
- Configuration directory: It contains two framework configuration files, one is the framework configuration file of fiber (M:N coroutine), which means that the current test program will use the fiber runtime, and the business will use fiber-related synchronization apis when writing code, the other is the framework configuration file of ordinary threads, indicating that the current test program will use the merged or separated runtime environment, and the business code will use future-related asynchronous apis.
- BUILD file: indicates that the program is built using bazel rules which depends on relative bazel rule of forward.proto in proto directory.
- Code file: Contains two files, one is a test code file that uses fiber to call the forward service synchronously (fiber_client.cc); the other is a test code file that uses a future to asynchronously call the forward service (future_client.cc). ** choose one when you tests**.

The overall logic flow of the two test codes fiber_client and future_client is the same, which can be divided into the following steps:
1. Parse and initialize framework configuration;
2. Put the business execution function `Run` into the runtime of the framework for execution;
    1. The `Run` function will first create a specific proxy `ServiceProxy` to access the backend service;
    2. Then use `ServiceProxy` to call the RPC interface of the backend service. The difference between fiber and future here is that one is a synchronous call and the other is an asynchronous call. In addition, when calling RPC, you need to create the context `ClientContext for the current RPC call `.

The proto direcotry includes a bazel WORKSPACE file, a bazel BUILD file, and 2 proto files.
- WORKSPACE file: indicate the proto directory is viewed as a independent bazel project. If check WORKSPACE file in root directory(name as forward project), you will find the proto project is imported by `local_repository`.
- BUILD file: use bazel rules to manage proto files which may contain complex dependency，it can be imported by `@proto//:xxx_proto` at BUILD files of server/client in forward project to use the generated C++ stub files.
- proto files: includes 2 proto files with dependency: `forward.proto` depends on `deps/message.proto`. It's file structure will be keeped at proto directory.

### Test

Enter the forward directory，compile and run the forward service (first compile will be slightly longer, and subsequent code changes and recompilation will be very fast).

```shell
./run_server.sh
```

After the forward starts and runs successfully, run the forward client test program.

```shell
./run_client.sh
```

If successful, you will see the following results.

fiber_client output:

```shell
FLAGS_service_name: trpc.demo.forward.ForwardService
FLAGS_client_config: client/conf/trpc_cpp_fiber.yaml
get rsp success
```

future_client output:

```shell
FLAGS_service_name: trpc.demo.forward.ForwardService
FLAGS_client_config: client/conf/trpc_cpp_future.yaml
get rsp success
```

## Access the backend service in the tRPC-Cpp service

Now that we have generated the service code based on the proto file and run it successfully, how to access the back-end service `HelloWorld` in the `tRPC-Cpp service`? The `tRPC-Cpp service` here is the program that serves as both the server and the client, as for the `future_client` and `future_client` programs in the previous section, they are only clients.

### Use fiber synchronously call the backend service

#### configure fiber runtime

When using fiber, you need to set the fiber-related runtime options in the framework configuration file. For example: Forward fiber-related global configuration of ./server/conf/trpc_cpp_fiber.yaml is as follows:

```shell
global:
  threadmodel:
    fiber:                              # Fiber threadmodel
      - instance_name: fiber_instance   # Thread instance unique identifier, currently does not support configuring multiple Fiber thread instances
        concurrency_hint: 8             # Number of physical threads running Fiber Workers, it is recommended to configure it as the actual number of available cores (otherwise read system configuration)
```

For the detailed fiber runtime configuration, please refer to [Framework Configuration](framework_config_full.md).

#### Get the proto file of the backend service

Here our backend service is the HelloWorld service, so the proto IDL file we want to obtain is helloworld.proto. Because of the bazel build tool we use, there is no need to copy helloworld.proto to the local, we only need to add the relevant dependencies in the bazel target rule when using helloworld.proto.

The code files service.h and service.cc depend on helloworld.proto, so we only need to rely on the target of helloworld.proto in the target build rule of service. For example: the service build target of ./server/BUILD is as follows:

```shell
cc_library(
    name = "service",
    srcs = ["service.cc"],
    hdrs = ["service.h"],
    deps = [
        "@proto//:forward_proto",
        "@trpc_cpp//examples/helloworld:helloworld_proto",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/log:trpc_log",
    ],
)
```

#### Create the ServiceProxy of the backend service

Before creating the ServiceProxy of the backend service, you need to define the options for creating the ServiceProxy. Here we set it through configuration. In addition to the configuration method, the framework also supports setting through code, code + configuration, for details, see [Client Development Guide](client_guide.md).

For example: The client-related ServiceProxy options in ./server/conf/trpc_cpp_fiber.yaml are configured as follows:

```shell
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

Here in the Forward service, we use ip/port to access the HelloWorld service directly. The target is the ip/port of the HelloWorld service.

After setting the options of the ServiceProxy, then creating and using the ServiceProxy to access the helloworld service.

First, add a constructor to the ForwardServiceServiceImpl class in ./server/service.h, and define a GreeterServiceProxy type smart pointer member variable.

```cpp
...
#include "examples/helloworld/helloworld.trpc.pb.h"
...
using GreeterProxyPtr = std::shared_ptr<::trpc::test::helloworld::GreeterServiceProxy>;

class ForwardServiceServiceImpl : public ::trpc::demo::forward::ForwardService {
 public:
  ForwardServiceServiceImpl();
 ...
 private:
  GreeterProxyPtr greeter_proxy_;
};
...
```

Then create GreeterServiceProxy in the ForwardServiceServiceImpl constructor of ./server/service.cc.

```cpp
...
#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/log/trpc_log.h"
...
ForwardServiceServiceImpl::ForwardServiceServiceImpl() {
  greeter_proxy_ =
      ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>("trpc.test.helloworld.Greeter");
}
...
```

Create a GreeterServiceProxy smart pointer using the `GetProxy` interface of `::trpc::GetTrpcClient()`.

#### Use ServiceProxy to call the backend service

Aftercreating GreeterServiceProxy, then use GreeterServiceProxy in the Route method of ForwardServiceServiceImpl to call the HelloWorld service.

```cpp
...
::trpc::Status ForwardServiceServiceImpl::Route(::trpc::ServerContextPtr context,
                                                const ::trpc::demo::forward::ForwardRequest* request,
                                                ::trpc::demo::forward::ForwardReply* reply) {
  TRPC_FMT_INFO("request msg: {}, client ip: {}", request->msg(), context->GetIp());

  auto client_context = ::trpc::MakeClientContext(context, greeter_proxy_);

  ::trpc::test::helloworld::HelloRequest hello_request;
  hello_request.set_msg(request->msg());

  ::trpc::test::helloworld::HelloReply hello_reply;

  // block current fiber, not block current fiber worker thread
  ::trpc::Status status = greeter_proxy_->SayHello(client_context, hello_request, &hello_reply);

  TRPC_FMT_INFO("status: {}, hello_reply: {}", status.ToString(), hello_reply.msg());

  reply->set_msg(hello_reply.msg());

  return status;
}
...
```

#### Run the test

Before testing, first you need to confirm whether the ip/port used by the Forward service and Admin is the same as the ip/port used by the HelloWorld service, to avoid failure to run the service due to the same port.

Then, compile and test the program.

First, Run HelloWorld and execute the following commands in the framework directory. For details, please refer to [Quick start](quick_start.md).

```shell
./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml
```

Then open another terminal and execute `./run_server.sh` in the forward directory. If the execution is successful, you can see the following information.

```shell
Server InitializeRuntime use time:29(ms)
```

Then open another terminal and execute `./run_client.sh` in the forward directory. If the execution is successful, you can see the following information.

```shell
FLAGS_service_name: trpc.demo.forward.ForwardService
FLAGS_client_config: client/conf/trpc_cpp_fiber.yaml
get rsp success
FLAGS_service_name: trpc.demo.forward.ForwardService
FLAGS_client_config: client/conf/trpc_cpp_future.yaml
get rsp success
```

If you only use fiber for programming, you can skip the following chapters.

### Use future asynchronously call the backend service

#### configure future runtime

When using future, you need to set the future-related runtime options in the framework configuration file. You can choose to merge or separate threadmodel runtime. Here we take the separated threadmodel runtime as an example.

```shell
global:
  threadmodel:
    default:
      - instance_name: default_instance
        io_handle_type: separate
        io_thread_num: 2
        handle_thread_num: 6
```

For the detailed separate or merge runtime configuration, please refer to [Framework Configuration](framework_config_full.md).

#### Get the proto file of the backend service

You can refer to the steps corresponding to fiber.

#### Create the ServiceProxy of the backend service

You can refer to the steps corresponding to fiber.

#### Use ServiceProxy to call the backend service

The future asynchronous calling code is as follows:

```cpp
::trpc::Status ForwardServiceServiceImpl::Route(::trpc::ServerContextPtr context,
                                                const ::trpc::demo::forward::ForwardRequest* request,
                                                ::trpc::demo::forward::ForwardReply* reply) {
  TRPC_FMT_INFO("request msg: {}, client ip: {}", request->msg(), context->GetIp());

  // use asynchronous response mode
  context->SetResponse(false);

  auto client_context = ::trpc::MakeClientContext(context, greeter_proxy_);

  ::trpc::test::helloworld::HelloRequest hello_request;
  hello_request.set_msg(request->msg());

  ::trpc::test::helloworld::HelloReply route_reply;

  greeter_proxy_->AsyncSayHello(client_context, hello_request)
      .Then([context](::trpc::Future<::trpc::test::helloworld::HelloReply>&& fut) {
        ::trpc::Status status;
        ::trpc::demo::forward::ForwardReply forward_reply;

        if (fut.IsReady()) {
          std::string msg = fut.GetValue0().msg();
          forward_reply.set_msg(msg);
          TRPC_FMT_INFO("Invoke success, hello_reply:{}", msg);
        } else {
          auto exception = fut.GetException();
          status.SetErrorMessage(exception.what());
          status.SetFrameworkRetCode(exception.GetExceptionCode());
          TRPC_FMT_ERROR("Invoke failed, reason:{}", exception.what());
          forward_reply.set_msg(exception.what());
        }

        context->SendUnaryResponse(status, forward_reply);
        return ::trpc::MakeReadyFuture<>();
      });

  return ::trpc::kSuccStatus;
}
```

#### Run the test

Just follow the operations in the fiber-related chapters above. The only difference is that you need to modify the `run_server.sh` content in the forward directory.

Before modify

```shell
...
bazel-bin/demo/forward/forward --config=demo/forward/conf/trpc_cpp_fiber.yaml
...
```

After modify

```shell
bazel-bin/demo/forward/forward --config=demo/forward/conf/trpc_cpp.yaml
```

At this point, you have successfully used the framework fiber and future to access backend service.

## Next step

- Learn how tRPC-Cpp works through [tRPC-Cpp Architecture Design](architecture_design.md) and [Terminology](https://github.com/trpc-group/trpc/blob/main/docs/en/terminology.md).
- Read [User Guide](../README.md) to use tRPC-Cpp more comprehensively.
