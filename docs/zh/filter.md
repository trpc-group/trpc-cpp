[English](/docs/en/filter.md)

[TOC]

# 自定义拦截器(filter)

## 前言

为了使框架有更强的可扩展性, tRPC引入了拦截器这个概念, 它借鉴了[java面向切面(AOP)](https://zh.wikipedia.org/wiki/%E9%9D%A2%E5%90%91%E5%88%87%E9%9D%A2%E7%9A%84%E7%A8%8B%E5%BA%8F%E8%AE%BE%E8%AE%A1)的编程思想。具体做法是通过在框架指定的动作前后设置埋点, 然后在埋点地方插入一系列的拦截器(也称为filter), 来扩展框架的功能，也便于对接不同的生态。

利用拦截器机制，框架将接口请求相关的特定逻辑组件化，插件化，从而同具体的业务逻辑解耦，达到复用的目的。例如监控拦截器，日志拦截器，鉴权拦截器等。

具体到业务逻辑上，使用者可以通过自定义拦截器来实现诸如服务限流、服务监控等功能，因此掌握如何自定义和使用框架拦截器很有必要。

下面，我们先介绍下拦截器的原理，然后介绍如何实现和使用自定义拦截器。

## 原理

tRPC-Cpp拦截器在实现上采用了数组遍历方式：即定义了一批埋点，然后在各个调用环节中依次执行对应的埋点逻辑。要理解框架实现拦截器的原理，重点是理解下面几个重要部分：

- [埋点位置和执行流程](#埋点位置和执行流程)
- [Filter类型与执行顺序](#filter类型与执行顺序)

### 埋点位置和执行流程

框架的埋点分为服务端埋点以及客户端埋点，各有10个埋点，定义见[filter_point.h](/trpc/filter/filter_point.h)，一般用户只需关注以下的几个埋点即可。如果需要使用其他埋点的话可参考rpcz说明文档。

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

其埋点位置和执行流程如下：

![img](/docs/images/filter_point.png)

**客户端埋点说明**

| 埋点 |说明|
| ------ | ------- |
| CLIENT_PRE_RPC_INVOKE | 用户调用rpc接口后立即执行此埋点, 此处请求内容还未进行序列化|
| CLIENT_PRE_SEND_MSG | 此处已完成请求内容的序列化，下一阶段就进入请求的编码及发送|
| CLIENT_POST_RECV_MSG | 此处已收到回包后并对其进行了解码，但还未对响应内容进行反序列化|
| CLIENT_POST_RPC_INVOKE | 此处已完成响应内容的反序列化，这时表示rpc调用完成|

**服务端埋点说明**

| 埋点 | 说明|
| ------ | ------- |
| SERVER_POST_RECV_MSG | 此处已收到请求包并对其进行了解码，但还未对响应内容进行反序列化|
| SERVER_PRE_RPC_INVOKE | 此处已完成请求内容的反序列化，下一阶段就进入RPC处理函数|
| SERVER_POST_RPC_INVOKE | 此外已完成RPC处理逻辑，RPC处理逻辑执行完后立即执行此埋点，此时响应内容还未进行序列化|
| SERVER_PRE_SEND_MSG | 此处已完成对响应内容的序列化，下一阶段就进入响应的编码和发送|

**埋点处执行逻辑说明**

框架在埋点处会调用`FilterController`类的拦截器执行函数，客户端调的是`RunMessageClientFilters`函数，服务端调的是`RunMessageServerFilters`函数。这时将执行所有注册到框架的filter所实现的 **`operator()`函数** 。这里**`operator()`函数** 是框架filter基类中的一个纯虚函数，具体的filter继承基类后都需要重写这个函数，然后利用接口参数中的context获取所需信息来实现具体功能。

### Filter类型与执行顺序

框架支持的filter类型，按作用范围划分的话有两种：

1. global级别的filter

   针对server或client下所有的service都生效，配置方式为
   
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

2. service级别的filter
   
   只针对当前service生效，配置方式为
   
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

一个filter是global级别的filter或service级别的filter取决于其配置的位置。如果一个filter同时配置为global及service filter时，将作为global filter对待。

filter的埋点配置以及执行顺序遵循如下原则：

- filter埋点需要配对

  为了支持filter执行失败后能正常退出，我们要求filter埋点需要配对。其中(偶数N，偶数N+1)为配对的filter埋点。

- 支持按配置顺序执行filter

  执行顺序上：先按配置顺序执行global filter的前置埋点，再按配置顺序执行service filter的前置埋点；后置埋点的执行上与前置埋点的执行顺序相反，即`First In Last Out`。

- 支持filter中断退出

  如果某个filter在前置埋点执行失败了，将中断filter链的执行，后续流程将只执行已执行过filter的后置埋点(不含此失败的filter)。

详见[filter设计原则](/trpc/filter/README.md)

## 如何实现和使用自定义拦截器(filter)

整体步骤为：实现拦截器 --> 将拦截器注册到框架 --> 使用拦截器

### 实现拦截器

- 服务端拦截器

  继承MessageServerFilter：重载GetFilterPoint函数，选择所需要的埋点；重载operator函数，实现filter的逻辑
  
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

- 客户端拦截器
  
  继承MessageClientFilter：重载GetFilterPoint函数，选择所需要的埋点；重载operator函数，实现filter的逻辑
  
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

### 将拦截器注册到框架

按对框架的使用方式分为如下两种情况：

1. 程序作为服务端
   
   即程序采用Server方式实现，继承于TrpcApp场景。这时你需要重载`trpc::TrpcApp::RegisterPlugins`接口，然后将filter注册逻辑放到里面
   
   ```cpp
   #include "trpc/common/trpc_plugin.h"
   
   int xxxServer::RegisterPlugins() {
     // register server-side filter
     auto server_filter = std::make_shared<CustomServerFilter>();
     TrpcPlugin::GetInstance()->RegisterServerFilter(server_filter);

     // register client-side filter
     auto client_filter = std::make_shared<CustomClientFilter>();
     TrpcPlugin::GetInstance()->RegisterClientFilter(client_filter);
   }
   ```
2. 程序作为纯客户端
   
   即程序只需使用客户端功能，且未使用TrpcApp场景。这时你需要将注册filter的逻辑置于`trpc::RunInTrpcRuntime`前面
   
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

### 使用拦截器

可以通过配置文件或代码指定方式来使用拦截器。

其中配置文件方式灵活性好，调整时不需要修改代码。配置方式见[Filter类型与执行顺序](#filter类型与执行顺序)。

代码指定方式则可以在`GetProxy获取ServiceProxy时`或`StartService启动Service时`通过代码指定需要使用的service filters。具体而已，获取proxy时，通过`ServiceProxyOption`的`service_filters`字段指定；启动service时，通过`ServiceConfig`的`service_filters`字段指定。

Note: global级别的filter只支持配置文件方式指定。

## 高级用法

### 为service配置独立的filter

默认一个filter对象会被多个serviceproxy或多个service共享，造成在某些场景中编程不太方便、同时也影响性能。如“根据service的调用情况进行限流”场景，由于filter是共享的，这时需要使用map结构存储service名和其调用情况，同时对map结果的存取也需要加锁。对此，一种解决方式是让一个serviceproxy或service拥有独立的filter对象，这样在上述场景下，就无需使用map结构以及加锁，在简化编程的基础上也提升了性能。

因此框架提供了对应的解决方案，用户可以通过重载Filter基类的Create接口来使每个service拥有独立的filter对象。

```cpp
virtual MessageClientFilterPtr Create(const std::any& param) { return nullptr; }
```

当Create返回的结果不为空时，则使用其作为service的filter对象；否则，采用共享的filter作为其service的filter对象。另外，此场景下，也支持为新创建的filter添加独立配置。

以客户端代理为例，使用方式如下：

```cpp
MyFilterConfig config;  // 具体的filter插件配置
trpc::ServiceProxyOption option;
...
option.service_filter_configs["my_filter"] = config;  // 其中my_filter为filter名
```

这样，当Filter的Create方法被调用时，会将其关联的filter配置作为参数传递给Create接口，从而用户可在Create实现中使用std::any_cast来获取对应的配置。

```cpp
trpc::MessageClientFilterPtr MyFilter::Create(const std::any& param) {
  MyFilterConfig config;
  if (param.has_value()) {
    config = std::any_cast<MyFilterConfig>(param);
  }

  return std::make_shared<MyFilter>(config);
}
```
