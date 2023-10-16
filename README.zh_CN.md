[English](README.md) | 中文

# tRPC-Cpp

[![API](https://img.shields.io/badge/api-latest-green)]()
[![Docs](https://img.shields.io/badge/docs-latest-green)]()
[![LICENSE](https://img.shields.io/badge/license-Apache--2.0-green.svg)](https://github.com/trpc-group/trpc-cpp/blob/main/LICENSE)
[![Releases](https://img.shields.io/github/release/trpc-group/trpc-cpp.svg?style=flat-square)](https://github.com/trpc-group/trpc-cpp/releases)
[![Build Status](https://github.com/trpc-group/trpc-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/trpc-group/trpc-cpp/actions/workflows/ci.yml)
[![Coverage](https://codecov.io/gh/trpc-group/trpc-cpp/branch/main/graph/badge.svg)](https://app.codecov.io/gh/trpc-group/trpc-cpp/tree/main)

tRPC-Cpp是tRPC开发框架的cpp语言版本，整体遵循tRPC的设计原则。

## 整体架构

![architecture design](docs/images/arch_design.png)

总体架构由"**框架核心**"和"**插件**"两部分组成. 如上图所示, 虚线框内为tRPC, 其中中间的红色实线框为框架核心, 蓝色框为插件部分.

其中框架核心又可以分三层:

- **运行层**: 由线程模型和IO模型组成, 负责框架任务的调度和IO的处理, 其中线程模型目前支持: 普通线程模型（分为io和handle分离或者合并的线程模型）、M:N协程模型（fiber线程模型）, IO模型目前支持: 用于网络IO的Reactor模型和用于磁盘IO得AsyncIO模型（基于io-uring, 目前只支持在合并线程模型使用）;

- **通信层**: 负责数据的传输和协议的编解码. 框架内置支持tcp/udp/unix-socket等通信协议, 传输协议采用基于proto的tRPC协议来承载RPC调用, 支持通过codec插件来使用其它传输协议;

- **调用层**: 封装服务和服务代理实体, 提供RPC调用接口, 支持业务用同步、异步、单向以及流式调用等方式进行调用;

此外框架还提供了admin管理接口, 方便用户或者运营平台可以通过调用admin接口对服务进行管理。 管理接口包括更新配置、查看版本、修改日志级别、查看框架运行时信息等功能，同时框架也支持用户自定义管理接口，以满足业务定制化需求.

详细介绍见：[架构设计](docs/zh/architecture_design.md)

## 特点

- Runtime
  - 线程模型: 支持 `fiber(m:n coroutine)` and `thread(io/handle merge or separate)`
  - io模型: 支持 `reactor(for network)` and `async-io(for disk)`
- 服务端
  - 网络传输: 支持 `tcp/udp/ssl/unix domain socket`
  - 开发方式: 支持 `rpc/streaming-rpc/non-rpc`
- 客户端
  - 网络传输: 支持 `tcp/udp/ssl`
  - 连接方式: 支持 `长连接(连接复用/连接池/pipeline）/短连接`
  - 调用方式: 支持 `rpc/streaming-rpc/non-rpc/one-way`
- 插件
  - 协议: 支持 `trpc/http(s/2.0)/grpc/...`
  - 序列化/反序列化: 支持 [pb/flatbuffers/json/noop(text and binary)](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/serialization.md)
  - 解压缩: 支持 [gzip/zlib/snappy/lz4](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/compression.md)
  - 名字服务: 支持 [polarismesh](https://github.com/trpc-ecosystem/cpp-naming-polarismesh)
  - 配置中心: 支持 [etcd](https://github.com/trpc-ecosystem/cpp-config-etcd)
  - 远程日志: 支持 [cls](https://github.com/trpc-ecosystem/cpp-logging-cls)
  - 监控系统: 支持 [prometheus](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/prometheus_metrics.md)
  - 调用链系统: 支持 [jaeger](https://github.com/trpc-ecosystem/cpp-tracing-jaeger)
  - 可观测系统: 支持 [opentelemetry](https://github.com/trpc-ecosystem/cpp-telemetry-opentelemetry)
- 工具
  - 支持: [admin](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/admin_service.md)/[tvar](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/tvar.md)/[rpcz](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/rpcz.md)
  - [proto IDL依赖管理](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/proto_management.md)
- 组件
  - [redis](https://github.com/trpc-group/trpc-cpp/blob/main/docs/zh/redis_client_guide.md)

## 如何使用

### 文档

- [编译安装](docs/zh/setup_env.md)
- [快速开始](docs/zh/quick_start.md)
- [基本教程](docs/zh/basic_tutorial.md)
- [用户指南](docs/README.zh_CN.md)
- [API接口](https://trpc-group.github.io/trpc-cpp.github.io/)

### 示例代码

- [helloworld demo](examples/helloworld)
- [various features demo](examples/features)

## 如何贡献

如果您有兴趣进行贡献，请查阅[贡献指南](CONTRIBUTING.zh_CN.md)并检查 [issues](https://github.com/trpc-group/trpc-cpp/issues) 中未分配的问题。认领一个任务，让我们一起为 tRPC-Cpp 做出贡献。

## 建议反馈

* bug、疑惑、修改建议都欢迎提在[Issues](https://github.com/trpc-group/trpc-cpp/issues)中.
