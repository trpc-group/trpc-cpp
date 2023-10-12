[English](../en/quick_start.md)

# 前言

本文通过一个简单的代码示例帮助您开始了解和使用 tRPC-Cpp 框架。

# 开发环境搭建

在 C++ 世界中，没有普遍接受的标准来管理项目依赖项。 因此在构建和运行本文中的 Hello World 示例之前，您需要进行开发环境的安装，具体可以参考 [环境搭建](setup_env.md) 这篇文章。刚开始体验 tRPC-Cpp 框架 时建议使用 bazel, 本节中接下来的步骤主要介绍如何使用 bazel 编译和运行 Hello World 示例。

# 编译和运行 demo 示例

## 编译框架

从 github 下载框架源代码

```shell
git clone https://github.com/trpc-group/trpc-cpp
```

编译 tRPC-Cpp 框架代码

```shell
cd trpc-cpp
./build.sh
```

使用 bazel 编译第一次编译框架时会比较慢，可能需要等几分钟。编译的结果信息 bazel 会输出到终端，若编译成功，可以看到类似如下的信息:

```shell
INFO: Elapsed time: 279.100s, Critical Path: 48.83s
INFO: 2629 processes: 2629 processwrapper-sandbox.
INFO: Build completed successfully, 5459 total actions
```

若出现失败，可能是网络原因（无法拉取远程代码）、环境安装（gcc、框架依赖的库版本过低）等导致。

## 编译示例代码

tRPC-Cpp 框架代码编译成功后，接下来开始编译和运行 Hello World 示例。

执行如下的命令编译 Hello World 示例代码。

```shell
bazel build //examples/helloworld/...
```

## 运行示例代码

编译成功后，运行服务端程序。

```shell
./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml
```

打开另外一个终端，运行客户端测试程序。

```shell
./bazel-bin/examples/helloworld/test/fiber_client --client_config=./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
```

客户端测试程序运行成功后，可以看到如下的输出。

```shell
FLAGS_service_name:trpc.test.helloworld.Greeter
FLAGS_client_config:./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
get rsp msg: Hello, fiber
```

同时也可以查看本地目录服务端的日志 helloworld_fiber.log 文件。

```shell
cat helloworld_fiber.log 
```

可以看到如下的输出。

```shell
[2023-08-25 15:30:56.587] [default] [info] [greeter_service.cc:37] remote address: 127.0.0.1:46074
[2023-08-25 15:30:56.587] [default] [info] [greeter_service.cc:38] request message: fiber
```

到这里，恭喜您已经成功使用 tRPC-Cpp 框架运行了一个客户端-服务器应用程序。

# 更新服务实现

现在让我们看看如何添加一个新的 RPC 方法来更新给服务端程序以供客户端调用。

当前我们 tRPC-Cpp 框架提供的服务是使用 protobuf IDL 定义来实现的，服务器和客户端存根都有一个 SayHello() RPC 方法，该方法从客户端获取 HelloRequest 参数并从服务器返回 HelloReply，并且该方法的定义如下：

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

打开 examples/helloworld/helloworld.proto，并添加一个新的 SayHelloAgain() 方法，具有相同的请求和响应类型。

```protobuf
syntax = "proto3";
 
package trpc.test.helloworld;

service Greeter {
  rpc SayHello (HelloRequest) returns (HelloReply) {}

  rpc SayHelloAgain (HelloRequest) returns (HelloReply) {}
}

message HelloRequest {
  string msg = 1;
}

message HelloReply {
  string msg = 1;
}
```

## 然后更新服务端程序

打开 examples/helloworld/greeter_service.h，给 GreeterServiceImpl 添加 SayHelloAgain 方法。

```cpp
class GreeterServiceImpl : public ::trpc::test::helloworld::Greeter {
 public:
  ::trpc::Status SayHello(::trpc::ServerContextPtr context,
                          const ::trpc::test::helloworld::HelloRequest* request,
                          ::trpc::test::helloworld::HelloReply* reply) override;

  ::trpc::Status SayHelloAgain(::trpc::ServerContextPtr context,
                               const ::trpc::test::helloworld::HelloRequest* request,
                               ::trpc::test::helloworld::HelloReply* reply) override;
};
```

打开 examples/helloworld/greeter_service.cc，给 GreeterServiceImpl 添加 SayHelloAgain 方法的代码实现。

```cpp
::trpc::Status GreeterServiceImpl::SayHelloAgain(::trpc::ServerContextPtr context,
                                                 const ::trpc::test::helloworld::HelloRequest* request,
                                                 ::trpc::test::helloworld::HelloReply* reply) {
  // Your can access more information from rpc context, eg: remote ip and port
  TRPC_FMT_INFO("remote address: {}:{}", context->GetIp(), context->GetPort());
  TRPC_FMT_INFO("request message: {}", request->msg());

  std::string response = "Hello, ";
  response += request->msg();
  response += " Again";

  reply->set_msg(response);

  return ::trpc::kSuccStatus;
}
```

## 更新客户端调用代码

现在存根中提供了一个新的 SayHelloAgain() 方法。 我们将遵循与先前调用 SayHello() 的代码，实现一下调用 SayHelloAgain() 的函数，打开 examples/helloworld/test/fiber_client.cc，在 DoRpcCall 函数后面添加如下代码。

```cpp
int DoRpcCallAgain(const std::shared_ptr<::trpc::test::helloworld::GreeterServiceProxy>& proxy) {
  ::trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(proxy);
  ::trpc::test::helloworld::HelloRequest req;
  req.set_msg("fiber");
  ::trpc::test::helloworld::HelloReply rsp;
  ::trpc::Status status = proxy->SayHelloAgain(client_ctx, req, &rsp);
  if (!status.OK()) {
    std::cerr << "get again rpc error: " << status.ErrorMessage() << std::endl;
    return -1;
  }
  std::cout << "get again rsp msg: " << rsp.msg() << std::endl;
  return 0;
}
```

然后, 我们在 Run 函数中调用上面的 DoRpcCallAgain 函数，如下：

```cpp
int Run() {
  auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>(FLAGS_service_name);

  int ret = DoRpcCall(proxy);

  ret = DoRpcCallAgain(proxy);

  return ret;
}
```

## 运行验证

编译整个示例代码

```shell
bazel build //examples/helloworld/...
```

再次运行服务端程序

```shell
./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml
```

打开另外一个终端，再次运行客户端测试程序。

```shell
./bazel-bin/examples/helloworld/test/fiber_client --client_config=./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
```

将看到如下的输出

```shell
FLAGS_service_name:trpc.test.helloworld.Greeter
FLAGS_client_config:./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
get rsp msg: Hello, fiber
get again rsp msg: Hello, fiber Again
```

# 下一步

- 通过 [tRPC-Cpp 架构设计](architecture_design.md) 和 [术语介绍](https://github.com/trpc-group/trpc/blob/main/docs/zh/terminology.md) 学习 tRPC-Cpp 的工作原理。
- 阅读 [基础教程](basic_tutorial.md) 来开发tRPC-Cpp服务。
- 阅读 [用户指南](../README.md) 来更全面的使用 tRPC-Cpp。
