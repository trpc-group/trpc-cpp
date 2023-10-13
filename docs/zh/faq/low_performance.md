## 性能问题排查步骤

按照章节顺序依次检查可能存在的性能问题，如果仍然难以定位，欢迎提issue一起讨论。

## 检查是否在框架现有未解决的性能场景上

比如，future异步调用场景下目前无法使用对象池和pb arena，这块待排期优化。

## 检查使用的框架版本与线程模型

根据不同的业务场景选择对应的线程模型，以获取更好的编程方式和性能，详见 [runtime](../runtime.md)。

## 代码编译阶段排查

### 是否开启编译优化及使用tcmalloc/jemallc

使用O2编译：查看是否加了O2编译选项，通过在".bazelrc"中添加"build --copt=-O2"

可以使用更好的内存分配器tcmalloc/jemalloc：

### 如何使用tcmalloc

在cc_binary目标中，添加tcmalloc动态库链接即可，如下所示：

```python
cc_binary(
    name = "xxx",    
    ...
    linkopts = [
        "/usr/lib64/libtcmalloc_minimal.so",
    ],
)
```

### 如何使用 jemalloc

相比tcmalloc在多核多线程场景下加锁大大减少，且对大块内存(2k~4k)分配效率更高，具体以实际测试为准。

以bazel下的使用jemalloc源码编译的静态库方式为例，首先在WORKSPACE中指定使用的jemalloc：

```python
# rules_foreign_cc
http_archive(
    name = "rules_foreign_cc",
    sha256 = "c2cdcf55ffaf49366725639e45dedd449b8c3fe22b54e31625eb80ce3a240f1e",
    strip_prefix = "rules_foreign_cc-0.1.0",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/0.1.0.zip",
)

load("@rules_foreign_cc//:workspace_definitions.bzl", "rules_foreign_cc_dependencies")
rules_foreign_cc_dependencies()

# jemelloc
http_archive(
    name = "com_github_jemalloc",
    build_file = "@//third_party/jemalloc:BUILD",
    strip_prefix = "jemalloc-5.2.1",
    sha256 = "461eee78a32a51b639ef82ca192b98c64a6a4d7f4be0642f3fc5a23992138fd5",
    urls = [
        "https://github.com/jemalloc/jemalloc/archive/5.2.1.zip",
    ],
)
```

在third_party/jemalloc下添加BUILD文件，定义"libjemalloc"目标：

```python
load("@rules_foreign_cc//tools/build_defs:configure.bzl", "configure_make")

package(
  default_visibility = ["//visibility:public"],
)

filegroup(
  name = "all",
  srcs = glob(["**"]),
  visibility = ["//visibility:public"]
)


# for global
configure_make(
  name = "libjemalloc",
  autogen = True,
  configure_in_place = True,
  configure_options = [],
  lib_source = "@com_github_jemalloc//:all",
  linkopts = [
    "-lpthread",
    "-ldl",
  ],
  static_libraries = [
    "libjemalloc.a",
  ],
)
```

在cc_binary目录中的deps中添加jemalloc依赖：

```python
deps = [
    ...
    "@com_github_jemalloc//:libjemalloc",
],
```

### 开启 pb arena

见 [tRPC-Cpp开启arena优化](../pb_arena.md)。

### 使用对象池

如果性能瓶颈在内存分配上，针对频繁分配的对象，可以考虑使用对象池来减少开销。

目前框架提供了TLS及share-nothing两种形式的对象池，具体使用方式见 [文档完善中](../object_pool.md)。

### 关闭不必要的功能编译选项

如果有些特性在线上运行时不需要，请移除。

比如您测试环境中使用了rpcz，但线上开启会影响性能，您需要去掉此特性的相关编译选项。具体是在 .bazelrc 文件或者命令行参数里，去掉 `--define=trpc_include_rpcz=true`。

## 业务自身代码检查

- 不要使用同步堵塞线程的操作，比如sleep/usleep，调用同步堵塞的第三方api。
- 使用future.Then方式让执行流程异步化，不要使用同步网络调用以及future的BlockingGet，避免不必要的阻塞。
- 在fiber执行单元中实现同步的话，请使用FiberMutex、FiberConditionVariable、FiberLatch等同步原语。避免阻塞当前fiber worker线程。
- 减少不必要的内存分配：优先使用栈变量、内层小对象直接和外层大对象一起分配(减少调用new的次数)。

## 框架配置项检查

### 开启绑核

默认不绑核，可通过配置globa下的enable_bind_core: true开启绑核

### 线程模型配置

- IO/Handle合并模型：io_thread_num一般配置成机器的CPU核数
- IO/Handle分离模型：一般IO/Handle线程数之和为机器的CPU核数，而IO和Handle线程数需要根据实际业务场景配置（可使用top -Hp pid命令查看各个io及handle线程的cpu占比）
- Fiber m:n协程模型：见 [fiber配置](../fiber_user_guide.md)

### 使用连接复用

如果协议支持请求唯一id的话，则可以使用连接复用来获取更好的网络通信性能。

配置方式：设置serviceproxy option的is_conn_complex: true(不配置的话默认使用连接复用)。

如果不支持使用连接复用的话，需要检测下连接池的max_conn_num配置(客户端默认为64)是否合理，可以根据实际场景进行调大（需要注意是否会将服务端的连接数打满）。

### redis下使用pipeline

可以通过设置serviceproxy option的support_pipeline:true来开启pipeline,提升redis调用性能

### 日志级别

设置合适的日志级别（通常INFO及以上），避免日志频繁打印带来开销。

### 关闭不必要的插件

可以通过关闭不需要使用的插件来消除对应插件的运行开销

## 代码运行阶段排查

通过使用性能分析工具，确定问题和性能瓶颈，一些常用的工具如下：

- top：如使用top -Hp pid查看各线程的CPU使用情况。
- perf：如使用perf top -p pid查看进程的耗时开销，或者使用其来生成火焰图
- tvar：框架提供的计数器功能，可以使用框架提供的tvar来记录各个代码块的执行耗时，详见 [tvar](../tvar.md)
- rpcz：框架提供的用来查看请求及响应处理中各环节的耗时请求的功能，详见 [rpcz](../rpcz.md)
