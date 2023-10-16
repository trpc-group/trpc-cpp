[中文](../zh/benchmark.md)

# Performance testing

## Testing environment

Test machine configuration: Tencent Cloud Standard SA2 CVM with an AMD EPYC™ Rome processor, operating at a frequency of 2.6GHz, 8 cores, 16GB of memory, and a Linux VM-0-13-centos 3.10.0 kernel version.

Network environment: IDC intranet environment, the average ping latency between the caller and the called machines is around 0.2ms.

Compilation and linking options for the executable file: Compiled using gcc8, with O2 optimization enabled, and linked with tcmalloc (gperftools-2.9.1).

## Test scenario

Throughput test: Test the QPS (Queries Per Second) of the service when the P999 latency of the caller is around 10ms. Based on whether the called service continues to call downstream, it can be divided into pure server mode and proxy server mode. In the pure server mode, the call chain is: Client --> Server (target service), while in the proxy server mode, the call chain is: Client --> ProxyServer (target service) --> Server.

Long-tail request test: In pure server mode, with a fixed QPS of 10,000 requests per second, simulate a scenario where 1% of the requests experience a 5ms long-tail latency (during the test, an additional stress testing tool is launched to send 1% of the long-tail request traffic), and examine the latency of ordinary requests.

### About caller and callee

The callee is an echo service using the trpc protocol. The RPC interface in pure server mode is:

```cpp
::trpc::Status GreeterServiceImpl::SayHello(::trpc::ServerContextPtr context,
                                            const ::trpc::test::helloworld::HelloRequest* request,
                                            ::trpc::test::helloworld::HelloReply* reply) {
  reply->set_msg(std::move(request->msg()));

  return ::trpc::kSuccStatus;
}
```

The RPC interface for the proxy server mode is (using code example in fiber mode):

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

The main caller is a stress testing tool. During the test, it will establish 100 connections to the server, and the body size of each request is 10Byte.

### Throughput test results

#### Pure server mode

| | QPS/w | AVG latency/ms | P90 latency/ms | P99 latency/ms | P999 latency/ms |
| ---| ----|------------| ------------| ------------| ------------|
|fiber mode(with 8 worker thread)| 50.8 | 3.36 | 4.69 | 8.36 | 13.47 |
|merge mode(with 8 worker thread)| 57.8 | 1.46 | 3.00 | 9.72 | 17.72 |

It can be seen that in the merge mode, the average latency is lower and the QPS is higher. This is because the request processing in the merge mode does not cross threads and the cache miss is fewer, so it can achieve a very low average latency and a high QPS.

#### Proxy server mode

| | QPS/w | AVG latency/ms | P90 latency/ms | P99 latency/ms | P999 latency/ms |
| ---| ----|------------| ------------| ------------| ------------|
|fiber mode(with 8 worker thread)| 20.7 | 3.57 | 4.95 | 6.33 | 7.67 |
|merge mode(with 8 worker thread)| 20.7 | 2.12 | 5.69 | 9.18 | 11.71 |

At around 10ms of P999 latency, the QPS of fiber mode and merge mode are comparable, but the fiber mode exhibits better performance in terms of tail latency compared to the merge mode. This is because in the fiber model, network IO and business processing logic can be parallelized across multiple cores, effectively utilizing multiple cores and achieving very low tail latency.

### Long-tail request test results

| without long-tail request / with long-tail request | AVG latency/ms | P99 latency/ms | P999 latency/ms | Rise rate |
| ----| ------------| ------------| ------------| ------------|
|fiber mode(with 8 worker thread)| 0.40/0.47 | 1.31/1.82 | 1.97/3.41 | avg latency: 17.5% ↑, P99 latency: 38.9% ↑ |
|merge mode(with 8 worker thread)| 0.39/0.62 | 1.23/5.86 | 1.92/7.80 | avg latency: 59.0% ↑, P99 latency: 376.4% ↑ |

Note: the latency of long-tail requests is not included in the results.

It can be seen that the fiber mode is less affected by long-tail requests and has strong resistance to interference from long-tail requests, while the merge mode is greatly affected by long-tail latency. This is because in the fiber mode, requests can be processed in parallel by all worker threads, while in the merge mode, the processing of requests by worker threads cannot be parallelized across multiple cores. Once a certain request is processed for a long time, it will affect the processing time of the overall request.