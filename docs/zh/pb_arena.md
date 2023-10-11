[English](../en/pb_arena.md)

# RPC 服务使用 PB-Arena 优化

# 前言

本文主要是介绍 RPC 服务如何使用 arena 优化，开发者可以了解到如下内容：

* arena 原理。
* 如何在 RPC 服务端使用 arena。
* 如何在调用 RPC 服务时使用 arena。
* FAQ。

# arena 原理

[arena](https://developers.google.com/protocol-buffers/docs/reference/overview)
是 protobuf 3.0 版本开始支持的一个特性，它用来接管 PB 对象的生命周期；使用它可以帮助你优化程序的内存使用、提高性能。

建议先阅读：[Why Use Arena Allocation?](https://developers.google.com/protocol-buffers/docs/reference/arenas#why)

原理：
arena 是由 protobuf 库去接管 PB 对象的内存管理。
它的原理很简单，protobuf 预先分配一个内存块，多次创建的 PB 对象复用这个内存块。
解析消息和构建消息等触发对象创建时是在已分配好的内存块上
[placement new](https://github.com/protocolbuffers/protobuf/blob/v3.8.0/src/google/protobuf/arena.h#L611) 出来；
arena 上的对象析构时会释放所有内存，理想情况下不需要运行任何被包含对象的析构函数。

好处：

* 减少复杂的 PB 对象中多次 malloc/free 和析构带来的系统开销。
* 减少内存碎片。
* PB 对象的内存连续，cache line 友好、读取性能高。

适用场景：PB 结构比较复杂，repeated 类型字段包含的数据个数比较多。

# 如何在 RPC 服务端使用 arena

PB 对象的生命周期由框架管理，可通过编译选项开启 arena 优化。

## 启用 arena

**使用 proto option 开启 arena**
我们需要在每个`.proto`文件中启用 arena 分配，因此需要在每个`.proto`文件中添加如下 ` option`：

```protobuf
option cc_enable_arenas = true;
```

**编译时开启 arena 选项**

在`.bazelrc` 中添加如下编译选项：

```
build --define trpc_proto_use_arena=true
```

执行以上两个步骤就可以开启 arena 优化。

## 设置自定义的 arena 生成函数 （可选项）

默认情况下，框架会调 arena 的默认构造函数生成 arena 对象。如果你期望设置自定义的 arena 生成函数，可以参考下面的方式。
*小提示：不设置自定义的 arena 生成函数`不影响使用 arena 特性`*

```cpp
/// @brief Set the ArenaOptions
///        the user can set this function to realize the function of customizing Arena's Options
///        if not set, use default option
void SetGlobalArenaOptions(const google::protobuf::ArenaOptions& options);
```

函数需要返回一个 `google::protobuf::Arena` 对象
([RVO](https://en.cppreference.com/w/cpp/language/copy_elision) 没有性能损失 )，
业务可以自行通过[google::protobuf::ArenaOptions](https://github.com/protocolbuffers/protobuf/blob/master/src/google/protobuf/arena.h#L130)
来构造 arena 对象并指定初始内存大小等参数。

在业务的 `Initialize()`中注册全局 `arena` 配置即可，下面给一个 Demo (指定每次生成 arena 时初始内存大小是 10K)：

```cpp
int XxxxServer::Initialize() {
    // 自行根据业务特点设置 options 的参数
    // 此处只设置了 start_block_size，其他参数业务自行研究
    google::protobuf::ArenaOptions options;
    options.start_block_size = 10240;

    SetGlobalArenaOptions(options);
}
```

# 如何在调用 RPC 服务时使用 arena

这种情况下，PB 对象由用户创建，PB 对象的生命周期由用户代码管理，`框架没法干预 PB 对象的生命周期`，因此需要用户自行使用 arena
优化内存。

## 启用 arena

**使用 proto option 开启 arena**
我们需要在每个`.proto`文件中启用 arena 分配，因此需要在每个`.proto`文件中添加如下 ` option`：

```protobuf
option cc_enable_arenas = true;
```

**创建 PB 对象**

在客户端调用 RPC 服务时，如果需要使用 arena，我们需要主动调用`google::protobuf::Arena::CreateMessage<MyMessage>(...)`接口。
在 RPC 服务端时，不需要上述步骤，只需要通过编译选项来开启 arena。

```cpp
// 包含 arena 头文件
#include "google/protobuf/arena.h"

google::protobuf::Arena arena_req;
auto* req = google::protobuf::Arena::CreateMessage<MyRequest>(&arena_req);

google::protobuf::Arena arena_rsp;
auto* rsp = google::protobuf::Arena::CreateMessage<MyResponse>(&arena_rsp);
```

第 2 行：包含 arena 需要的头文件。

第 4~7 行：定义了两个 arena 对象，分别管理 req 和 rsp 的内存。

第 5 行和第 8 行：用两个 arena 分别创建两个 PB 对象。

## 注意事项

arena 维护的所有对象都是在 arena 析构的时候统一释放的。在使用过程中，它内部维护的内存块只会不断地append，并不会删除。
所以，这也决定了由 arena 维护的对象要么只能是局部对象，要么是不可变的。否则它的内存会无限增长下去。

- 建议 arena 对象不要重复使用，一个 arena 管理一个 PB 对象的生命周期。
    - 重复使用可能会导致 arena 扩容，影响性能。
    - 一个 arena 生成多个 PB 对象，生命周期不好管理。
- arena 对象的生命周期要么是局部的、要么不可变。
- arena 的生命周期要比所管理的对象长。
- CreateMessage 创建出来的 PB 对象禁止删除。

如果使用不当，会导致服务出现 OOM、Core dump，需要熟悉原理后再使用。

# 性能数据

经测试，在 PB 结构较大且复杂时，使用 arena 后，性能提升约`14%`。

**测试设备配置**

```
// 云主机
Architecture:          x86_64
CPU(s):                8
On-line CPU(s) list:   0-7
Thread(s) per core:    1
Model name:            AMD EPYC 7K62 48-Core Processor
Stepping:              0
CPU MHz:               2595.124
Hypervisor vendor:     KVM
L1d cache:             32K
L1i cache:             32K
L2 cache:              4096K
L3 cache:              16384K
```

**测试逻辑**

为了模拟真实的场景，我们借用某业务的 proto 文件定义。
测试逻辑：

* 并发起 4 个线程线程，每个线程循环执行 500 次任务。
* 在每个任务中，创建 Request 和 Response 两个复杂 PB 结构体，并填充 repeated 字段，使得结构体的 ByteSizeLong() 大概是 200K。

**编译选项**

开启 O2 编译，区分是否使用 tc_malloc 库。

**测试结果**

|           | 不链接 tc_malloc | 链接tc_malloc |
|-----------|---------------|-------------|
| 不使用arena  | 6024ms        | 5129ms      |
| 使用arena优化 | 5198ms        | 4510ms      |
| arena提升比例 | 13.8％         | 14.1%       |

# FAQ

## 1 程序崩溃，提示：`CHECK failed: GetArenaNotVirtual() == nullptr`
使用 CreateMessage 创建出来的 PB 对象`禁止删除`，检查下是否析构了 PB 对象。
只有析构了 CreateMessage 创建出来的对象会触发这个错误。
