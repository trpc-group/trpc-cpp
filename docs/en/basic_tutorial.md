[中文](../zh/basic_tutorial.md)

## Overview

This tutorial provides basic introductions on how a C++ programmer can develop a service with tRPC-Cpp from scratch.

In [Quick Start](quick_start.md), you have successfully run HelloWorld demo. Next, we will lead you to implement a proxy service that forwarding requests to HelloWorld server. Through this example, you will learn how to:

- Define a service in a .proto file.
- Use trpc-cmdline tool to generate a project skeleton which contains server and client code.
- Use synchronous and asynchronous APIs to access services.

Below is the calling graph of the proxy service(we call it `Forward` for short) with others:

Client <---> Forward <---> HelloWorld

- `HelloWorld`: Service in [Quick Start](quick_start.md).
- `Forward`: Service that will receive the client's requests, and then forward it by sending RPC requests to the `HelloWorld` service.
- `Client`: Client that will send RPC requests to `Forward` service.

## Environment requirements

Before continue, you must make sure below environments are already satisfied:

1. Follow [Quick Start](quick_start.md) to install the development toolkits and compile `HelloWorld` successfully.
2. Install [trpc-cmdline](https://github.com/trpc-group/trpc-cmdline#install-trpc-cmdline) tools.
3. Create a empty directory using `mkdir trpc-cpp-quickstart` anywhere you want, all the opeartions after will ongoing here.

## Define the service

First, you should give service the definition about it's service name, methods, request/response message type of each method. Luckily, you can use protobuf IDL to easily do the work.

Create a subdirectory using `mkdir deps` under `trpc-cpp-quickstart` and then create a file named `message.proto`. Fill it with below request/response message type defition:

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

Create a file `forward.proto` under `trpc-cpp-quickstart` directory and fill it with a `service` named `Forward`:

```protobuf
service Forward {
   ...
}
```

Then, you can define `rpc` methods in `service`. tRPC-Cpp allows you to define unary and streaming rpc methods. But only unary rpc method is showed here, for stream method, you can refer to [trpc stream](trpc_protocol_streaming_service.md) for details. Add method named `Route` with request message type as `ForwardRequest` and response message type as `ForwardReply`. Pay attention that, the message types are defined in different proto file(`deps/message.proto`) and should be imported using `import` keyword. You will see the complete `forward.proto` file as below:

```protobuf
syntax = "proto3";

package trpc.demo.forward;

import "deps/message.proto";

service ForwardService {
  rpc Route (ForwardRequest) returns (ForwardReply) {}
}
```

For what the `package` defintion means in both proto files, we disscuss at [terminology](https://github.com/trpc-group/trpc/blob/main/docs/en/terminology.md). In conclusion, we suggest the three-part formula `trpc.{app}.{server}`. Here, `{app}` is demo, and `{server}` is `forward`.

Under `trpc-cpp-quickstart` and execute `tree .`, you will see below file structure:

```shell
.
├── deps
│   └── message.proto
└── forward.proto
```

If you have proto files with complex dependency, they must be organized at a directory before use trpc-cmdline tool(shown as below). For examples, if proto files have below dependency:

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

## Generate project code using proto file

### Generate project code

Now, in the `trpc-cpp-tutorial` directory, you can simply execute following command:

```shell
trpc create -l cpp --protofile=forward.proto
# The complex example methioned above should use command: trpc create -l cpp --protofile=mytest/helloworld2.proto
```

Among them, `trpc` is the trpc-cmdline tool you've installed at `Environment requirements` step, the two parameters of this tool are `-l` specifys that the tool should generate a tRPC-Cpp project(change to go will generate tRPC-Go project, etc) and `--protofile` specifys the tool should generate based on which proto file.

Once done, a tRPC-Cpp project based on `forward.proto` will be output under `trpc-cpp-tutorial`. It's directory name is the same as proto file. Execute command `tree forward` and you will see the following file structure:

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

*PS: the genearted project will use bazel to build. Genearting a project using cmake to build is not supported yet.*

Direcotry server contains service implementation code, client contains testing code that will send RPC requests to service, and proto contains code that manages proto file with it's dependency.

Direcotry server contains 1 configuration direcotry, 4 souce code files, and 1 bazel BUILD file.

- Configuration directory: There are two yaml files describing framework configuration here. One is configure using fiber (M:N coroutine), which means that the current service will use the fiber runtime, and developers should use synchronous api in their code. The other is configure using thread runtime, which means that the current service will use the merged or separated runtime, and should use future-promise based asynchronous api. **You must first choose a runtime environment before developing tRPC-Cpp services**. For how to select among different runtimes(fiber or separted/merge), please refer to [runtime](runtime.md) for details.
- BUILD file: Bazel will use it to Manage the building process.
- Source code files: There are two types of them. One is service process-level source code(server.h and server.cc). They define a class named ForwardServer, which inherits the `TrpcApp` class of tRPC-Cpp. Developers shoud override its' `Initialize` and `Destroy` methods. So during startup or terminate of process, user-defined initialization operations will be executed. The other type is the service-level source code(service .h and service.cc). They implement the user-defined logic of how service methods shoud work.

The implementation steps of the `Forward` service are:

1. Choose a runtime environment(we also call it the thread model), and fill it to global->threadmodel part of tRPC-Cpp configuration yaml file.
2. Define a process-level class (here ForwardServer) which must inherit the `TrpcApp` class of the tRPC-Cpp. Then override its `RegisterPlugins`, `Initialize` and `Destroy` methods as you need.
3. Define a service-level class (here ForwardServiceServiceImpl) which must inherit the auto-genearted service class defined by the proto file. Then override its' RPC methods, where the first parameter `ServerContext` of each methods is the context of the current RPC execution and the second/third parameter represents request/response instance.
4. If needed, registry user-defined plugin you want to use at `RegisterPlugins` mentioned in 2(eg: register the custom protocol `Codec`).
5. Complete the process-level business-related initialization at `Initialize` mentioned in 2.
    1. First initialize the business-related logical operations that depend on the framework, such as: pull a remote configuration, create and start a thread pool.
    2. After the business-related operations are completed, registry service. Here, the ForwardServiceServiceImpl instance is created, and then registered into ForwardServer by invoke `RegisterService` function. Note that there will be only one ForwardServiceServiceImpl instance in the process.
6. Complete the process-level business destruction operation in the `Destroy` method mentioned in 2.
    1. Generally, you should stop and destroy dynamic resources created in `Initialize`, for example: stop running the thread pool.
7. Use process-level implementation class at program entrypoint `main`. Invoking its' `Main` and `Wait` functions to start the program.

Directory client contains 1 configuration direcotry, 2 souce code files, and 1 bazel BUILD file.

- Configuration directory: There are two yaml files describing framework configuration here. One is configure using fiber (M:N coroutine), which means that the current service will use the fiber runtime, and developers should use synchronous api in their code. The other is configure using thread runtime, which means that the current service will use the merged or separated runtime, and should use future-promise based asynchronous api.
- BUILD file: Bazel will use it to Manage the building process.
- Source code files: Two are act as RPC clients. One will use synchronous rpc call to access Forward service (fiber_client.cc), The other will use future-promise based asynchronous rpc call(future_client.cc). **choose one of them in your test**.

The implemention of these two clients is similar. We divide it into following steps:

1. Parse configuration yaml file and use it to initialize tRPC-Cpp runtime.
2. Put `Run` function into the runtime environment where the business logic will be executed. As the implementation of `Run`:
    1. `ServiceProxy` will be created first using configuration at yaml(client->service). This proxy will in charge of handing all the RPC invoke between client and conresponding backend service.
    2. Then, use `ServiceProxy` to send RPC request. The difference between fiber_client and future_client is that one uses a synchronous call and the other uses an asynchronous call. In addition, you should always create `one ClientContext for each RPC call`.

Directory proto contains a bazel WORKSPACE file, a bazel BUILD file, and 2 proto files.

- WORKSPACE file: Indicate the proto directory should be viewed as an independent bazel project. If you check WORKSPACE file in project root directory, you will find the proto project is imported via `local_repository`.
- BUILD file: Use bazel rules to manage proto files which may contain complex dependency，it can be imported by `@proto//:xxx_proto` at BUILD files of server/client in forward project. Relatvie stub code files will be generated after build. Service should inherit the generated class to implment bussiness logic per RPC methods(eg. ForwardServiceServiceImpl). Client can use the generated proxy to do RPC call.
- proto files: Contain 2 proto files: `forward.proto` depends on `deps/message.proto`. Their file structure at `trpc-cpp-quickstart` will be keeped at proto directory.

### Test

Enter the forward directory，compile and run the `Forward` service:

```shell
./run_server.sh
```

*PS: It will take a minutes to do the progress for the first time.*

Then complie and run fiber_client and future_client respectively:

```shell
./run_client.sh
```

If success, you will see the following results:

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

Now that you have a successfull running `Forward` service. But how to let it access the backend service `HelloWorld` we got in [Quick Start](quick_start.md)? The `Forward` program serves as both server and client. As for the `future_client` and `future_client` programs, they are only clients.

### Use fiber do synchronous call to the backend service

#### configure fiber runtime

Add fiber runtime configuration. Open file `./server/conf/trpc_cpp_fiber.yaml`, you can check global->threadmodel part of it as below:

```yaml
global:
  threadmodel:
    fiber:                              # Use fiber threadmodel
      - instance_name: fiber_instance   # ThreadModel instance unique identifier, currently does not support configuring multiple Fiber instances
        concurrency_hint: 8             # Number of physical threads running Fiber Workers, it is recommended to configure it as the same number as machine available cores.
```

For the full fiber runtime configuration, please refer to [Framework Configuration](framework_config_full.md).

#### Get the proto file of the backend service

Here our backend service is the `HelloWorld` service, so the proto IDL file we want to obtain is helloworld.proto. Thanks to bazel, instead of copying, we can easily get it via bazel remote target.

Add remote bazel target in deps of file `./server/BUILD` as follows:

```shell
cc_library(
    name = "service",
    srcs = ["service.cc"],
    hdrs = ["service.h"],
    deps = [
        "@proto//:forward_proto",
        "@trpc_cpp//examples/helloworld:helloworld_proto", # add this line
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/log:trpc_log",
    ],
)
```

#### Create the ServiceProxy of the backend service

Before creating the ServiceProxy of the backend service, you need to define the it's options. Here we demonstrate setting them through configuration. Besides, tRPC-Cpp also supports through only code, code + configuration, for details, refer to [Client Development Guide](client_guide.md).

Add below proxy options in configuration file `./server/conf/trpc_cpp_fiber.yaml`:

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

By the configuration, `Forward` service will use target(ip:port) to access the `HelloWorld` service.

After setting the options of the ServiceProxy, then create and use the ServiceProxy to access the helloworld service.

First, add a constructor to the ForwardServiceServiceImpl class in file `./server/service.h`, and define a GreeterServiceProxy type smart pointer member variable.

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

Then create GreeterServiceProxy at the ForwardServiceServiceImpl constructor in file `./server/service.cc`.

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

Create a GreeterServiceProxy smart pointer using `::trpc::GetTrpcClient()->GetProxy`.

#### Use ServiceProxy to call the backend service

After creating GreeterServiceProxy, use it in the Route method implemention of ForwardServiceServiceImpl to call the `HelloWorld` service.

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

Before testing, to avoid failure due to service listen on the same port, you need to confirm whether the ip/port used by the `Forward` service and admin(configure using `admin_ip`/`admin_port` in yaml file) is different from the `HelloWorld` service.

Then, compile and test the program.

First, Run `HelloWorld` and execute the following commands in the trpc-cpp directory. For details, please refer to [Quick Start](quick_start.md).

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

If you only use fiber for programming, you can skip the following sections.

### Use future do asynchronous call to the backend service

#### configure future runtime

Add future runtime configuration. Open file `./server/conf/trpc_cpp.yaml`, you can check global->threadmodel part of it as below:

```shell
global:
  threadmodel:
    default:
      - instance_name: default_instance
        io_handle_type: separate
        io_thread_num: 2
        handle_thread_num: 6
```

You can choose to merge or separate threadmodel runtime. Here we take the separate threadmodel runtime as an example.

For detailed separate or merge runtime configuration, please refer to [Framework Configuration](framework_config_full.md).

#### Get the proto file of the backend service

You can refer to the corresponding section in fiber.

#### Create the ServiceProxy of the backend service

You can refer to the corresponding section in fiber.

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

Just follow the operations in the fiber-related sections above. The only difference is that you need to modify the `run_server.sh` content in the forward directory.

Before modify:

```shell
...
bazel-bin/demo/forward/forward --config=demo/forward/conf/trpc_cpp_fiber.yaml
...
```

After modify:

```shell
bazel-bin/demo/forward/forward --config=demo/forward/conf/trpc_cpp.yaml
```

At this point, you have successfully used fiber and future in tRPC-Cpp to access backend service.

## Next step

- Learn how tRPC-Cpp works through [tRPC-Cpp Architecture Design](architecture_design.md).
- Read [User Guide](../README.md) to use tRPC-Cpp more comprehensively.
