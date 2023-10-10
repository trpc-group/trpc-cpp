[English](../en/server_guide.md

# 服务端开发向导

[快速入门](quick_start.md)中介绍了如何开发一个简单的 tRPC 服务。本文是进阶文章，将详细介绍进行服务端程序开发时需要考虑哪些问题，做哪些事情。如果服务中需要调用下游，阅读完本文后，请阅读[客户端开发向导](client_guide.md)。

## Runtime选型

请参考[如何选择及配置runtime](runtime.md#how-to-choose-and-configure-the-runtime)。

## 定义Service

service 即服务提供者，提供接口规范供客户端调用。从网络通信的角度讲, 每个service对应一个IP + 端口 + 协议。

### 协议选择

目前框架支持的内置协议有tRPC、HTTP、gRPC、Redis等。如果你使用的协议不属于框架内置协议的话，则需要自行实现对应的codec插件。

在对协议无特殊要求的场景，推荐使用tRPC协议。因为tRPC协议相比其他协议，支持的功能更丰富，如流式传输、附件分发等，性能方面也做了针对性的优化，且未来新特性我们会优先在tRPC协议上实现。

### 定义Proto Service

Proto Service是一组接口的逻辑组合，它需要定义package，proto service，rpc name以及接口请求和响应的数据类型。

#### IDL协议类型

[IDL](https://en.wikipedia.org/wiki/Interface_description_language)语言可以通过一种独立于编程语言的方式来描述接口，并使用工具把IDL文件转换成指定语言的桩代码，使程序员专注于业务逻辑开发。对于IDL协议类型的服务，Proto Service的定义通常分为以下五步（以tRPC协议为例）：

Step 1：采用IDL语言描述RPC接口规范

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

Step 2：通过脚手架生成项目

参考[快速上手](quick_start.md)中的步骤

Step 3：实现服务端逻辑

参考[快速上手](quick_start.md)中的步骤

Step 4：将Proto Service注册到Server

生成的桩代码中，已经做了这一步，具体代码位于"helloworld_server.cc"中：

```cpp
int helloworldServer::Initialize() {
 
  std::string service_name("trpc.");
  service_name += trpc::TrpcConfig::GetInstance()->GetServerConfig().app;
  service_name += ".";
  service_name += trpc::TrpcConfig::GetInstance()->GetServerConfig().server;
  service_name += ".Greeter";
 
  TRPC_LOG_INFO("service name1:" << service_name);
 
  trpc::ServicePtr my_service(new GreeterServiceImpl());
 
  // service_name需和配置文件中的service/name对应，以便关联service配置
  RegisterService(service_name, my_service);
 
  return 0;
}
```

接下来用户需要实现greeter_service.cc中具体的接口，可参考[快速上手](quick_start.md)中的步骤。

#### 非 IDL 协议类型

其中常见的协议为http协议，详细介绍可参考[HTTP 服务开发指南](http_protocol_service.md)。

## 提供对应的框架配置文件

作为服务端，框架配置文件中需要提供"global"及"server"两部分的配置，plugins部分根据所使用的的插件进行配置。

一个使用fiber m:n协程runtime的配置示例如下:

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

## 异步回包

有时候当服务端收到请求时，我们需要异步执行某些任务，然后等任务执行完后再进行回包。这时候就需要用到框架的异步回包功能。使用方式如下：

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

说明：如果不需要设置响应体，只返回Status状态，那使用只带Status参数的接口即可：

```cpp
void SendUnaryResponse(const Status& status);
```

## 约束

### 请求包最大长度默认为10M

对于框架内置的trpc及http协议而已，框架限制请求包的最大长度为10M。这个限制是Service级别的，用户可通过增加service的max_packet_size配置项进行修改，方式如下：

```yaml
server:
  service:
    - name: trpc.test.helloworld.Greeter
      max_packet_size: 10000000  # max packet size limited
```

### 空闲连接超时

服务端的连接超时时间默认是60秒，如果调用方连续60秒没有发送数据包则连接会被断开。这个限制是Service级别的，可以通过配置项idle_time进行修改：

```yaml
server:
    service:
      - name: trpc.test.helloworld.Greeter
        idle_time: 60000  # connection idle timeout
```

### 服务自注册

默认情况下，框架不会向名字服务系统进行service实例的自动注册与反注册。如需开启框架的自注册模式，需要进行如下配置：

```yaml
server:
  registry_name: xxx      # 要注册到哪个名字服务系统
  enable_self_register: true  # 开启框架的自注册
```

### 服务心跳上报和僵死检测

框架支持上报service实例的心跳到名字服务系统，以报告service自身的可用状态。使用时需增加如下配置：

```yaml
server:
  registry_name: xxx  # 上报心跳到哪个名字服务系统
```

对于IO/Handle分离及合并线程模型，框架支持worker线程(IO及Handle线程)的僵死检测。其原理为：框架worker线程每 3 秒会标记自己的活跃状态，如果某个线程超过阈值（60秒）都没有标记活跃状态则认为该线程僵死。

如果检测到分离模式下所有的Handle线程僵死或合并模式下所有worker线程僵死，则认为服务不可用，这时会停止向名字服务系统上报心跳。这样可在服务不可用时，通过名字服务系统让调用方及时感知到服务的状态，避免调用到不可用的服务节点，起到保护的作用。

对于离线计算等服务，这时请求处理时间可能超过 60 秒，会导致误判，可以调大超时阈值来解决：

```yaml
global:
  heartbeat:
    thread_heartbeat_time_out: 60000 # 单位毫秒，判断handle线程僵死的阈值
```

另外，如果不需要上报心跳的话，可通过如下方式关闭：

```yaml
heartbeat:
  enable_heartbeat: false  # 心跳上报开关，默认为true
```

### 关于fork

fork 在多线程下有历史深坑，请阅读[相关文章](https://pubs.opengroup.org/onlinepubs/009695399/functions/fork.html)先了解这个前提。

tRPC-Cpp 是多线程的框架，只支持一种 fork 用法：fork with exec，即 fork 后立马调用 exec 相关函数。

而不支持 fork without exec，如果一定要这样用：请尽可能早的使用fork(在框架初始化前)。

使用 fork with exec 时，如果 exec 返回则代表执行失败，请调用 _exit() 立马退出子进程，不要调用 exit() 退出；这样做是为了避免某些单例或者全局变量析构导致一些未定义行为(比如在子进程处理父进程开的thread等)。

## 常见协议类型的服务开发

[开发tRPC协议服务](trpc_protocol_service.md)

[开发HTTP协议服务](http_protocol_service.md)

[开发grpc协议服务](grpc_protocol_service.md)

## 错误码

详见[框架错误码]()说明

## 高级功能

### 指定请求的处理线程

详见[请求指定线程](../../examples/features/request_dispatch/README.md)

### 超时控制

详见[超时控制]()

### 透明代理

详见[透明代理](transparent_service.md)

### 定时器

详见[定时器]()

### 流控和过载保护

详见[流控和过载保护]()
