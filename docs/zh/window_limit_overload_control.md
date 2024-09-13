# 使用示例

基于流量控制的局部过载保护过滤器，**当前只能应用于服务端，客户端暂时不支持**，用户使用需修改代码，开启编译和增加配置即可。

## 编译选项

编译选项：在`.bazelrc` 文件中加入下面一行

```sh
build --define trpc_include_overload_control=true
```
## 配置文件

服务端流量控制配置如下（详细配置参考：[flow_test.yaml](../../trpc/overload_control/flow_control/flow_test.yaml)）：
```
# Plugin configuration.
plugins:
  window_limiter_overload_control:
    window_limiter:
      - service_name: xxxx
        is_report: xxxx
        service_limiter: xxxx
        window_size: xxxx
        func_limiter: 
          - name: xxxx
            limiter: xxxx
            window_size: xxxx
```

配置关键点如下：

- window_limiter  ：流量控制过载保护器的名称
- service_name：流量控制对应的服务名称
- service_limiter：指定服务名 `service_name` 下的服务级流控算法；这里配置 `default(100000)` 介绍如下：
  - `default`（等同 `seconds`）：表示此处选择了固定窗口流量控制算法
  - `100000`：表示每秒最大的请求数，也即最大 QPS 是 100000
- func_limiter：指定服务名 `service_name` 下的接口级流控策略；每个服务可以存在多个接口，每个接口可以选择不同的流量控制算法，介绍如下：
  - `name`：接口名称，如：`SayHello` 和 `Route`
  - `limiter`：指定接口流控算法，例如：`Route` 配置了滑动窗口（smooth）算法，最大 QPS 是 80000
- is_report：是否上报监控数据到监控插件，**注意，该配置必须与监控插件一起使用(例如配置：plugins->metrics->prometheus，则会上报到 prometheus 上)，如果没有配置监控插件，该选项无意义**，不同流量控制算法监控数据分别如下：
  - seconds：
    - `SecondsLimiter`：流控算法名称，用于检查配置和程序中执行的流量控制算法是否一致
    - `/{callee_name}/{method}`：监控名称格式，由被调服务(callee_name)和方法名(method)组成，例如：`/trpc.test.helloworld.Greeter/SayHello`
    - `current_qps`：当前并发 QPS 值
    - `max_qps`: 配置的最大 QPS，用于检查配置和程序中执行的流量控制最大 QPS 是否一致
    - `window_size`：采样窗口个数，用户可不关注
    - `Pass`：单个请求的通过状态，0：拦截；1：通过
    - `Limited`：单个请求的拦截状态，1：拦截；0：通过。与上面的 `Pass` 监控属性是相反的
  - smooth：
    - `SmoothLimiter`：流控算法名称，用于检查配置和程序中执行的流量控制算法是否一致
    - `/{callee_name}/{method}`: 监控名称格式，由被调服务(callee_name)和方法名(method)组成，例如：`/trpc.test.helloworld.Greeter/SayHello`
    - `active_sum`：所有命中的时间片中的请求总和，表示除去当前请求后此刻的 QPS 值，若超过最大 QPS，下面的 `hit_num` 则为 0
    - `hit_num`：当前命中时间片增加一个请求后的的 QPS 值
    - `max_qps`：配置的最大 QPS，用于检查配置和程序中执行的流量控制最大 QPS 是否一致
    - `window_size`：采样窗口个数，用户可不关注
    - `Pass`：单个请求的通过状态，0：拦截；1：通过
    - `Limited`：单个请求的拦截状态，1：拦截；0：通过。与上面的 `Pass` 监控属性是相反的

## 代码方式采用流量控制

除了通过配置流量控制之外，还必须注册filter。业务使用者注册服务级和接口级流量控制器方式如下：

```cpp
//引入filter头文件后
class HelloWorldServer : public ::trpc::TrpcApp {
 public:
  // ...
  int RegisterPlugins() {
  // register server-side filter
  auto server_filter = std::make_shared<trpc::overload_control::WindowLimitOverloadControlFilter>();
  trpc::TrpcPlugin::GetInstance()->RegisterServerFilter(server_filter);
  return 0;
  }

};
```


