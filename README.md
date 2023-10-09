English | [中文](README.zh_CN.md)

# tRPC-Cpp

**tRPC-Cpp is the cpp version of tRPC. It follows the overall design principles of tRPC. It is mainly an RPC framework designed with high performance and plug-in.**

## Architecture Design

![architecture design](docs/images/arch_design.png)

more details：[architecture design](docs/en/architecture_design.md)

## Features

* Runtime
  * thread-model: `fiber(m:n coroutine)` and `thread(io/handle merge or separate)`
  * io-model: `reactor(for network)` and `async-io(for disk)`
* Server
  * network: support `tcp/udp/ssl/unix domain socket`
  * rpc-impl: support `rpc/stream-rpc/non-rpc`
* Client
  * network: support `tcp/udp/ssl`
  * connection-way: support `conn-complex/conn-pool/conn-pipeline`
  * rpc-call: support `rpc/stream-rpc/non-rpc/one-way`
* Plugin
  * protocol(codec): support `trpc/http(s/2.0)/grpc/...`
  * serialization/deserialization: support `pb/flatbuffers/json/noop(text or binary)`
  * compressor/decompressor: support `gzip/zlib/snappy/lz4`
  * naming: support `polarismesh`
  * config: support `etcd`
  * logging: support `cls`
  * metrics: support `prometheus`
  * tracing: support `jaeger`
  * telemetry: support `opentelemetry`
* Tools
  * support: `admin/tvar/rpcz`
  * proto IDL dependency management
* Components
  * redis

## To start using tRPC

### Docs

- [compile & install](docs/en/setup_env.md)
- [quick_start](docs/en/quick_start.md)
- [basic_tutorial](docs/en/basic_tutorial.md)
- [user_guide](docs/README.md)

### Examples

- [helloworld demo](examples/helloworld)
- [various features demo](examples/features)

## How to contribute

If you're interested in contributing, please take a look at the [CONTRIBUTING](CONTRIBUTING.md) and check the [unassigned issues][issues] in the repository. Claim a task and let's contribute together to tRPC-Cpp.