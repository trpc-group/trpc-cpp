[English](../en/timeout_control.md)

# 前言

超时控制是rpc框架的一个基本能力，让服务调用按照一个约定的时间去执行，可以避免陷入无限的等待之中，从而提高系统的可用性和稳定性，优化资源的使用，也可以减少客户端和服务端的不一致行为。

这篇文档将对tRPC-Cpp的超时控制机制进行介绍。

# 机制

## 整体介绍

tRPC-Cpp超时控制机制的原理图如下：

![timeout control](../images/timeout_control.png)

从图中可以看到，超时控制主要有下面**三个影响因素**：

* **链路超时（Link timeout）**：上游调用方通过协议字段把自己允许的超时时间传给当前服务，意思是说上游只给这么多的超时时间，请在该超时时间内务必返回数据。如客户端A调用服务B的链路超时时间。
* **消息超时（Message timeout）**：当前服务配置的从收到请求消息到返回响应数据的最长消息处理时间，这是当前服务控制自身不浪费资源的手段。如服务B内部的请求整体超时时间。
* **调用超时（Call timeout）**：当前服务调用下游服务设置的每一个rpc请求的超时时间，如B调用C的单个超时时间。一次请求可能会连续进行多次rpc调用，如B调完C，继续串行调用D和E，这个调用超时控制的是每个rpc的独立超时时间。

实际发起rpc调用请求时，**真正生效的超时时间是通过以上三个因素实时计算的最小值**，其计算过程如下：

* **首先服务端计算得到链路超时和消息超时的最小值，作为本次请求的最长处理时间。**比如：链路超时2s，消息超时1s，则当前请求的最长处理时间为1s。
* **服务端进一步发起rpc调用时，再次计算当前请求最长处理时间和单个调用超时的最小值。**比如：当前请求的最长处理时间为1s，B调用C设置的调用超时为5s，则实际上的真实超时时间为1s。而若B调用C的调用超时为500ms，则真实超时将为500ms，并且500ms这个值也会通过协议字段传给C，在服务端C的视角来看就是它的链路超时时间。**链路超时时间会在整个rpc调用链上一直传递下去，并逐渐减少，直至为0，这样也就永远不会出现死循环调用的问题。**
* **因为每一次rpc调用都会实际消耗一部分时间，所以当前请求最长处理时间需要实时计算剩余可用时间。**比如B调用C真实耗时200ms，那么调用结束后最长处理时间就只剩下800ms了。在发起第二次rpc调用时，则需要计算此时剩余的请求超时时间和单个调用时间的最小值。如B调用D设置的单个超时时间为1s，但实际生效的超时时间为800ms。

注意：

1. 通信协议需要支持携带链路超时，达到服务端能感知客户端超时时长的目的。例如`trpc`协议。
2. 服务端进一步发起rpc调用时，需要使用`MakeClientContext`接口，根据ServerContext构造ClientContext，自动计算剩余调用时间。

## 超时配置

### 链路超时

链路超时时间默认会从最源头的服务一直通过协议字段透传下去，服务端可以通过`“server”`的`”disable_request_timeout“`选项来配置是否启用全链路超时机制。默认取值为`false`，表示会继承上游的设置的链路超时时间；配置`true`为禁用，表示忽略上游调用当前服务时协议传递过来的链路超时时间。

```yaml
server:
  service:
    - name: trpc.app.server.service
      disable_request_timeout: false
```

### 消息超时

每个服务都可通过`“server”`的`“timeout”`选项来配置该服务所有请求的最长处理超时时间。不设置默认为UINT32_MAX。

```yaml
server:
  service:
    - name: trpc.app.server.service
      timeout: 1000  # 单位ms
```

### 调用超时

每个客户端都可以通过`“client”`的`“timeout”`选项配置调用超时。不设置默认为UINT32_MAX。

```yaml
client:
  service:
    - name: trpc.app.server.service
      timeout: 500  # 单位ms
```

除了该配置选项外，框架中还有其他的调用超时设置方式：

1. 通过`GetProxy`获取代理时，显式指定了option的timeout，则调用超时以代码为准，配置文件指定的不生效。但我们建议使用配置文件方式，更加灵活和直观。
2. 客户端可以在代码中为每次调用单独设置超时时间，方式为调用ClientContext的`SetTimeout`接口。

    ```cpp
    void SetTimeout(uint32_t value, bool ignore_proxy_timeout = false);
    ```

    这种情况下，最终的调用超时是由代理的timeout配置和SetTimeout的值共同决定的：
    * ignore_proxy_timeout设置为false时，框架会取两者中的较小值作为调用超时。
    * ignore_proxy_timeout设置为true时，则忽略代理的timeout配置，以用SetTimeout的值为准。

## 拓展功能

### 自定义超时处理函数

框架提供了请求处理超时情况下的回调接口，供用户在超时的情况下进行一些额外处理，例如打印更多的日志或者上报监控以方便定位问题。

#### 服务端超时处理函数

服务端超时处理函数的类型如下：
```cpp
using ServiceTimeoutHandleFunction = std::function<void(const ServerContextPtr& context)>;
```

用户需要在服务初始化时为Service设置自定义的超时处理函数：
```cpp
// 自定义超时处理函数
void UserServiceTimeoutFunc(const trpc::ServerContextPtr& context) {
  TRPC_LOG_ERROR("server status:" << context->GetStatus().ToString());
}

int RouteServer::Initialize() {
  ...
  trpc::ServicePtr route_service(std::make_shared<RouteService>());
  // 设置自定义的超时处理函数
  route_service->SetServiceTimeoutHandleFunction(UserServiceTimeoutFunc);
  RegisterService(service_name, route_service);
  return 0;
}
```

#### 客户端超时处理函数

客户端超时处理函数的类型如下：
```cpp
using ClientTimeoutHandleFunction = std::function<void(const ClientContextPtr&)>;
```

用户需要在初始化proxy时设置自定义的客户端超时处理函数：
```cpp
// 自定义超时处理函数
void UserClientTimeoutFunc(const trpc::ClientContextPtr& context) {
  TRPC_LOG_ERROR("client status:" << context->GetStatus().ToString());
}

{
	...
    // 设置自定义的超时处理函数
	trpc::ServiceProxyOption option;
	option.proxy_callback.client_timeout_handle_function = UserClientTimeoutFunc;
	GreeterProxyPtr proxy =
        trpc::GetTrpcClient()->GetProxy<trpc::test::helloworld::GreeterServiceProxy>(service_name, &option);
	...
}
```

# FAQ

### 1 为什么设置了很大的超时时间，但实际上耗时很短就提示超时失败了？

框架对每次收到的请求都有一个最长处理时间的限制，每次rpc后端调用的超时时间都是根据当前剩余最长处理时间和调用超时实时计算的，这种情况大概率是因为多个串行rpc调用时，前面已经把时间耗的差不多了，所以留给这次rpc的时间不够用了。

### 2 为什么客户端没有配置超时时间，但是发现超时时长是5s？

对于客户端代理和ClientContext都没有设置超时时间的情况，框架会将实际超时时间设置为5s，而不是无限大。
