
Welcome to use tRPC-Cpp. The tRPC-Cpp framework is the cpp version of tRPC. It follows the overall design principles of tRPC. It is an RPC framework designed with high performance and pluggable.

## What tRPC-Cpp can do

You can use it:
- You can build support multiple protocols (one port can only correspond to one protocol) services ([trpc](https://github.com/trpc-group/trpc-cpp/blob/main/docs/en/trpc_protocol_service.md)/[http(s)](https://github.com/trpc-group/trpc-cpp/blob/main/docs/en/http_protocol_service.md)/[grpc](https://github.com/trpc-group/trpc-cpp/blob/main/docs/en/grpc_protocol_service.md) etc.), and can handle client requests by synchronized/asynchronously .
- You can access various protocol backend services ([trpc](https://github.com/trpc-group/trpc-cpp/blob/main/docs/en/trpc_protocol_client.md)/[http(s)](https://github.com/trpc-group/trpc-cpp/blob/main/docs/en/http_protocol_client.md)/[grpc](https://github.com/trpc-group/trpc-cpp/blob/main/docs/en/grpc_protocol_client.md) etc.) synchronously, asynchronously and one-way, and call various storage systems (redis, etc.), and automatically integrate monitoring/tracing capabilities, making service development and operation more convenient and simple.
- Support streaming rpc programming([trpc streaming](https://github.com/trpc-group/trpc-cpp/blob/main/docs/en/trpc_protocol_streaming_service.md), [grpc streaming](https://github.com/trpc-group/trpc-cpp/blob/main/docs/en/grpc_protocol_streaming_service.md), [http streaming upload/download](https://github.com/trpc-group/trpc-cpp/blob/main/docs/en/http_protocol_upload_download_service.md) etc.), to implement streaming applications like push, file upload, video/voice, etc.
- Support various protocols and connect with service management systems by plugin, such as: developing custom protocols, connect with naming/metrics/tracing/config/logging systems, etc., to facilitate service interoperability and service operation.
- Different application scenarios can choose different runtime models to meet the performance requirements of different business scenarios such as io-bound, cpu-bound, stateful business logic, and disk storage. For example: io-bound scenarios (such as : business access gateway/nosql storage) can choose merge mode, cpu-bound scenarios (such as: recommendation/search, etc.) can choose fiber (m:n coroutine), and business stateful scenarios (such as: the single-threaded mode commonly used in game business) can choose separation mode.
- You can manage and debug services through admin interface.


## Features

- Easy to expand

Pluggable design is the biggest feature of the framework. Through the pluggable design of filter and plugin factory, the framework can support various protocols, solve the problem of interoperability with different services, and can connect with various service management systems to better solve the problem of service operation and maintenance, it is also convenient for users to implement customized development and solve the needs of different business personalization.

- high performance

Conventional framework performance test data is only performance data in relatively simple scenarios, and does not represent good performance in real and different business scenarios. Considering that Tencent has many different business scenarios, the requirements for framework performance are also different, for example:
- business accesses gateway scenarios: the feature is that the business logic is light, hign qps, large number of connections (long/short connections), heavy network io operations, and asynchronous programming is often used for programming;
- recommendation/search scenarios: the feature is that the business logic is heavy, the qps is not large, each request needs to be calculated in parallel, pay attention to the long tail delay, and the programming often use synchronous programming;
- game business scenarios: the feature is that logic very complex and  has stateful, large qps, single-threaded programming, and use synchronous programming;
- storage scenarios:  the feature is that the business logic is light, large qps, low latency requirement, heavy network io and disk io operations, and asynchronous programming is often used for programming;
-...

Therefore, in terms of high performance, tRPC pays more attention to meeting the performance requirements of users in different application scenarios.

In terms of framework design, we abstractly designed a layer of runtime, using pluggable ideas to expand and support multiple threading models, currently supporting io/handle merged or separated threading model, and fiber (m:n coroutine) threading model, to meet the performance requirements of different business scenarios. For example, in heavy io business scenarios of business gateways and storage types, the thread model of io/handle merge or separation is generally choosed, and in heavy cpu business scenarios of recommendation/search types, fiber( m:n coroutine) threading model is generally choosed, while game-like business logic stateful scenarios generally choose a single-threaded model.

In terms of specific implementation, we also optimize the performance of the framework from several factors that mainly affect the performance of the framework (cpu/memory/io, etc.), such as: reduction of lock conflicts in task scheduling under multi-threading, network io to rpc data zero copy, memory pool/object pool, concurrent writing fd, etc. In addition, we have borrowed the performance optimization technical ideas of industry frameworks(seastar/brpc, etc.).

- Ecologically rich

At present, most of Tencent's internal communication protocols and service management systems have supported by plugin, and also support ecosystems(such as: redis/etcd/promethues/opentelemetry, etc.), the business choose to use what you need.

## How to use tRPC-Cpp

Before you get started, you should have basic theoretical knowledge, including but not limited to:
- [RPC Concepts](https://cloud.tencent.com/developer/article/1343888), calling remote service interfaces is like calling local functions and can make it easier for you to create distributed applications.

- [tRPC Terminology](https://github.com/trpc-group/trpc/blob/main/docs/en/terminology.md) Introduction, as it is important to understand the core concepts in tRPC design in advance, especially the meaning of Service Name and Proto Name, and their interrelationship.

- [proto3 knowledge](https://developers.google.com/protocol-buffers/docs/proto3), a cross-language protocol describing the service interface, is simple, convenient, and universal.

With the above basic theoretical knowledge in mind, we recommend learning tRPC-Cpp in the following order:

- [Quick start](quick_start.md): through a simple Hello World example, establish a preliminary understanding of tRPC-Cpp, and understand the basic process of developing a background service.

- [Development specifications](../../DEVELOP_SPECIFICATIONS.md): be sure to follow the tRPC-Cpp development specification, especially the code specification inside.

- [User Guide](../README.md): the above steps have enabled you to develop simple services, but not enough, advanced knowledge needs to continue to read in detail to deal with a variety of complex scenarios.

- [FAQ](): If you encounter a problem, you should check the FAQ first, and raise an issue if you can't solve it.
