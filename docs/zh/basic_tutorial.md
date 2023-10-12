[English](../en/basic_tutorial.md)

# 前言

本教程向C++程序员介绍如何使用tRPC-Cpp。

在[快速开始](quick_start.md)中，你已经成功地运行了 tRPC-Cpp HelloWorld。接下来，我们将展示一个请求代理转发服务的示例， 通过此示例，您将了解如何：

- 在 `.proto` 文件中定义服务。
- 使用trpc-cmdline工具生成服务端和客户端代码。
- 在tRPC-Cpp服务中使用同步和异步API访问后端服务。

请求转发服务（我们简称为Forward）的调用关系如下：

> Client <---> Forward <---> HelloWorld

- `HelloWorld`：快速开始中的服务代码
- `Forward`：接收Client的请求，然后调用 HelloWorld 的服务代码
- `Client`：客户端测试代码

# 环境搭建

在编译和运行本文中的 Forward 示例之前，您需要

1. 执行 [快速开始](quick_start.md) 中的开发环境安装和框架代码编译操作。
2. 安装 [trpc-cmdline](https://github.com/trpc-group/trpc-cmdline#install-from-release) 工具。

# 定义服务

我们的第一步是使用 [Protobuf](https://protobuf.dev/overview) 定义 tRPC-Cpp *服务* 以及方法 *请求* 和 *响应* 类型。

要定义服务，您可以在 `.proto` 文件定义一个具体名字的 `service`：

```protobuf
service Forward {
   ...
}
```

然后，您在 `service` 中定义 `rpc` 方法，指定它们的请求和响应类型。 tRPC-Cpp允许您可以定义一应一答（unary）的rpc方法和流式（stream）的rpc，这里只用到了一应一答（unary）的rpc方法，流式（stream）的rpc可以参考 [trpc stream](trpc_protocol_streaming_service.md) 。我们的 `forward.proto` 文件内容定义如下，它依赖了同级目录 `deps` 下的 `message.proto` 文件：

```protobuf
syntax = "proto3";

package trpc.demo.forward;

import "deps/message.proto";

service ForwardService {
  rpc Route (ForwardRequest) returns (ForwardReply) {}
}
```

在 `forward.proto` 同级目录下创建子目录 `deps`，并把 `forward.proto` 依赖的 `message.proto` 放置在该目录下

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

两个proto文件中，packge的名字方式建议参考 [terminology](https://github.com/trpc-group/trpc/blob/main/docs/zh/terminology.md)，这里package的命名我们使用三段式名字 trpc.{app}.{server} ，{app} 这里为demo，{server} 为forward。

# 基于 proto 文件生成代码

## 生成服务代码

首先通过执行下面命令创建一个目录, 用于随教程体验tRPC-Cpp的使用.

```shell
mkdir trpc-cpp-tutorial
cd trpc-cpp-tutorial
```

将 `forward.proto` 文件 和 `deps` 目录放到trpc-cpp-tutorial目录下，执行 `tree .` 可看到proto文件之间的位置关系如下：

```shell
.
├── deps
│   └── message.proto
└── forward.proto
```

如果您的proto文件之间有复杂的依赖关系，需要在一个目录下先自行组织好，才能用工具生成，比如有如下依赖关系：

```text
test3/test.proto ---> test2/test.proto --->
                                            mytest/helloworld.proto
                      test/test.proto  --->
```

您需要组织为：

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

在 trpc-cpp-tutorial 目录下执行命令：

```shell
trpc create -l cpp --protofile=forward.proto
# 上诉复杂依赖例子执行命令为: trpc create -l cpp --protofile=mytest/helloworld2.proto
```

其中 trpc 为你在环境搭建中安装的 trpc-cmdline工具，这个工具的两个参数分别是 -l 指定编程语言为cpp，--protofile 指定基于哪个proto文件进行生成。

命令运行成功后，会在trpc-cpp-tutorial下生成了一个项目，项目名字就是你在proto文件的名字，比如：我的server名是forward, 执行`tree forward`可以看到如下：

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

*注：生成项目编译工具使用bazel，暂不支持生成使用cmake编译的项目.*

其中 server 目录下的代码是此服务相关实现的代码，client 目录下的代码是用于本地测试服务的客户端代码，proto目录下的代码把带依赖关系的proto文件组织成了一个项目。

server 目录包含一个框架配置文件的目录、4个代码文件、一个 bazel BUILD 文件。

- 配置目录：里面包含两个框架配置文件，一个是fiber（M：N协程）的框架配置文件，表示当前的服务运行时会使用框架的fiber运行环境，业务写代码时会使用fiber相关的同步api，另一个是普通线程的框架配置文件，表示当前的服务运行时会使用框架的合并或者分离运行环境，业务写代码会使用异步future相关的api。**业务在写tRPC-Cpp的服务时，得先选择框架的一种运行环境**。fiber运行环境、合并或者分离运行环境的相关介绍和如何选择可以参考 [runtime](./runtime.md)。
- BUILD 文件：表示使用bazel规则来构建程序，其依赖了proto目录下forward.proto对应的相关bazel规则。
- 代码文件：包含两类文件，一类是服务进程级别的代码文件（server.h和server.cc），里面定义了一个ForwardServer类，它继承了框架的 `TrpcApp` 类，业务需要重载实现其 `Initialize` 和 `Destroy` 方法，完成服务进程在启动过程中业务相关的初始化操作，和在停止过程中的业务析构操作，比如注册Service具体实现；一类是Service级别的代码文件（service.h和service.cc），这两个文件是proto接口的具体实现。

forward 服务具体实现可以分成以下步骤：

1. 确定 forward 这个服务使用的运行环境（我们也称线程模型），并在框架配置的global配置项进行设置；
2. 定义一个进程级别的实现类（这里是ForwardServer），继承框架的 `TrpcApp` 类，并重载实现其`RegisterPlugins`、`Initialize` 和 `Destroy` 方法；
3. 定义一个Serice级别的实现类（这里是ForwardServiceServiceImpl），继承proto文件定义Service类，并重载实现其中的RPC方法，其中每个RPC方法，第一个参数是当次RPC执行的上下文 `ServerContext`；
4. 如有需要，在1中的 `RegisterPlugins` 方法中完成自定义插件的注册操作，比如：自定义协议Codec的注册；
5. 在1中的 `Initialize` 方法中完成进程级别的业务初始化操作；
   1. 先初始化依赖框架的业务相关逻辑操作，比如：拉取远程配置、创建和启动一个线程池;
   2. 业务相关的操作执行完后，一般最后执行Service的注册操作，这里是创建ForwardServiceServiceImpl对象，然后调用 `RegisterService` 将其注册ForwardServer中，注意ForwardServiceServiceImpl对象在进程中只会存在一个.
6. 在1中的 `Destroy` 方法中完成进程级别的业务析构操作；
   1. 这里一般停止在 `Initialize` 中创建的动态资源，比如：停止运行线程池;
7. 在程序的入口函数 `main` 定义进程级别实现类的实例，然后调用其 `Main` 和 `Wait`方法来启动程序；

client 目录包括一个框架配置文件的目录、2个代码文件、一个 bazel BUILD 文件。

- 配置目录：里面包含两个框架配置文件，一个是fiber（M：N协程）的框架配置文件，表示当前的测试程序会使用框架的fiber运行环境，业务写代码时会使用fiber相关的同步api，另一个是普通线程的框架配置文件，表示当前的测试程序会使用框架的合并或者分离运行环境，业务写代码会使用异步future相关的api。
- BUILD文件：表示使用bazel规则来构建程序，其依赖了proto目录下forward.proto对应的相关bazel规则。
- 代码文件：包含两个文件，一个是使用fiber同步调用forward服务的测试代码文件（fiber_client.cc）；一个是使用future异步调用forward服务的测试代码文件（future_client.cc），测试时，**选择其中一个即可**。

fiber_client 和 future_client 这两个测试代码整体的代码逻辑流程是一样的，具体可以分成以下步骤：

1. 解析和初始化框架配置；
2. 将业务的执行函数 `Run` 放到框架的runtime中执行；
   1. `Run` 函数会先创建访问后端服务的具体代理 `ServiceProxy`;
   2. 然后使用 `ServiceProxy` 调用后端服务的RPC接口，这里fiber和future的区别是一个是同步调用，一个是异步调用，另外注意每次调用RPC时，需要创建当次RPC调用的上下文 `ClientContext`.

proto 目录包含一个 bazel WORKSPACE 文件，一个 bazel BUILD 文件，两个 proto 文件

- WORKSPACE 文件：表示此 proto 目录是一个 bazel 项目，在 forward bazel 项目的 WORKSPACE 文件里，作为独立项目通过 `local_repository` 引入到 forward 项目中。
- BUILD 文件：使用bazel规则来组织管理 proto 文件，forward 项目的 server/client 的 BUILD 文件里，可通过 `@proto//:xxx_proto` 依赖对应 proto 文件生成的 C++ 桩代码。
- proto 文件：包含2个带依赖关系的 proto 文件，`forward.proto` 依赖 `deps/message.proto`，其文件组织位置关系，保持与trpc-cpp-tutorial 下一致。

## 测试

然后进入 forward 目录，编译和运行 forward 代码（第一次编译时间会稍长, 后续代码变更再编译会很快）。

```shell
./run_server.sh
```

forward 启动运行成功后，运行 forward 客户端测试程序

```shell
./run_client.sh
```

如果成功, 你将会看到如下的结果.

fiber_client 测试客户端的输出:

```shell
FLAGS_service_name: trpc.demo.forward.ForwardService
FLAGS_client_config: client/conf/trpc_cpp_fiber.yaml
get rsp success
```

future_client 测试客户端的输出:

```shell
FLAGS_service_name: trpc.demo.forward.ForwardService
FLAGS_client_config: client/conf/trpc_cpp_future.yaml
get rsp success
```

# 在 tRPC-Cpp 服务中访问后端服务

现在我们已经基于proto文件生成服务代码，并成功运行，接下来如何在 `tRPC-Cpp服务` 中访问后端服务 `HelloWorld`，这里的 `tRPC-Cpp服务` 是这个程序即做服务端，也做客户端，而不是上节中的 `future_client` 和 `future_client`程序，它们仅仅只做客户端。

## 在 tRPC-Cpp 服务中使用 fiber 同步调用后端服务

1. **确定服务使用的运行环境（线程模型）**

    使用fiber时，需要在框架的配置文件中设置fiber相关的运行环境配置，例如：./server/conf/trpc_cpp_fiber.yaml的fiber相关的global配置如下：

    ```yaml
    global:
      threadmodel:
        fiber:                               # 使用 Fiber(M:N协程) 线程模型
          - instance_name: fiber_instance    # 线程实例唯一标识，目前暂不支持配置多个Fiber线程实例
            concurrency_hint: 8              # 运行的Fiber Worker物理线程个数，选填，不设置的话自动设置为系统cpu的核数
    ```

    详细的fiber运行环境配置可以参考 [框架配置](./framework_config_full.md)。

2. **获取被调服务的 proto 文件**

    这里我们的被调服务是 tRPC-Cpp 的示例 helloworld 服务，因此我们要获取的 proto IDL 文件是 helloworld.proto。由于我们使用的bazel 构建工具，不需要将 helloworld.proto 复制一份到本地，我们只需要在使用 helloworld.proto 的 bazel 目标规则中，添加相关的依赖即可。

    代码文件 service.h 和 service.cc 依赖了 helloworld.proto，因此我们只需要 service 的目标构建规则中依赖 helloworld.proto 的目标。例如：./server/BUILD的service 构建目标如下：

    ```bzl
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

3. **创建被调服务的 ServiceProxy**

    在创建被调服务的 ServiceProxy 之前，需要定义好创建 ServiceProxy 的参数选项，这里我们通过配置的方式来设置，除了配置的方式之外，框架还支持通过代码、代码+配置来设置，具体可以看 [客户端开发指南](client_guide.md)。

    例如：./server/conf/trpc_cpp_fiber.yaml 里 client 相关的 ServiceProxy 参数配置如下：

    ```yaml
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

    这里在 forward 服务中我们使用 ip/port 直连的方式访问 helloworld 服务，target 是 helloworld 服务的 ip 和 port 信息。

    设置好 ServiceProxy 的参数选项后，接下来开始创建访问 helloworld 服务的 ServiceProxy。

    首先在 ./server/service.h 的 ForwardServiceServiceImpl 类里添加构造函数，并定义 GreeterServiceProxy 类型智能指针成员变量。

    ```cpp
    ...
    #include "examples/helloworld/helloworld.trpc.pb.h"
    ...
    using GreeterProxyPtr = std::shared_ptr<::trpc::test::helloworld::GreeterServiceProxy>;
    
    class ForwardServiceServiceImpl : public ::trpc::demo::forward::ForwardService {
     public:
      ForwardServiceServiceImpl();
     // ...
     private:
      GreeterProxyPtr greeter_proxy_;
    };
    // ...
    ```

    然后在 ./server/service.cc 的 ForwardServiceServiceImpl 构造函数中创建 GreeterServiceProxy。

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

    创建 GreeterServiceProxy 智能指针使用框架的 `::trpc::GetTrpcClient()` 的 `GetProxy` 接口。

    - **使用 ServiceProxy 调用被调服务**

       创建 GreeterServiceProxy 成功后，接下来在 ForwardServiceServiceImpl 的 Route 方法中使用 GreeterServiceProxy 调用 helloworld 服务。

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
       // ...
       ```

    - **运行测试**

       测试之前，最好先确认一下 forward 服务和 admin 使用的 ip/port 是否与 helloworld 服务使用的 ip/port 一样，以免因为端口一样导致运行服务失败。

       接下来开始程序的编译和测试。

       首先把 helloworld 运行起来，在框架的目录下执行以下命令，具体可以参考: [快速开始](quick_start.md)

       ```shell
       ./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml
       ```

       再打开一个终端，执行forward目录下的 `./run_server.sh`，如果执行成功可以看到如下信息。

       ```shell
       Server InitializeRuntime use time:29(ms)
       ```

       再打开一个终端，执行forward目录下的 `./run_client.sh`, 如果执行成功可以看到如下信息。

       ```shell
       FLAGS_service_name: trpc.demo.forward.ForwardService
       FLAGS_client_config: client/conf/trpc_cpp_fiber.yaml
       get rsp success
       FLAGS_service_name: trpc.demo.forward.ForwardService
       FLAGS_client_config: client/conf/trpc_cpp_future.yaml
       get rsp success
       ```

       如果你只需要使用框架fiber进行编程，可以跳过下面的章节。

## 在 tRPC-Cpp 服务中使用 future 异步调用后端服务

1. **确定服务使用的运行环境（线程模型）**

    使用 future 时，需要在框架的配置文件中设置 future 相关的运行环境配置，这个可以选择合并或者分离的线程模型运行环境，这里我们以分离的线程模型运行环境为例。

    ```yaml
    global:
      threadmodel:
        default:
          - instance_name: default_instance
            io_handle_type: separate
            io_thread_num: 2
            handle_thread_num: 6
    ```

    详细的合并和分离运行环境配置可以参考 [框架配置](framework_config_full.md)。

2. **获取被调服务的 proto 文件**

    按上面  fiber 相关章节中的内容操作即可。

3. **创建被调服务的 ServiceProxy**

    按上面  fiber 相关章节中的内容操作即可。

    - **使用 ServiceProxy 调用被调服务**

       future 异步的调用代码如下：

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

    - **运行测试**

       按上面fiber相关章节中的内容操作即可，唯一需要区别的地方是需要修改forward目录下的 `run_server.sh` 内容。

       修改前

       ```shell
       ...
       bazel-bin/demo/forward/forward --config=demo/forward/conf/trpc_cpp_fiber.yaml
       ...
       ```

       修改后

       ```shell
       bazel-bin/demo/forward/forward --config=demo/forward/conf/trpc_cpp.yaml
       ```

       到这里，你已经成功使用框架fiber和future进行访问后端服务了。

# 下一步

- 通过 [tRPC-Cpp架构设计](architecture_design.md) 和 [术语介绍](terminology.md) 学习 tRPC-Cpp 的工作原理。
- 阅读 [用户指南](../README.zh_CN.md) 来更全面的使用 tRPC-Cpp。
