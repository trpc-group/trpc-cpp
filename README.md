English | [中文](README.zh_CN.md)

# tRPC-Cpp

**tRPC-Cpp is the cpp version of tRPC, it follows the overall design principles of tRPC. **

## Overall Architecture

![architecture design](docs/images/arch_design.png)

The overall architecture consists of two parts: "**framework core**" and "**plugins**". As shown in the figure above, the dotted box is tRPC. The red solid line box in the middle is the framework core, and the blue box is the plugin part.

The framework core can be divided into three layers:

- **Runtime**: It consists of thread model and io model, responsible for scheduling of framework cpu tasks and io operations. thread model currently supports: ordinary thread model (io/handle separated or merged thread model), M:N coroutine model (fiber thread model). io model currently supports: reactor model for network IO and asyncIO model for disk io(based on io-uring, currently only supported in the merged thread model);

- **Transport**: Responsible for data transmission and protocol encoding and decoding. The framework currently supports tcp/udp/unix-socket and uses the tRPC protocol to carry RPC messages, and also supports other protocol through codec plugin;

- **RPC**: Encapsulates services and service proxy entities, provides RPC interfaces and support synchronous, asynchronous, one-way, and streaming calls;

In addition, the framework also provides an admin management interface, which is convenient for users or operating platforms to manage services. The management interface includes functions such as updating configuration, viewing version, modifying log level, viewing framework runtime information, and the framework also supports user  interface to meet the customized needs.

more details：[architecture design](docs/en/architecture_design.md)

## Features

* Runtime
  * thread-model: support`fiber(m:n coroutine)` and `thread(io/handle merge or separate)`
  * io-model: support `reactor(for network)` and `async-io(for disk)`
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

If you're interested in contributing, please take a look at the [CONTRIBUTING](CONTRIBUTING.md) and check the [unassigned issues](https://github.com/trpc-group/trpc-cpp/issues) in the repository. Claim a task and let's contribute together to tRPC-Cpp.