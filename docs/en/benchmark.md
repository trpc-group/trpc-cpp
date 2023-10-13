[中文](../zh/benchmark.md)

# Performance testing

## Testing environment

Test machine configuration: Tencent Cloud Standard SA2 CVM with an AMD EPYC™ Rome processor, operating at a frequency of 2.6GHz, 8 cores, 16GB of memory, and a Linux VM-0-13-centos 3.10.0 kernel version.

Network environment: IDC intranet environment, the average ping latency between the caller and the called machines is around 0.2ms.

Compilation and linking options for the executable file: Compiled using gcc8, with O2 optimization enabled, and linked with tcmalloc (gperftools-2.9.1).

## Test scenario

Throughput test: Test the QPS (Queries Per Second) of the service when the P999 latency of the caller is around 10ms.

Long-tail request test: With a fixed QPS of 10,000 requests per second, simulate a scenario where 1% of the requests experience a 5ms long-tail latency (during the test, an additional stress testing tool is launched to send 1% of the long-tail request traffic), and examine the latency of ordinary requests (the latency of long-tail requests is not included in the results).

### About caller and callee

The callee is an echo service using the trpc protocol, and its RPC interface is:

```
::trpc::Status GreeterServiceImpl::SayHello(::trpc::ServerContextPtr context,
                                            const ::trpc::test::helloworld::HelloRequest* request,
                                            ::trpc::test::helloworld::HelloReply* reply) {
  reply->set_msg(std::move(request->msg()));

  return ::trpc::kSuccStatus;
}
```

The main caller is a stress testing tool. During the test, it will establish 100 connections to the server, and the body size of each request is 10Byte.

### Throughput test results

| | QPS/w | AVG latency/ms | P90 latency/ms | P99 latency/ms | P999 latency/ms |
| ---| ----|------------| ------------| ------------| ------------|
|fiber mode(with 8 worker thread)| 50.8 | 3.36 | 4.69 | 8.36 | 13.47 |
|merge mode(with 8 worker thread)| 57.8 | 1.46 | 3.00 | 9.72 | 17.72 |

It can be seen that the average latency of the merge mode is the lowest and the QPS is the highest. This is because the request processing in the merge mode does not cross threads and the cache miss is the smallest. So it can achieve a very low average latency and a high QPS.

### Long-tail request test results

| without long-tail request / with long-tail request | AVG latency/ms | P99 latency/ms | P999 latency/ms | Rise rate |
| ----| ------------| ------------| ------------| ------------|
|fiber mode(with 8 worker thread)| 0.40/0.47 | 1.31/1.82 | 1.97/3.41 | avg latency: 17.5% ↑, P99 latency: 38.9% ↑ |
|merge mode(with 8 worker thread)| 0.39/0.62 | 1.23/5.86 | 1.92/7.80 | avg latency: 59.0% ↑, P99 latency: 376.4% ↑ |

It can be seen that the fiber mode is less affected by long-tail requests and has strong resistance to interference from long-tail requests, while the merge mode is greatly affected by long-tail latency. This is because in the fiber mode, requests can be processed in parallel by all worker threads, while in the merge mode, the processing of requests by worker threads cannot be parallelized across multiple cores. Once a certain request is processed for a long time, it will affect the processing time of the overall request.