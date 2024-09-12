# 前言

tRPC-Cpp框架目前已经实现了多种过载保护插件，本文主要是介绍一种基于令牌桶（Token-Bucket）算法实现的过载保护插件，其本质上是一种流量速率控制方法。令牌桶可以看作是一个存放令牌的容器，用户可以预先设定一定的**桶容量**和**令牌的产生速度**，表示桶内最多可以存放的”令牌“数量,以及向桶中补充令牌的速度。当系统收到一个请求时，先要到令牌桶里面拿“令牌”，拿到“令牌”才能进一步处理，拿不到就要丢弃请求。

# 原理

## 基于令牌桶过载保护原理图

<img src="..\images\token_bucket_limiter.png" alt="token_bucket_limiter" style="zoom: 33%;" />

图中核心点就是`TokenBucketLimiter`，它维护了一个令牌桶，系统定时地在不超过令牌桶容量的情况下往令牌桶添加令牌。当一个请求`（Req）`到达时，判断当前令牌桶中是否至少拥有一个令牌`（current_token>0?）`，如果拥有令牌，则消耗一个令牌并接受请求`（Continue）`，否则拒绝请求`（Reject）`。

## 实现代码

 框架的过载保护实现方式是基于[过滤器](./filter.md)，为了保证尽可能早进行限流保护；该过滤器会埋点在请求入队前，具体埋点实现如下：

```c++
std::vector<FilterPoint> TokenBucketLimiterServerFilter::GetFilterPoint() {
  return {
      FilterPoint::SERVER_PRE_SCHED_RECV_MSG,
      // ...
  };
}
```

源码参考：[token_bucket_limiter_server_filter](../../trpc/overload_control/token_bucket_limiter/token_bucket_limiter_server_filter.cc)

# 使用示例

用户使用无需修改任何代码，只需开启编译和增加配置即可，非常方便。

## 编译选项

编译选项：在`.bazelrc`文件中加入下面一行

```sh
build --define trpc_include_overload_control=true
```

## 配置文件

```yaml
#Server configuration
server:
  app: test #Business name, such as: COS, CDB.
  server: helloworld #Module name of the business
  admin_port: 21111 # Admin port
  admin_ip: 0.0.0.0 # Admin ip
  service: #Business service, can have multiple.
    - name: trpc.test.helloworld.Greeter #Service name, needs to be filled in according to the format, the first field is default to trpc, the second and third fields are the app and server configurations above, and the fourth field is the user-defined service_name.
      network: tcp #Network listening type: for example: TCP, UDP.
      ip: 0.0.0.0 #Listen ip
      port: 10001 #Listen port
      protocol: trpc #Service application layer protocol, for example: trpc, http.
      accept_thread_num: 1 #Number of threads for binding ports.
      filter:
        - token_bucket_limiter

#Plugin configuration.
plugins:
#  metrics:
#    prometheus:
      # ...
  overload_control:
    token_bucket_limiter:
      capacity: 50 # Maximum token bucket count capacity. It is configured small for unit testing purposes, but users can configure it to be larger.
      initial_token: 0 # Initial token count.
      rate: 5 # Token production rate(count/second).
      is_report: true # Whether to report
```

配置关键点如下：

- token_bucket_limiter：并发请求过载保护器的名称
- capacity：令牌桶的最大容量
- initial_token：令牌桶中令牌的初始数量
- rate：令牌的产生速度（个/秒）
- is_report：是否上报监控数据到监控插件，**注意，该配置必须与监控插件一起使用(例如配置：plugins->metrics->prometheus，则会上报到 prometheus 上)，如果没有配置监控插件，该选项无意义**，被监控数据有：
  - `current_token`：当前的令牌数量
  - `/{callee_name}/{method}`: 上报监控的 RPC 方法名，属于固定值；由被调服务(callee_name)和方法名(method)组成，例如：`/trpc.test.helloworld.Greeter/SayHello`。
  - `Pass`：单个请求的通过状态，0：拦截；1：通过
  - `Limited`：单个请求的拦截状态，1：拦截；0：通过。与上面的 `Pass` 监控属性是相反的
