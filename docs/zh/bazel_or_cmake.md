[English](../en/bazel_or_cmake.md)

# 使用Bazel构建项目

[bazel](https://bazel.build/)是一种现代C++项目构建工具，tRPC-Cpp从一开始就使用bazel进行项目构建管理，推荐使用此工具来构建您的项目。

## Bazel工具安装及使用

- 安装见[setup_env](setup_env.md)。
- 常见命令：
  - 可以通过 `bazel build ...` 构建所有二进制及库。
  - 可以通过 `bazel build //relative-path-from-root:bazel_target` 来构建指定的二进制及库。
  - 可以通过在 .bazelrc 或者 命令行里添加开启带符号表的debug编译 `build --copt=-g --strip=never`。
- 如何使用：
  - 见[bazel核心概念](https://bazel.build/concepts/build-ref)。
  - 见[bazel在cpp下的例子](https://bazel.build/tutorials/cpp-use-cases)。

## trpc_proto_library的使用

### 简介

可以在proto文件里，定义服务及其RPC接口，tRPC-Cpp通过bazel的机制，提供了 trpc_proto_library 管理带复杂依赖关系的proto文件，它能基于proto文件生成适用于tRPC-Cpp的RPC桩代码以及protobuf的C++桩代码。

在任意的bazel BUILD文件，可以把proto文件引入到项目里，如以下的简单例子：

```python
# 使用 trpc_proto_library 引入 proto 文件
trpc_proto_library(
    name = "hello_proto",
    srcs = ["hello.proto"],
    use_trpc_plugin = True,
    rootpath = "@trpc_cpp",
    deps = [
      # 可依赖其他通过trpc_proto_library描述的bazel规则
    ],
)

# 使用 cc_library 或其他cpp相关bazel规则引入 trpc_proto_library
cc_library(
    name = "user_lib",
    srcs = ["xxx.cc"],
    hdrs = ["xxx.h"],
    deps = [":hello_proto"]
)
```

PS: *编译后，生成的桩代码可以在 bazel-bin 下项目里对应proto文件路径下找到*

### 完整选项及说明

trpc_proto_library支持的完整选项如下：

```python
trpc_proto_library(
    name = "xxx_proto",
    srcs = ["xxx.proto"],
    use_trpc_plugin = True,
    rootpath = "@trpc_cpp",
    generate_new_mock_code = False,
    enable_explicit_link_proto = False,
    deps = [
      # 其他trpc_proto_library的依赖
    ],
    native_proto_deps = [
      # proto_library的依赖
    ],
    native_cc_proto_deps = [
      # cc_proto_library的依赖
    ],
)
```

各选项说明如下：

- name: bazel规则的名字，建议带有后缀 `_proto` 表明此规则可以通过此规则，引入 proto 文件。
- srcs: 文件列表，多个proto文件可以放在一个srcs里（不管是否有依赖关系），带依赖关系的proto，建议声明多个 trpc_proto_library 进行组织。
- use_trpc_plugin: True代表生成tRPC-Cpp的桩代码（包含 \*.trpc.pb.h、\*.trpc.pb.cc、\*.pb.h、\*.pb.cc），False代表只生成pb桩代码（只有\*.pb.h、\*.pb.cc），建议保持为True，只生成pb桩代码的场景使用[bazel cc_proto_library](https://bazel.build/reference/be/c-cpp#cc_proto_library)，不填默认False。
- rootpath: 必填，可随引入 trpc_proto_library 进而引入tRPC-Cpp用于构建服务相关的依赖库，建议保持为 `@trpc_cpp`，代表始终以外部参数进行引入。
- generate_new_mock_code: 选填（True/False，不填默认False），可生成适用于proto文件的tRPC-Cpp Mock桩代码（\*.pb.mock.h）。
- enable_explicit_link_proto: 选填（True/False，不填默认False），用于获取业务自定义的option元数据，见小节 [支持代码里获取proto文件元数据](#支持代码里获取proto文件元数据)。
- deps: 可依赖同样使用 trpc_proto_library 定义的bazel规则。
- native_proto_deps: 可依赖使用 [proto_library](https://bazel.build/reference/be/protocol-buffer#proto_library) 定义的bazel规则，常用于依赖了外部proto库，但proto库里不希望引入tRPC-Cpp框架。
- native_cc_proto_deps: 可依赖使用 [cc_proto_library](https://bazel.build/reference/be/c-cpp#cc_proto_library) 定义的bazel规则，常用于依赖了外部proto库，但proto库里不希望引入tRPC-Cpp框架。

### Bazel编译选项

在业务项目的 .bazelrc 或者 编译命令 里加上 `--define xxxx=true`，支持的系列选项见[tRPC-Cpp支持bazel编译选项](https://github.com/trpc-group/trpc-cpp/blob/main/trpc/BUILD)，建议查阅时切换到您使用的特定tRPC-Cpp版本。

### 使用option扩展

引入[protobuf options](https://protobuf.dev/programming-guides/proto3/#options)支持tRPC下的扩展，需要配合tRPC扩展proto文件使用，目前支持下面的扩展特性：

- alias别名

#### 使用option扩展alias别名

适用场景：常见于注册一个rpc服务，但以http协议提供访问，可以定义http的url。

首先在使用alias的proto文件里import tRPC扩展proto文件，并且标识方法别名，如下所示：

```protobuf
...
// 使用alias需要引入trpc_options文件
import "trpc/proto/trpc_options.proto";

service Greeter {
  rpc SayHello (HelloRequest) returns (HelloReply) {
    option (trpc.alias) = "/cgi-bin/hello"; // 您可以通过 http://domain/cgi-bin/hello 访问服务这个的这个接口
  }
}
...
```

然后在BUILD文件里引入 trpc_options.proto 对应的依赖，如下所示：

```python
trpc_proto_library(
    name = "helloworld_proto",
    srcs = ["helloworld.proto"],
    native_proto_deps = [
      # 引入trpc_options.proto
        "@com_github_trpc_group_trpc//trpc/proto:trpc_options_proto",
    ],
    use_trpc_plugin = True,
)
```

最后在本项目根目录的WORKSPACE里，引入 com_github_trpc_group_trpc 对应的外部库即可，如下所示：

```python
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
git_repository(
    name = "com_github_trpc_group_trpc",
    remote = "https://github.com/trpc-group/trpc.git",
    branch = "main",
)
```

### 其他高级特性

#### 直接通过proto_library或者cc_proto_library生成桩代码

适用场景：引入外部的proto库，proto库所有proto文件均使用 proto_library 或者 cc_proto_library 定义，希望直接基于proto文件生成适用于tRPC-Cpp的桩代码。

下面是个引入的例子:

```python
# BUILD文件路径：test/server/BUILD
trpc_proto_library(
    name = "rpc_service_proto", # 生成桩代码文件名前缀
    srcs = [], # 保持为空
    deps = [], # 保持为空
    use_trpc_plugin = True,
    rootpath = "@trpc_cpp",
    native_proto_deps = [
      # 保持仅存在一个
      "@remote_pb_lib//proto/common:basic_proto",
    ],
    native_cc_proto_deps = [], # 保持为空，或者存在一个，native_proto_deps为空
)
# 生成trpc桩代码路径：bazel-bin/test/server/rpc_service_proto.trpc.pb.h
# 使用：#include "test/server/rpc_service_proto.trpc.pb.h"
```

需要注意的是：

- 生成桩代码路径为定义 trpc_proto_library 的BUILD文件路径，这是因为bazel本身限制导致的。
- 生成桩代码文件名字为 name 字段的值，推荐保持name取值和远程proto文件一致。
- 保持 srcs 和 deps 为空，保持 native_proto_deps 和 native_cc_proto_deps 二者之一仅存在一个proto规则，需要针对多个proto文件生成服务桩代码，需要定义多个 trpc_proto_library。

#### 支持import不带路径的pb文件

适用场景：一般不推荐，除非已经有一堆带依赖关系的proto文件都是这种import，改造困难，常见于import同个目录下proto文件引入同目录下的其他proto文件。

比如存在如下两个proto文件：

```protobuf
// path: proto/svc/basic.proto
message HelloRequest {
   string msg = 1;
}
message HelloReply {
   string msg = 1;
}

// path: proto/svc/rpc_svc.proto
import "basic.proto";

service Greeter {
  rpc SayHello (HelloRequest) returns (HelloReply) {}
}
```

BUILD里需要使用 proto_library 及其选项 `strip_import_prefix` 来把import前缀路径替换成空字符串。

```python
// proto/svc/BUILD
trpc_proto_library(
    name = "rpc_svc_proto",
    srcs = ["rpc_svc.proto"],
    use_trpc_plugin = True,
    rootpath = "@trpc_cpp",
    native_proto_deps = [
      ":basic_proto",
    ],
)

proto_library(
    name = "basic_proto",
    srcs = ["basic.proto"],
    strip_import_prefix = "", # 必须填空，以让能import不带路径的pb文件，否则将会编译出错。
)
```

### 支持代码里获取proto文件元数据

适用场景：通过[protobuf options](https://protobuf.dev/programming-guides/proto3/#options)进行业务自定义扩展，需要获取相关元数据类实例，从中获得扩展字段。

在BUILD文件里，开启 trpc_proto_library 的 `enable_explicit_link_proto` 选项，如下所示：

```python
trpc_proto_library(
    name = "xxx_proto",
    srcs = ["xxx.proto"],
    use_trpc_plugin = True,
   enable_explicit_link_proto = True, # 需要添加此选项显示地开启option获取的相关桩代码生成
)
```

目前提供如下两种方式以获取元数据：

- 获取Service的元数据：可通过 `*.trpc.pb.h` 里的Service提供的静态方法 `XXXImplService::GetServiceDescriptor()` 获取 [ServiceDescriptor](https://protobuf.dev/reference/java/api-docs/com/google/protobuf/Descriptors.ServiceDescriptor) 进而获取  [ServiceOptions](https://protobuf.dev/reference/java/api-docs/com/google/protobuf/DescriptorProtos.ServiceOptions)。
- 客户端发起RPC调用后，获取调用方法的元数据：RPC调用后，使用客户端上下文的 `GetProtobufMethodDescriptor` 获取 [MethodDescriptor](https://protobuf.dev/reference/java/api-docs/com/google/protobuf/Descriptors.MethodDescriptor) 进而获取 [MethodOptions](https://protobuf.dev/reference/java/api-docs/com/google/protobuf/DescriptorProtos.MethodOptions)，如下代码所示:

  ```c++
  // 发起下游调用前，获取不到
  TRPC_ASSERT(client_ctx->GetProtobufMethodDescriptor() == nullptr);
  // 发起下游调用，在桩代码里赋值MethodOptions
  ::trpc::Status status = proxy->YourInvokeMethod(client_ctx, request, &reply);
  // 业务侧，在发起下游调用后，才可获取到
  context->GetProtobufMethodDescriptor() != nullptr;
  ```

# 使用CMake构建项目

[CMake](https://cmake.org/)是一种广泛被使用的构建工具，tRPC-Cpp提供了对CMake编译的支持，方便集成到您现有的CMake项目里。

## CMake工具安装及使用

- 安装见[setup_env](setup_env.md)
- 常见命令：
  - 可以通过 `mkdir build && cd build && cmake .. && make -j8` 来构建项目，-j8 代表使用8个cpu核心来编译。
  - 设置编译Debug版本 `cmake -DCMAKE_BUILD_TYPE=Debug ..`。
  - 设置编译Release版本 `cmake -DCMAKE_BUILD_TYPE=Release ..`。
- 更多用法见[CMake tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html)

## 如何引入tRPC-Cpp

### （推荐）以外部库形式源码引入

推荐使用这种方式，因为能很方便切换框架的版本，同时，框架相关依赖可随构建的SDK目标引入。

参考如下示例，在您项目的 CMakeLists.txt 以外部库源码方式引入：

```shell
# 拉取并以库的形式添加tRPC-Cpp
include(FetchContent)
FetchContent_Declare(
    trpc-cpp
    GIT_REPOSITORY    https://github.com/trpc-group/trpc-cpp.git
    GIT_TAG           recommanded_always_use_latest_tag
    SOURCE_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/cmake_third_party/trpc-cpp
)
FetchContent_MakeAvailable(trpc-cpp)

# 设置proto文件桩代码生成工具的路径(PROTOBUF_PROTOC_EXECUTABLE/TRPC_TO_CPP_PLUGIN将会在tRPC-Cpp被引入后自动填充)
set(PB_PROTOC ${PROTOBUF_PROTOC_EXECUTABLE})
set(TRPC_CPP_PLUGIN ${TRPC_TO_CPP_PLUGIN})

# 在您的构建目标里link trpc库
target_link_libraries(your_cmake_target trpc)
```

### 通过make install引入

参考如下命令，先安装trpc到系统里:

```shell
# 可checkout切换到最新的版本
git clone https://github.com/trpc-group/trpc-cpp.git
cd trpc-cpp
mkdir build && cd build

# 默认编译静态库，可以通过cmake选项编译动态库: -DTRPC_BUILD_SHARED=ON
cmake ..
make -j8
make install
```

然后，参考如下步骤，在您项目的 CMakeLists.txt 里，引入trpc库：

```shell
# 设置tRPC-Cpp的安装位置
set(TRPC_INSTALL_PATH /usr/local/trpc-cpp/trpc)

# 加载tRPC-Cpp头文件及库路径
include(${TRPC_INSTALL_PATH}/cmake/config/trpc_config.cmake)
include_directories(${INCLUDE_INSTALL_PATHS})
link_directories(${LIBRARY_INSTALL_PATHS})

# 设置proto文件桩代码生成工具的路径 - 具体使用方式见相关章节
include(${TRPC_INSTALL_PATH}/cmake/tools/trpc_utils.cmake)
set(PB_PROTOC ${TRPC_INSTALL_PATH}/bin/protoc)
set(TRPC_CPP_PLUGIN ${TRPC_INSTALL_PATH}/bin/trpc_cpp_plugin)

# 添加trpc库及其依赖的三方
set(LIBRARY trpc ${LIBS_BASIC})

# 在您的构建目标里link trpc库
target_link_libraries(your_cmake_target trpc)
```

## 生成proto文件桩代码

编译tRPC-Cpp时，也编译了用于生成proto文件对应桩代码的工具，在引入tRPC-Cpp后，可以通过CMake函数

- `COMPILE_PROTO`: 生成 \*.pb.h 和 \*.pb.cc
- `TRPC_COMPILE_PROTO`: 生成 \*trpc.pb.h 和 \*.trpc.pb.cc

它们的使用方式及参数说明参考如下例子：

```shell
# 注意在业务项目的CMakeLists.txt需要设置工具的路径 PB_PROTOC 和 TRPC_CPP_PLUGIN

# 设置proto文件列表，可一次设置多个
set(PB_SRCS ${CMAKE_SOURCE_DIR}/xxx/aaa.proto
           ${CMAKE_SOURCE_DIR}/yyy/bbb.proto)

# COMPILE_PROTO: 根据proto文件生成相应的.pb.cc和.pb.h桩代码
# - param 1: 该参数为传出参数，表示proto生成.pb.cc文件
# - param 2: 该参数表示proto文件集合
# - param 3: 该参数为编译过程中生成的build/bin目录下的protoc可执行程序
# - param 4: 该参数为工程下的CMakeLists.txt所在目录
COMPILE_PROTO(OUT_PB_SRCS "${PB_SRCS}" ${PB_PROTOC} ${CMAKE_SOURCE_DIR})

# TRPC_COMPILE_PROTO: 根据proto文件生成相应的.trpc.pb.cc和.trpc.pb.h桩代码
# - param 1: 该参数为传出参数，表示proto生成.trpc.pb.cc文件
# - param 2: 该参数表示proto文件集合
# - param 3: 该参数为编译过程中生成的build/bin目录下的protoc可执行程序
# - param 4: 该参数为编译过程中生成的build/bin目录下的trpc_cpp_plugin可执行程序
# - param 5: 该参数为工程下的CMakeLists.txt所在目录
TRPC_COMPILE_PROTO(OUT_TRPC_PB_SRCS "${PB_SRCS}" ${PB_PROTOC} ${TRPC_CPP_PLUGIN} ${CMAKE_SOURCE_DIR})

# 添加生成桩代码到构建目标或者库里
add_executable(${TARGET} xxx.cc
                        ${OUT_PB_SRCS}
                        ${OUT_TRPC_PB_SRCS})
```

### CMake编译选项

在 CMake命令执行的时候 加上 `-DXXX=ON/OFF`，支持的系列选项见[tRPC-Cpp支持的CMake编译选项](https://github.com/trpc-group/trpc-cpp/blob/main/CMakeLists.txt)，option定义的即为选项，建议查阅时切换到您使用的特定tRPC-Cpp版本。

如果您是通过 make install 方式引入框架的话，在构建目标时，需要根据您开启的编译选项，引入对应的库，对应库名可以在 [trpc_config.cmake](https://github.com/trpc-group/trpc-cpp/blob/main/cmake/config/trpc_config.cmake) 里找到。

比如，需要开启SSL编译，cmake命令为 `cmake .. -DTRPC_BUILD_WITH_SSL=ON`，同时，在您项目内的CMakeLists.txt里，需要为构建目标链接SSL库，具体地，需要在 `target_link_libraries` 里额外添加 `${LIB_SSL}`。

如果您是通过以外部库形式源码引入，则无需如此组织，这也是我们推荐使用这种方式引入框架的一个原因。
