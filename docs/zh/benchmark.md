[English](../en/benchmark.md)

# 性能测试

## 测试环境

测试机配置：腾讯云标准型SA2 cvm，处理器型号AMD EPYC™ Rome，主频2.6GHz、8核16G内存，内核版本为Linux VM-0-13-centos 3.10.0。

网络环境：IDC内网环境，主调和被调机器间ping平均延迟为0.2ms左右。

可执行文件编译及链接选项：采用gcc8编译，开启O2选项，链接tcmalloc(gperftools-2.9.1)。

## 测试场景

吞吐测试：调用方的P999延时在10ms左右时，测量服务的QPS。根据被调服务是否继续调用下游，又分为纯服务端模式和中转调用模式。其中纯服务端模式调用链为: Client --> Server(被测服务)，中转调用模式调用链为: Client --> ProxyServer(被测服务) --> Server。

长尾请求测试：在纯服务端模式下，固定QPS为1w/s时，在有1%的请求处理存在5ms长尾延时情况下(测试时额外启动一个压测工具发1%的长尾请求流量)，考察普通请求的处理延时情况。

### 关于主调与被调

被调方为采用trpc协议的echo服务，纯服务端模式的rpc接口为：

```cpp
::trpc::Status GreeterServiceImpl::SayHello(::trpc::ServerContextPtr context,
                                            const ::trpc::test::helloworld::HelloRequest* request,
                                            ::trpc::test::helloworld::HelloReply* reply) {
  reply->set_msg(std::move(request->msg()));

  return ::trpc::kSuccStatus;
}
```

中转调用模式的rpc接口为(以fiber模式下的代码为例)：

```cpp
::trpc::Status ForwardServiceImpl::Route(::trpc::ServerContextPtr context,
                                         const ::trpc::test::helloworld::HelloRequest* request,
                                         ::trpc::test::helloworld::HelloReply* reply) {
  auto client_context = ::trpc::MakeClientContext(context, greeter_proxy_);

  ::trpc::test::helloworld::HelloRequest route_request;
  route_request.set_msg(request->msg());

  ::trpc::test::helloworld::HelloReply route_reply;

  // block current fiber, not block current fiber worker thread
  ::trpc::Status status = greeter_proxy_->SayHello(client_context, route_request, &route_reply);

  reply->set_msg(route_reply.msg());

  return status;
}
```

主调方为压测工具，测试时其会向服务端建立100个连接，每次请求的body大小为10Byte。

### 吞吐测试结果

#### 纯服务端模式

| | QPS/w | AVG latency/ms | P90 latency/ms | P99 latency/ms | P999 latency/ms |
| ---| ----|------------| ------------| ------------| ------------|
|fiber模式(8个worker线程)| 50.8 | 3.36 | 4.69 | 8.36 | 13.47 |
|合并模式(8个worker线程)| 57.8 | 1.46 | 3.00 | 9.72 | 17.72 |

可见合并模式的平均延时更低、QPS更高，这是由于合并模式下请求处理不跨线程，cache miss更少，可以做到很低的平均延时和高的QPS。

#### 中转调用模式

| | QPS/w | AVG latency/ms | P90 latency/ms | P99 latency/ms | P999 latency/ms |
| ---| ----|------------| ------------| ------------| ------------|
|fiber模式(8个worker线程)| 20.7 | 3.57 | 4.95 | 6.33 | 7.67 |
|合并模式(8个worker线程)| 20.7 | 2.12 | 5.69 | 9.18 | 11.71 |

P999延时在10ms左右时，fiber模式和合并模式的QPS相当，但fiber模式的长尾延时表现要好于合并模式。这是由于fiber模型下网络IO和业务处理逻辑可多核并行化，能充分利用多核，可做到很低的长尾延时。

### 长尾请求测试结果

| 不带长尾请求场景耗时/带长尾请求场景耗时| AVG latency/ms | P99 latency/ms | P999 latency/ms | Rise rate |
| ----| ------------| ------------| ------------| ------------|
|fiber模式(8个worker线程)| 0.40/0.47 | 1.31/1.82 | 1.97/3.41| avg latency: 17.5% ↑, P99 latency: 38.9% ↑ |
|IO/Handle合并模式(8个worker线程)| 0.39/0.62 | 1.23/5.86 | 1.92/7.80 | avg latency: 59.0% ↑，P99 latency: 376.4% ↑  |

Note: 长尾请求的延时不计入上述统计结果。

可见fiber模式受长尾请求的影响相对小，长尾请求抗干扰能力强，而合并模式受长尾延时的影响大。这个是因为fiber模式下请求可被所有worker线程并行处理的，而合并模式下由于worker线程对请求的处理不能多核并行化，一旦有某个请求处理较长，会影响整体请求的处理时长。
