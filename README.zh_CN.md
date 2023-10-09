[English](README.md) | 中文

# tRPC-Cpp

tRPC-Cpp是tRPC开发框架的cpp语言版本，整体遵循tRPC的设计原则，目标是做一个高性能、可插拔的rpc开发框架。

## 架构设计

![architecture design](docs/images/arch_design.png)

详细介绍见：[架构设计](docs/zh/architecture_design.md)

## 特点

* Runtime
  * 线程模型: `fiber(m:n coroutine)` and `thread(io/handle merge or separate)`
  * io模型: `reactor(for network)` and `async-io(for disk)`
* 服务端
  * 网络传输: 支持 `tcp/udp/ssl/unix domain socket`
  * 开发方式: 支持 `rpc/stream-rpc/non-rpc`
* 客户端
  * 网络传输: 支持 `tcp/udp/ssl`
  * 连接方式: 支持 `长连接(连接复用/连接池/pipeline）/短连接`
  * 调用方式: 支持 `rpc/stream-rpc/non-rpc/one-way`
* 插件
  * 协议: 支持 `trpc/http(s/2.0)/grpc/...`
  * 序列化/反序列化: 支持 `pb/flatbuffers/json/noop(text or binary)`
  * 解压缩: 支持 `gzip/zlib/snappy/lz4`
  * 名字服务: 支持 `polarismesh`
  * 配置中心: 支持 `etcd`
  * 远程日志: 支持 `cls`
  * 监控系统: 支持 `prometheus`
  * 调用链系统: 支持 `jaeger`
  * 可观测系统: 支持 `opentelemetry`
* 工具
  * 支持: `admin/tvar/rpcz`
  * proto IDL依赖管理
* 组件
  * redis

## 如何使用

### 文档

- [编译安装](docs/zh/setup_env.md)
- [快速开始](docs/zh/quick_start.md)
- [基本教程](docs/zh/basic_tutorial.md)
- [用户指南](docs/README.md)
- [api接口]()

### 示例代码

- [helloworld demo](examples/helloworld)
- [various features demo](examples/features)

## 如何贡献

如果您有兴趣进行贡献，请查阅[贡献指南](CONTRIBUTING.md)并检查 [issues][] 中未分配的问题。认领一个任务，让我们一起为 tRPC-Cpp 做出贡献。