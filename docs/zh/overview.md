
欢迎大家使用tRPC-Cpp, tRPC-Cpp框架是tRPC的cpp版本, 遵循tRPC的整体设计原则, 主要是以高性能, 可插拔为出发点而设计的RPC框架。

## tRPC-Cpp可以做什么

你可以使用它：
- 搭建多个端口支持多个协议（一个端口只能对应一个协议）的服务([trpc](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/trpc_protocol_service.md)/[http(s)](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/http_protocol_service.md)/[grpc](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/grpc_protocol_service.md)等), 并能同步/异步处理客户端请求。
- 可以以同步、异步、单向的方式访问各种协议后端服务([trpc](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/trpc_protocol_client.md)/[http(s)](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/http_protocol_client.md)/[grpc](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/grpc_protocol_client.md)等), 调用各种存储系统(redis等), 并自动集成监控/调用链等能力, 服务开发和运维更方便更简单.
- 可以流式rpc编程, 支持[trpc流式](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/trpc_protocol_streaming_service.md), [grpc流式](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/grpc_protocol_streaming_service.md), [http流式上传/下载](https://github.com/trpc-group/trpc-cpp/blob/main/docs/en/http_protocol_upload_download_service.md)等, 实现类似push, 文件上传, 视频/音乐等流式应用服务.
- 可以插件化支持各种协议和对接服务治理系统, 比如: 开发自定义的协议、对接业务各种的名字服务/监控系统/调用链系统/配置系统/日志系统等, 方便服务互通和服务运营.
- 不同应用场景可以选择不同的runtime模型, 满足重网络IO的转发、重计算、重有状态的业务逻辑、重磁盘IO的存储等不同业务场景的性能需求. 比如: io-bound的场景（比如: 业务接入网关类/nosql存储）可以选择merge合并模式的runtime, cpu-bound类的场景（比如: 推荐/搜索等）可以选择fiber（m:n协程）的runtime, 还有业务逻辑服务且有状态的场景（比如: 游戏业务常用的单线程模式）可以选择separate分离模式的runtime.
- 可以通过admin管理服务和调试服务.

## tRPC-Cpp的特色

- 易扩展

插件化设计是框架最大的特色, 通过filter和插件工厂的插件化设计, 框架可以支持各种不同协议, 解决与不同服务互通的问题, 可以对接各种服务治理系统, 更好得解决不同运营体系下服务运维的问题, 还可以方便用户进行定制化的开发, 解决不用业务个性化的需求.

- 高性能

常规的框架性能测试数据只是比较简单场景下的性能数据, 并不能代表在真实不同业务场景下也能有很好的表现. 考虑到腾讯的业务形态比较众多, 对框架性能的需求也各不相同, 比如:
- 后台业务接入网关类的场景: 特点是业务逻辑轻, qps量大, 连接数多(大量的长连接/短连接), 重网络io操作, 编程常采用异步编程;
- 后台推荐/搜索类的场景: 特点是业务逻辑重, qps不大, 每个请求需要并行计算, 注重长尾延时, 编程常采用同步编程;
- 后台游戏业务类的场景: 特点是业务逻辑复杂且有状态, qps量大, 使用单线程模式编程, 编程常采用同步编程;
- 后台存储类的场景: 特点是业务逻辑轻, qps量大, 延时要求高, 重网络io和磁盘io操作, 编程常采用异步编程;
- ...

因此tRPC在高性能这块, 更多得注重能满足不同应用场景下用户的性能需求. 

在框架设计上, 我们抽象设计了一层runtime, 采用插件化的思路扩展支持支持多种线程模型, 目前支持io/handle合并或者分离的线程模型, 以及fiber(m:n协程)的线程模型, 满足不同业务场景下的性能需求. 比如: 业务网关类和存储类的重io业务场景一般会选择io/handle合并或者分离的线程模型, 推荐/搜索类的重cpu业务场景一般会选择fiber(m:n协程)的线程模型, 而游戏类的业务逻辑有状态类场景一般选择单线程模型.

在具体实现上, 我们也从主要影响框架性能因素的几个方面因素(cpu/内存/io等), 对框架的性能进行优化, 比如: 多线程下任务调度的锁冲突减少、网络io到rpc处理过程中的数据零拷贝、内存池/对象池、fd的并发写等等，另外我们借鉴了业界框架(seastar/brpc等)的性能优化技术思想。

- 生态丰富

目前对腾讯内部大部分的通信协议和服务治理系统, 都开发了相应的插件, 同时也针对业界主流的生态系统(比如: redis/etcd/promethues/opentelemetry等)进行了支持, 业务只需要在使用上选择自己需要的即可.

## 如何使用tRPC-Cpp

在真正开始使用之前, 首先需要掌握基本理论知识, 包括但不限于：
- [RPC 概念](https://cloud.tencent.com/developer/article/1343888)，调用远程服务接口就像调用本地函数一样，能让你更容易创建分布式应用。
- [tRPC 术语介绍](https://github.com/trpc-group/trpc/blob/main/docs/zh/terminology.md)，必须提前了解 tRPC 设计中的核心概念，尤其是 Service Name 和 Proto Name 的含义，以及相互关系。
- [proto3 知识](https://developers.google.com/protocol-buffers/docs/proto3)，描述服务接口的跨语言协议，简单，方便，通用。

掌握好以上基本理论知识以后，建议按以下推荐顺序开始学习 tRPC-Cpp：
- [快速上手](quick_start.md)：通过一个简单的 Hello World 例子初步建立对 tRPC-Cpp 的认识, 了解开发一个后台服务的基本流程.
- [研发规范](../../DEVELOP_SPECIFICATIONS.zh_CN.md)：务必一定遵守 tRPC-Cpp 研发规范, 特别是里面的代码规范.
- [用户指南](../README_zh.md)：通过以上步骤已经能够开发简单服务，但还不够，进阶知识需要继续详细阅读以应对各种各样的复杂场景.
- [常见问题]()：碰到问题应该首先查看常见问题, 如果不能解决再提issue.
