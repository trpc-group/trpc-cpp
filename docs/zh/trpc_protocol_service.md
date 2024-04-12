[English](../en/trpc_protocol_service.md)

# 前言

相比较于 [tRPC-Cpp快速上手](./quick_start.md) ，此文档更专注于tRPC协议的服务端开发，内容更全面详实。通过此篇文章开发者可以了解到

- 使用 Protobuf 的 IDL 文件生成 tRPC 桩代码方式
- tRPC 协议服务端开发基本流程
- 开启使用服务端插件
- 框架的一些特性，例如初始化服务时设置一些回调

所以在行文过程中以 tRPC 协议为例，串联起完整的服务端程序开发流程；其他协议服务端开发文档同此文档形式上是平行关系，内容上重点介绍对应协议的特性。

更完善的 tRPC 协议参考[trpc 协议设计](trpc_protocol_design.md)，tRPC 协议是默认协议，实际使用中也推荐使用 tRPC 协议。

# 基于 Protobuf 生成服务桩代码

本章介绍基于 Protobuf 文件生成桩代码的流程，若用户无需 Protobuf 文件，可以忽略该章节

## 编写 Protobuf IDL 文件

这里以[helloworld.proto](../../examples/helloworld/helloworld.proto)为例

```pb
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

该文件定义了一个`Greeter`的`Service`，其中`Greeter`中有一个`SayHello`的 `rpc Method`；`SayHello`接收一个`HelloRequest`类型的请求，返回`HelloReply`类型的响应；`HelloRequest`和`HelloReply`都有个`msg`字符串类型的字段（并非要求请求和回复的字段名称必须相同）。

除此之外，还有几点需要注意一下

- `syntax`建议使用`proto3`，tRPC 都是默认基于 proto3 的，当然也支持 proto2。
- `package`内容格式建议为`trpc.{app}.{server}`，`app`为你的应用名，`server`为你的服务进程名，脚手架生成工具将会解析 Protobuf 文件的`app`和`server`，用于生成项目，上面`helloworld.proto`文件建议自己定义`app`和`server`的名字，方便服务的部署
- 定义`rpc`方法时，一个`server`（服务进程）可以有多个`service`（对`rpc`逻辑分组），一般是一个`server`一个`service`，一个`service`中可以有多个 `rpc` 调用。
- 编写 Protobuf 时必须遵循[谷歌官方规范](https://protobuf.dev/programming-guides/style/)。

上述定义了一个标准的 Protobuf 协议的 IDL 文件，接下来构建项目代码。

## 根据 Protobuf IDL 文件构建项目代码

若用户已经构建了项目，则本节可以直接跳过，直接查看下节。
为了方便用户构建项目，框架提供基于 Protobuf 的 IDL 文件快速构建项目的trpc-cmdline工具，这时执行下面命令:
```sh
# 其中trpc为你在环境搭建中安装的trpc-cmdline工具，-l指定language为cpp语言，protofile为用于生成桩代码的proto文件
trpc create -l cpp --protofile=helloworld.proto
```

命令执行后，会在当前目录下生成`helloworld`项目，其目录结构如下：

```txt
.
├── build.sh
├── clean.sh
├── client
│   ├── BUILD
│   ├── conf
│   │   ├── trpc_cpp_fiber.yaml
│   │   └── trpc_cpp_future.yaml
│   ├── fiber_client.cc
│   └── future_client.cc
├── proto
│   ├── BUILD
│   ├── helloworld.proto
│   └── WORKSPACE
├── README.md
├── run_client.sh
├── run_server.sh
├── server
│   ├── BUILD
│   ├── conf
│   │   ├── trpc_cpp_fiber.yaml
│   │   └── trpc_cpp.yaml
│   ├── server.cc
│   ├── server.h
│   ├── service.cc
│   └── service.h
└── WORKSPACE
```

介绍一下该项目目录：

- build.sh 和 clean.sh表示构建和清理项目；run_client.sh 和 run_server.sh表示启动客户端和服务端测试
- 其中server目录下的代码是此服务相关实现的代码，client目录下的代码是用于本地测试服务的客户端代码
- `WORKSPACE` 用于`bazel`编译所需工作区配置内容如下

  ```bzl
  load('@bazel_tools//tools/build_defs/repo:git.bzl', 'git_repository')
  git_repository(
      name = "trpc_cpp",
      tag = "{trpc_ver}", # 指定版本, 一般推荐最新的 tag
      remote = "{trpc_cpp_addr}", # 指定 trpc—cpp 的git地址
      # for example
      # tag = "v1.0.0",
      # remote = "https://github.com/trpc-group/trpc-cpp.git",
  )
  load("@trpc_cpp//trpc:workspace.bzl", "trpc_workspace")

  trpc_workspace()
  ```

  这里必须执行 `trpc_workspace` 表示加载tRPC-Cpp框架加载所有必须得第三方库

## 生成IDL文件对应的tRPC桩代码

框架支持`bazel`和`cmake`两种编译方式，推荐使用`bazel`编译方式；`bazel`编译要求每个目录下必须存在`BUILD`文件，用于设置编译规则。

### 编写BUILD文件

为了方便讲解，这里以上节生成项目中的`test/helloworld`目录下的`BUILD`文件为例介绍（若是自己创建的项目，也需要采用以下方式编写BUILD文件），如下：

```bzl
load("@trpc_cpp//trpc:trpc.bzl", "trpc_proto_library")

# ...

trpc_proto_library(
    name = "helloworld_proto",
    srcs = ["helloworld.proto"],
    use_trpc_plugin=True,
    rootpath="@trpc_cpp",
    # 依赖其他的protobuf，若是没有依赖可以无需写 deps
    deps = [
        ":deps_proto",
    ],
)

# 被依赖 protobuf 的编译规则，若是srcs为空，可以不写；这里是写是方便理解，不影响使用
trpc_proto_library(
    name = "deps_proto",
    srcs = [], 
)

# 服务编译规则，依赖上面的 helloworld_proto
cc_library(
    name = "greeter_service",
    srcs = ["greeter_service.cc"],
    hdrs = ["greeter_service.h"],
    deps = [ 
        "//test/helloworld:helloworld_proto",
        # ...
    ],
)
# ...
```

需要注意的点如下：

- load函数(line1)加载桩代码生成规则**trpc_proto_library**，这里的`@trpc_cpp`表示当前项目**远程依赖trpc-cpp仓库**，在``更多细节参考：[bazel build](https://bazel.build/?hl=zh-cn)

- trpc_proto_library参数：
    `use_trpc_plugin`: 必须是**True**，表示生成服务桩代码，即会生成带有 **.trpc.h/.trpc.cc**后缀的文件
    `rootpath`: 表示该项目**远程依赖trpc-cpp仓库**，即是 `WORKSPACE` 中的拉取的仓库的名称
    `deps`: 表示依赖的其他的 protobuf 文件，若没有可以不写

### 编译生成桩代码

```sh
# bazel build your_trpc_pb_target，这里以helloworld为例
bazel build test/helloworld:helloworld_proto
```

将自动在 **bazel-bin/test/helloworld/** 目录下生成对应的桩代码，生成文件如下：
![trpc_protocol_service_stub](../images/trpc_protocol_service_stub_zh.png)
感兴趣的可以看下生成桩代码的服务端实现(helloworld.trpc.pb.h/helloworld.trpc.pb.cc文件);当然，既然能够方便地生成桩代码，那么接下来把关注点放到需要实现的业务逻辑。

# 编写服务代码

框架编写服务端代码的基本流程如下：

**step 1 实现具体协议的 Service**
**step 2 实现主程序类 Server**
**step 3 创建并启动主程序类 Server**
**step 4 提供配置并启动**

整个流程在`step 1` 实现具体协议`Service` 和`step 4`提供对应配置时，会根据实际协议进行特化，下面会介绍基于桩代码编写服务流程，以前面生成的桩代码项目`helloworld`为例（若用户无需 Protobuf，不能生成桩代码，则可以参考例子：[trpc_noop](../../examples/features/trpc_noop/server/demo_server.cc) 编写服务端）

## 实现具体协议的 Service

上述会生成两种服务端API，分别为同步形式和异步形式，也即`trpc::test::helloworld::Greeter`和`trpc::test::helloworld::AsyncGreeter`；下面会分别介绍这两种方式以及其接口实现中上下文`::trpc::ServerContextPtr`的使用。

### 同步 API

首先引入需要的`helloworld.trpc.pb.h`头文件（见第1行），并继承桩代码中生成的`Greeter`类和以`override`方式声明要重写`SayHello`函数

```c++
#include "test/helloworld/helloworld.trpc.pb.h"

// ... 省略部分代码
class GreeterServiceImpl : public ::trpc::test::helloworld::Greeter {
public:
  ::trpc::Status SayHello(::trpc::ServerContextPtr context,
                          const ::trpc::test::helloworld::HelloRequest* request,
                          ::trpc::test::helloworld::HelloReply* reply) override {
    // Implement business logic here
    TRPC_FMT_INFO("got req");
    std::string hello("hello");
    reply->set_msg(hello + request->msg());
    // Implement business logic end

    return ::trpc::kSuccStatus;
  }
};
```

`SayHello`方法是 Protobuf 文件中定义的 rpc 方法名称；若有多个方法，这里也会生成多个，用户根据业务需求改写各个方法；从继承上可以看出其父类是`::trpc::test::helloworld::Greeter`(定义在**bazel-bin/test/helloworld/helloworld.trpc.pb.h**)，继承于`::trpc::RpcServiceImpl`（参考：[rpc_service_impl](../../trpc/server/rpc/rpc_service_impl.h)）；所以最终可以追溯`GreeterServiceImpl`属于`::trpc::Service`（参考：[service](../../trpc/server/service.h)）的子类。
上述例子中是直接在`SayHello`中设置`reply`的值，属于同步返回回复；若用户希望后续自行回复客户端，则可使用**异步回复**，请参考：[service 异步回包](./server_guide.md#异步回包)

### 异步 API

类似同步 API，需要先引入`helloworld.trpc.pb.h`头文件（见第1行）

```c++
#include "test/helloworld/helloworld.trpc.pb.h"

// ... 省略部分代码
class AsyncGreeterServiceImpl : public ::trpc::test::helloworld::AsyncGreeter {
public:
  ::trpc::Future<::trpc::test::helloworld::HelloReply> SayHello(const ::trpc::ServerContextPtr& context, 
                                                                const ::trpc::test::helloworld::HelloRequest* request) override {
    // Implement business logic here
    TRPC_FMT_INFO("got req");
    trpc::test::helloworld::HelloReply rsp;
    rsp.msg("FutureResponse");
    // Implement business logic end

    return ::trpc::MakeReadyFuture<HelloReply>(std::move(rsp));
  }
};
```

不同于同步接口，异步 `SayHello`接口返回的是 `::trpc::Future` 模版类；`::trpc::test::helloworld::AsyncGreeter`最终父类也是`::trpc::Service`

### Service 接口中的上下文使用

以上述 `SayHello`接口为例，可以看到一个关键参数`context`，它属于连接整个请求处理过程中的上下文，通过该参数能获取到复杂业务场景处理需要的信息，下面给出几种常用方式

#### 通过上下文ServerContext获取信息

通过`context`获取各种信息(主调ip端口、请求id等)，参考：[ServerContext](../../trpc/server/server_context.h)，伪代码如下：

```c++
class GreeterServiceImpl : public ::trpc::test::helloworld::Greeter {
public:
  ::trpc::Status SayHello(::trpc::ServerContextPtr context,
                          const ::trpc::test::helloworld::HelloRequest* request,
                          ::trpc::test::helloworld::HelloReply* reply) override {
    // 通过ServerContext获取信息
    TRPC_LOG_INFO("remote address:" << context->GetIp() << ":" << context->GetPort());
    TRPC_LOG_INFO("request id:" << context->GetRequestId());
    // ...
    return ::trpc::kSuccStatus;
  }
 // ... omit some code
};
```

#### 服务端链路数据透传

若用户希望在`SayHello`中将需要服务要透传的数据传递给客户端，可以参考伪代码如下:

```c++
class GreeterServiceImpl : public ::trpc::test::helloworld::Greeter {
public:
  ::trpc::Status SayHello(::trpc::ServerContextPtr context,
                          const ::trpc::test::helloworld::HelloRequest* request,
                          ::trpc::test::helloworld::HelloReply* reply) override {
    // 通过ServerContext->AddRspTransInfo 接口透传数据给客户端
    context->AddRspTransInfo("key3", "value3");
    // ...
    return ::trpc::kSuccStatus;
  }
 // ... omit some code
};
```

#### 处理服务端一个端口同时监听tcp和udp

如果需要此特性，可以在`SayHello` 根据 `ServerContext->GetNetType()`获取网络类型，伪代码如下：

```c++
class GreeterServiceImpl : public ::trpc::test::helloworld::Greeter {
public:
  ::trpc::Status SayHello(::trpc::ServerContextPtr context,
                          const ::trpc::test::helloworld::HelloRequest* request,
                          ::trpc::test::helloworld::HelloReply* reply) override {
    if (context->GetNetType() == ::trpc::ServerContext::NetType::kUdp) {
      TRPC_LOG_INFO("udp request");
    } else if (context->GetNetType() == ::trpc::ServerContext::NetType::kTcp) {
      TRPC_LOG_INFO("tcp request");
    }
    reply->set_rsp_msg("receive");
    // ...
    return ::trpc::kSuccStatus;
  }
 // ... omit some code
};
```

## 实现主程序类 Server

主程序类继承`trpc::TrpcApp`，并重写需要的方法：如`Initialize`、`Destroy`、`RegisterPlugins`等（参考：[trpc_app](../../trpc/common/trpc_app.h)）；其中基础用法为重写`Initialize`，并在`Initialize`实现具体`Service`的注册即可，代码如下：

```c++
// ... 省略部分代码
int HelloworldServer::Initialize() override {
  const auto& config = ::trpc::TrpcConfig::GetInstance()->GetServerConfig();
  // Set the service name, which must be the same as the value of the `/server/service/name` configuration item
  // in the configuration file, otherwise the framework cannot receive requests normally.
  std::string service_name1 = fmt::format("{}.{}.{}.{}", "trpc", config.app, config.server, "Greeter");

  TRPC_FMT_INFO("service name1:{}", service_name1);

  ::trpc::ServicePtr my_service1(std::make_shared<GreeterServiceImpl>());
  RegisterService(service_name1, my_service1);

  return 0;
}
```

通过接口`RegisterService(service_name1, my_service1)`注册服务，其中**service_name1** 为`::trpc::Service`名称，**my_service1**为具体的`::trpc::Service`实例。大部分场景下，重写`Initialize`，并在`Initialize`实现具体`Service`的注册即可。除此之外，还可以初始化一些特殊场景下的逻辑，下面分别介绍一下。

### Initialize 中拉取业务配置

若用户希望可以拉取远程配置等，可参考伪代码如下：

``` c++
// 业务初始化
int HelloworldServer::Initialize() override {
    // 远程拉取配置
    LoadRemoteConfig(...); // 该接口属于伪代码，框架无此接口
    
    // 注册服务
    RegistryService(service_name, greeter_service);
    
    return 0;
}
```

### Initialize 中实现预热逻辑

若用户希望同时如果需要在`Service`注册之前做一些预热，可以`Initialize`阶段处理预热逻辑，伪代码如下：

```c++
int HelloworldServer::Initialize() override {
    // 先执行业务预热的代码，如加载比较大的数据等
    DoPreHotWork(); // 该接口属于伪代码，框架无此接口
    
    // 上述预热工作做完之后，再注册服务
    RegistryService(service_name, greeter_service);
    
    return 0;
}
```

### Initialize 中注册自定义回调

框架提供自定义回调特性，如下：

- 用户注册的请求到处理线程的分发方法
- 用户自定义接收连接的回调
- 用户自定义握手连接建立的回调
- ...

这些回调的设置也可以在`Initialize`阶段完成完成，下面展示特定请求由固定线程处理为例，伪代码如下（参考：[request_dispatcher](../../examples/features/request_dispatch/server/demo_server.cc)）：

```c++
int Initialize() override {
  const auto& config = ::trpc::TrpcConfig::GetInstance()->GetServerConfig();
  // Set the service name, which must be the same as the value of the `/server/service/name` configuration item
  // in the configuration file, otherwise the framework cannot receive requests normally.
  std::string service_name = fmt::format("{}.{}.{}.{}", "trpc", config.app, config.server, "Greeter");
  TRPC_FMT_INFO("service name:{}", service_name);

  trpc::ServicePtr service(std::make_shared<GreeterServiceImpl>());

  service->SetHandleRequestDispatcherFunction(DispatchRequest);

  RegisterService(service_name, service);

  return 0;
}

```

### Initialize 中注册自定义管理命令

还可以在Initialize阶段自定义管理命令，伪代码如下：

```c++
// MyAdminHandler实现
class MyAdminHandler : public trpc::AdminHandlerBase {
 public:
  MyAdminHandler() { description_ = "This is my own command"; }
  void CommandHandle(trpc::http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override {
    result.AddMember("message", "this is just a test handler", alloc);
  }
};

int Initialize() override {
    RegisterCmd(trpc::http::OperationType::GET, "/myhandler", new MyAdminHandler);
    
    // 其他工作，如RegistryService等    
    return 0;
}

```

## 创建并启动主程序类 Server

 ```c++
// ... 省略部分代码
int main(int argc, char** argv) {
   test::helloworld::HelloworldServer helloworld_server;
   helloworld_server.Main(argc, argv);
   helloworld_server.Wait();

   return 0;
 }
 ```

在`main`函数中实例化`HelloWorldServer`对象用于服务启动。服务端编写完成之后，编译生成服务端程序；编译整个项目的方式可以使用脚本`build.sh`， 也可以直接运行

 ```sh
bazel build ...
 ```

## 提供配置并启动

运行服务可以使用`run_server.sh`脚本，也可以直接运行

 ```sh
bazel-bin/test/helloworld/helloworld --config=test/helloworld/conf/trpc_cpp_fiber.yaml
 ```

在桩代码项目`helloworld`的路径`test/helloworld/conf`下提供两种类型的配置，可以任选一种使用，这里选择了`trpc_cpp_fiber.yaml`，下面简单介绍一下配置需要注意的地方

### 服务配置

```yaml
server:
  app: test
  server: helloworld
  admin_port: 6666
  admin_ip: 0.0.0.0
  service:
    - name: trpc.test.helloworld.Greeter
      protocol: trpc
      network: tcp 
      ip: 0.0.0.0
      port: 54321
```

业务实现服务注册的接口`RegisterService(service_name, service)`中的`service_name`需要与这里的**server-service-name**（即`trpc.test.helloworld.Greeter`）命名保持一致，第一个字段默认为`trpc`，第二、三个字段为上边的`app`(**第2行**)和`server`(**第3行**)配置，第四个字段为用户定义的服务名（即本文提到的`Greeter`）

### 插件配置

框架以插件方式对接多种服务治理平台，例如：metrics（监控）、log（日志）、telemetry（遥测）等。框架插件常常配合[filter](./filter.md)使用，所以需要在`server`配置`filter`(每一个拦截器本身会对性能有一定程度的损失，尽量只配置自己需要的拦截器)，配置示例如下：

```yaml
server:
  app: test
  server: helloworld
  service:
    - name: trpc.test.helloworld.Greeter
      protocol: trpc
      network: tcp 
      ip: 0.0.0.0
      port: 54321
      filter:
        - prometheus
        - tpstelemetry
plugins:
  log:
    default:
      - name: default
        sinks:
          local_file:
            filename: trpc_fiber_server.log
  metrics:
    prometheus:
      histogram_module_cfg:
        - 1
        - 5
        - 10
      const_labels:
        const_key1: const_value1
        const_key2: const_value2
  telemetry:
    tpstelemetry:
      addr: 127.0.0.1:20001
      protocol: http
      tenant_id: default
      report_req_rsp: true
      sampler:
        fraction: 0
      metrics:
        enabled: true
        registry_endpoints: ["127.0.0.1:20002"]
      logs:
        enabled: true
        level: "info"
```

从配置可以看出，同一类型下的插件可以有不同种类；例如：上面列出的`metrics`的插件是`prometheus`，若还有其他类型的监控插件也可以配置在`metrics`字段下。另外；插件生效前提必须在`service`下的`filter`字段（第10行）配置对应的插件名称。另外，这里需要注意一点，日志插件比较特殊，没有在`filter`中配置；主要是因为日志属于用户自己主动调用接口，不属于`filter`被动埋点方式工作。更多配置相关的介绍参考：[framework_config_full](./framework_config_full.md)

# FAQ

## 代码中RegistryService未在配置文件中配置正确，启动时会直接Aborted掉

服务启动失败，直接Aborted，此时在日志文件中有如下打印：

```txt
[2021-02-27 15:14:51.398] [thread 23865] [error] [trpc/server/trpc_server.cc:168] service_name:trpc.test.helloworld.Greeter not found.
[2021-02-27 15:14:51.398] [thread 23865] [critical] [trpc/server/trpc_server.cc:169] assertion failed: service_adapter_it != service_adapters_.end()
```

目前服务端要求注册的service一定要在配置文件中配置正确，否则提示启动失败。
