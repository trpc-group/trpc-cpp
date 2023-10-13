[English](../en/benchmark.md)

# 性能测试

## 测试环境

测试机配置：腾讯云标准型SA2 cvm，处理器型号AMD EPYC™ Rome，主频2.6GHz、8核16G内存，内核版本为Linux VM-0-13-centos 3.10.0。

网络环境：IDC内网环境，主调和被调机器间ping平均延迟为0.2ms左右。

可执行文件编译及链接选项：采用gcc8编译，开启O2选项，链接tcmalloc(gperftools-2.9.1)。

## 测试场景

吞吐测试：调用方的P999延时在10ms左右时，测量服务的QPS。

长尾请求测试：固定QPS为1w/s时，在有1%的请求处理存在5ms长尾延时情况下(测试时额外启动一个压测工具发1%的长尾请求流量)，考察普通请求的处理延时情况(长尾请求的延时不计入结果)。

### 关于主调与被调

被调方为采用trpc协议的echo服务，其rpc接口为：

```cpp
::trpc::Status GreeterServiceImpl::SayHello(::trpc::ServerContextPtr context,
                                            const ::trpc::test::helloworld::HelloRequest* request,
                                            ::trpc::test::helloworld::HelloReply* reply) {
  reply->set_msg(std::move(request->msg()));

  return ::trpc::kSuccStatus;
}
```

主调方为压测工具，测试时其会向服务端建立100个连接，每次请求的body大小为10Byte。

### 吞吐测试结果

| | QPS/w | AVG latency/ms | P90 latency/ms | P99 latency/ms | P999 latency/ms |
| ---| ----|------------| ------------| ------------| ------------|
|fiber模式(8个worker线程)| 50.8 | 3.36 | 4.69 | 8.36 | 13.47 |
|合并模式(8个worker线程)| 57.8 | 1.46 | 3.00 | 9.72 | 17.72 |

可见合并模式的平均延时最低、QPS最高，这是由于合并模式下请求处理不跨线程，cache miss最小，可以做到很低的平均延时和高的QPS。

### 长尾请求测试结果

| 不带长尾请求场景下耗时/带长尾请求场景下耗时| AVG latency/ms | P99 latency/ms | P999 latency/ms | Rise rate |
| ----| ------------| ------------| ------------| ------------|
|fiber模式(8个worker线程)| 0.40/0.47 | 1.31/1.82 | 1.97/3.41| avg latency: 17.5% ↑, P99 latency: 38.9% ↑ |
|IO/Handle合并模式(8个worker线程)| 0.39/0.62 | 1.23/5.86 | 1.92/7.80 | avg latency: 59.0% ↑，P99 latency: 376.4% ↑  |

可见fiber模式受长尾请求的影响相对小，长尾请求抗干扰能力强，而合并模式受长尾延时的影响大。这个是因为fiber模式下请求可被所有worker线程并行处理的，而合并模式下由于worker线程对请求的处理不能多核并行化，一旦有某个请求处理较长，会影响整体请求的处理时长。
