[English](../en/flatbuffers_protocol_service.md)

[TOC]

# 前言

[flatbuffers](https://google.github.io/flatbuffers/usergroup0.html)是一种类似与protobuf的协议，它具有高效的内存访问特性(具体性能参数请参考[flatbuffers_benchmarks](https://google.github.io/flatbuffers/flatbuffers_benchmarks.html))，满足一些对性能要求苛刻的特定场景，例如：游戏场景。
tRPC-Cpp框架内置桩代码生成工具， 可以根据flatbuffers的IDL文件(以`.fbs`为后缀)生成对应的桩代码，整个原理和tRPC-Cpp框架通过插件生成protobuf类型的桩代码是一样的。

# 编写flatbuffers对应的IDL文件

下面给出两种常见的flatbuffers对应的IDL文件，目前约定IDL文件的后缀为`.fbs`，后面都简称为fbs文件。

## 定义无依赖的fbs文件-greeter.fbs

``` fbs
namespace trpc.test.helloworld;

# 定义请求
table FbRequest {
  message:string;
}

# 定义回复
table FbReply {
  message:string;
}

# 定义 rpc 服务接口
rpc_service FbGreeter {
  SayHello(FbRequest):FbReply;
}
```

该fbs文件中用到的字段定义比较简单，属于比较常用的，若想了解更多关于flatbuffers的字段定义可去查阅[flatbuffers官网](https://google.github.io/flatbuffers/flatbuffers_guide_writing_schema.html)。

## 定义有依赖的fbs文件-forward.fbs

``` fbs
include "greeter.fbs";

namespace trpc.test.forward;

rpc_service FbForward {
  Route(trpc.test.helloworld.FbRequest):trpc.test.helloworld.FbReply;
}
```

依赖文件需要注意以下两点

- 被依赖的文件 **include "greeter.fbs"** (第1行) **必须放在`namespace trpc.test.forward` 的前面**，不然会编译不过
- 被依赖的文件和依赖文件必须处于同级目录下或者前者属于后者的子目录中，例如：

   ```txt
   ├── b
   │   └── greeter.fbs
   ├── helloworld.fbs
   └── forward.fbs
   ```

  由于`b`属于`forward.fbs`所在目录的子目录，所以`forward.fbs`可以依赖`b`中的`greeter.fbs`，反之则不行
  `forward.fbs`和`helloworld.fbs`属于同级目录，所以`forward.fbs`也可以依赖`helloworld.fbs`

# 利用fbs桩代码工具生成桩代码

为了更好的讲解，这里首先给出了讲解项目目录：

```txt
.
├── bazel_build.sh
├── clean.sh
├── cmake_build.sh
├── CMakeLists.txt
├── server
│   ├── BUILD
│   ├── commom.cmake
│   ├── demo_server.cc
│   ├── demo_server.h
│   ├── demo_service.cc
│   ├── demo_service.h
│   ├── forward.fbs
│   ├── greeter.fbs
│   └── trpc_cpp_fiber.yaml
└── WORKSPACE
```

tRPC-Cpp框架内置的fbs桩代码工具，支持`bazel`和`cmake`两种编译方式，下面以两种方式分别进行介绍

## bazel编译方式

`bazel`是框架推荐的编译方式，该方式需要配置`WORKSPACE`和`BUILD`文件，一个项目只需配置一个`WORKSPACE`文件，项目中的每个文件夹下都需要配置`BUILD`文件

### WORKSPACE编写如下

```txt
workspace(name = "fbs_demo")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

# bazel会自动拉取trpc代码进行编译
git_repository(
    name = "trpc_cpp",
    tag = "{trpc_ver}", # 指定版本, 一般推荐最新的 tag
    remote = "{trpc_cpp_addr}", # 指定 trpc—cpp 的git地址
    # for example
    # tag = "v1.0.0",
    # remote = "https://github.com/trpc-group/trpc-cpp.git",
)

load("@trpc_cpp//trpc:workspace.bzl", "trpc_workspace")

trpc_workspace()
```

`WORKSPACE`文件内容介绍如下：

- git_repository：指定`trpc-cpp`项目的地址和版本
- trpc_workspace：加载`trpc-cpp`所需要的依赖，该函数可以指定框架所依赖的第三方包的版本， 例如：`trpc_workspace(fbs_ver="23.5.26")`

### 无依赖greeter.fbs对应的BUILD文件

``` bazel
load("@trpc_cpp//trpc:flatbuffers.bzl", "trpc_fbs_library")
trpc_fbs_library(
    name = "greeter_fbs",
    srcs = ["greeter.fbs"],
    flatc_args = [ 
        "--gen-object-api",
        "--gen-compare",
        "--no-includes",
        "--gen-mutable",
        "--reflect-names",
        "--cpp-ptr-type flatbuffers::unique_ptr",
        "--trpc",
    ],
    deps = [],
)
```

在`BUILD`文件中，需要注意以下几行：

- line1： 通过 **load("@trpc_cpp//trpc:flatbuffers.bzl", "trpc_fbs_library")** 函数加载**trpc_fbs_library**函数，该函数是根据fbs文件生成桩代码的关键
- line7： 参数 **--no-includes** 表示不依赖其他的fbs文件，若有依赖则不能加该参数
- line8： 参数 **--trpc** 表示生成以`.trpc.fb.h/.trpc.fb.cc`为后缀的文件，若没有该参数则不会生成（前提是`fbs文件`中必须存在`rpc_service`字段）

编译生成文件：`greeter_generated.h`、`greeter.trpc.fb.cc`、`greeter.trpc.fb.h`，这个三个文件都在**bazel-bin/server**的目录中，它们的作用以及内容如下:

- **greeter_generated.h：** 属于和flatbuffers关联的代码，由桩代码工具转化fbs文件时必定会生成的文件，命名规则是在fbs文件名上加`_generated`后缀；类似于protoc生成的pb.h文件，里面主要包含字段和接口的实现，有兴趣可以自己生成文件并对照[flatbuffers guide](https://google.github.io/flatbuffers/flatbuffers_guide_use_cpp.html)进行理解
- **greeter.trpc.fb.h/greeter.trpc.fb.cc**: 这两个文件的生成的前提是在上面的`BUILD`文件中必须加上`--trpc`参数
- **greeter.trpc.fb.h** 源码

  ``` cpp
  // greeter.trpc.fb.h
  #pragma once
  
  #include "greeter_generated.h"
  
  // ... 省略部分代码
  
  namespace trpc {
  namespace test {
  namespace helloworld {
  
  // 服务基类
  class FbGreeter: public ::trpc::RpcServiceImpl {
   public:
    FbGreeter();
  
    // 服务方法，业务根据需要继承重写
    virtual ::trpc::Status SayHello(const ::trpc::ServerContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>* request, flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>* response);
  };
  
  class FbGreeterServiceProxy : public ::trpc::RpcServiceProxy {
   public:
    // 同步接口
    virtual ::trpc::Status SayHello(const ::trpc::ClientContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>& request, flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>* response);
  
    // 异步接口
    virtual ::trpc::Future<flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>> AsyncSayHello(const ::trpc::ClientContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>& request);
  };
  
  }  // namespace helloworld
  }  // namespace test
  }  // namespace trpc
  ```

  生成的服务桩代码，包含服务端`FbGreeter`和客户端`FbGreeterServiceProxy`，类方法都是虚函数，方便用户继承重写业务逻辑
- **greeter.trpc.fb.cc** 源码：

  ``` cpp
  
  #include "greeter.trpc.fb.h"
  
  // ... 省略部分代码
  
  namespace trpc {
  namespace test {
  namespace helloworld {
  
  static const char* FbGreeter_method_names[] = { 
    "/trpc.test.helloworld.FbGreeter/SayHello",
  };
  
  FbGreeter::FbGreeter() {
  // ... 省略部分代码
  }
  
  ::trpc::Status FbGreeter::SayHello(const ::trpc::ServerContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>* request, flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>* response) {
    (void) context;
    (void) request;
    (void) response;
    return ::trpc::Status(-1, "");
  }
  
  ::trpc::Status FbGreeterServiceProxy::SayHello(const ::trpc::ClientContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>& request, flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>* response) {
    context->SetFuncName(FbGreeter_method_names[0]);
    context->SetReqEncodeType(trpc::EncodeType::FLATBUFFER);
    return UnaryInvoke<flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>, flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>>(context, request, response);
  }
  
  ::trpc::Future<flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>> FbGreeterServiceProxy::AsyncSayHello(const ::trpc::ClientContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>& request) {
    context->SetFuncName(FbGreeter_method_names[0]);
    context->SetReqEncodeType(trpc::EncodeType::FLATBUFFER);
    return AsyncUnaryInvoke<flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>, flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>>(context, request);
  }
  
  }  // namespace trpc
  }  // namespace test
  }  // namespace helloworld
  ```

  服务端方法`FbGreeter::SayHello`默认空实现，用户必须继承重写，否则返回错误信息。
  客户端方法提供两种：`FbGreeterServiceProxy::SayHello`同步调用；`FbGreeterServiceProxy::AsyncSayHello`异步调用。对于客户端而言，可以直接使用，不必继承重写，其中`context->SetReqEncodeType(trpc::EncodeType::FLATBUFFER)`表示设置数据编码为`flatbuffer`类型

### 有依赖forward.fbs对应的BUILD文件

``` fbs
trpc_fbs_library(
    name = "forward_fbs",
    srcs = ["forward.fbs"],
    include_paths = ["test/route-coroutine/route"],
    includes = [
        "greeter.fbs",
    ],
    flatc_args = [
        "--gen-object-api",
        "--gen-compare",
        "--gen-mutable",
        "--reflect-names",
        "--cpp-ptr-type flatbuffers::unique_ptr",
        "--trpc",
    ],
    deps = [
        ":greeter_fbs",
    ],
    visibility = [
     "//visibility:public",
    ],
)
```

相比于无依赖的`greeter.fbs`文件，参数有以下不同：

- line3：参数 **include_paths** 表示依赖的文件所在的目录
- line4：参数 **includes** 表示具体依赖的文件名称
- 去除了 **--no-includes** 参数

对应编译生成的文件有：`forward_generated.h`、`forward.trpc.fb.cc`、`forward.trpc.fb.h`，具体源码类似上面的`greeter.fbs`生成的桩代码，这里就不贴出来了。

## cmake 编译方式

采用`cmake`编译时，可以直接引用框架内置的桩代码生成工具，`CMakeLists`编写如下：

```cmake
cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

#---------------------------------------------------------------------------------------
# Add trpc-cpp to project
#---------------------------------------------------------------------------------------
# You can import trpc-cpp as external libs in two ways.
# 1(Recomended). FetchContent_Declare and then FetchContent_MakeAvailable
# 2. download trpc-cpp and then use add_subdirectory to import
include(FetchContent)

FetchContent_Declare(
    trpc-cpp
    GIT_REPOSITORY    ${trpc_cpp_addr}, # 指定 trpc—cpp 的 git 地址
    GIT_TAG           ${trpc_ver}, # 指定版本,一般推荐最新的 tag
    SOURCE_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/cmake_third_party/trpc-cpp
    # GIT_REPOSITORY  https://github.com/trpc-group/trpc-cpp.git
    # GIT_TAG         v1.0.0
)
FetchContent_MakeAvailable(trpc-cpp)

set(TEST_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${TEST_ROOT})

# Set fbs tool
set(FBS_CPP_PLUGIN ${FBS_TO_CPP_PLUGIN})

#---------------------------------------------------------------------------------------
# Compile project
#---------------------------------------------------------------------------------------
project(fbs_demo)

# compile fbs file
set(SERVER_FBS_SRC ${TEST_ROOT}/server/greeter.fbs
                  ${TEST_ROOT}/server/forward.fbs)

TRPC_COMPILE_FBS(OUT_TRPC_FBS_SERVER_SRCS "${SERVER_FBS_SRC}" ${FBS_CPP_PLUGIN} ${TEST_ROOT})

# compile server
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/server/*.cc)
add_executable(demoserver ${SRC_FILES} ${OUT_TRPC_FBS_SERVER_SRCS})
# include current direcotry to include fbs stub code deps
target_include_directories(demoserver PUBLIC ${TEST_ROOT})
target_link_libraries(demoserver trpc)

```

生成的文件`greeter_generated.h`, `greeter.trpc.fb.cc`, `greeter.trpc.fb.h`可以在当前项目的`server`目录下找到，生成文件前文已经描述，这里不再赘述。

# 基于桩代码构建服务端代码

生成转代码后，可以通过以下流程，编写一个完整的服务端代码

## 编写服务端业务代码

以`greeter.fbs`生成代码为例来编写服务端业务代码。

``` cpp

#include "server/demo_service.h"

#include <memory>
#include <string>

#include "trpc/log/trpc_log.h"
#include "trpc/util/flatbuffers/trpc_fbs.h"

#include "server/greeter.trpc.fb.h"
#include "server/greeter_generated.h"

namespace examples::flatbuffers {

// 继承桩代码生成的服务对象
class DemoServiceImpl final : public ::trpc::test::helloworld::FbGreeter {
 public:
  ::trpc::Status SayHello(
      const ::trpc::ServerContextPtr& context,
      const ::flatbuffers::trpc::Message<::trpc::test::helloworld::FbRequest>* request_msg,
      ::flatbuffers::trpc::Message<::trpc::test::helloworld::FbReply>* response_msg) override;
};

::trpc::Status DemoServiceImpl::SayHello(
    const ::trpc::ServerContextPtr& context,
    const ::flatbuffers::trpc::Message<::trpc::test::helloworld::FbRequest>* request_msg,
    ::flatbuffers::trpc::Message<::trpc::test::helloworld::FbReply>* response_msg) {
  ::flatbuffers::trpc::MessageBuilder mb;

  // We call GetRoot to "parse" the message. Verification is already
  // performed by default. See the notes below for more details.
  const ::trpc::test::helloworld::FbRequest* request = request_msg->GetRoot();

  // Fields are retrieved as usual with FlatBuffers
  const std::string& name = request->message()->str();

  TRPC_FMT_INFO("client request msg:{}", name);

  // `flatbuffers::grpc::MessageBuilder` is a `FlatBufferBuilder` with a
  // special allocator for efficient tRPC buffer transfer, but otherwise
  // usage is the same as usual.
  auto msg_offset = mb.CreateString(name);
  auto hello_offset = ::trpc::test::helloworld::CreateFbReply(mb, msg_offset);
  mb.Finish(hello_offset);

  // The `ReleaseMessage<T>()` function detaches the message from the
  // builder, so we can transfer the resopnse to tRPC while simultaneously
  // detaching that memory buffer from the builer.
  *response_msg = mb.ReleaseMessage<::trpc::test::helloworld::FbReply>();
  TRPC_ASSERT(response_msg->Verify());

  // Return an OK status.
  return ::trpc::kSuccStatus;
}

}  // namespace examples::flatbuffers
```

从上述业务代码可以看出，flatbuffers生成的API接口用起来不如protobuf方便，所以需要注意以下几点：

- 必须包含**trpc/util/flatbuffers/trpc_fbs.h**头文件。
- 请求和回复对象类型必须是模板，例如：代码中的请求对象的类型是**flatbuffers::trpc::Message<::trpc::test::helloworld::FbRequest>**，它是`greeter.fbs`中**FbRequest**和**flatbuffers::trpc::Message**两种类型组合而成。
- 获取请求/回复的方式，以请求为例，**::trpc::test::helloworld::FbRequest属于私有继承**，用户不能直接实例化，获取请求的字段分为两步：
a.通过 **request_msg->GetRoot()** 获取到 **trpc::test::helloworld::FbRequest** 对象指针(line20)
b.通过 **trpc::test::helloworld::FbRequest** 对象指针获取字段值(line23)。
- 构造请求/回复对象，以回复为例，必须借助**flatbuffers::trpc::MessageBuilder**来构建请求或者回复数据，源码line30～line37是构建回复的整个过程(构建请求也是如此，只需将`CreateFbReply`改成`CreateFbRequest`即可), 比较复杂，具体原理可以参考[flatbuffers guide](https://google.github.io/flatbuffers/flatbuffers_guide_use_cpp.html)。
- 数据校验，**response_msg->Verify()**(line38)是用于检测构建的数据是否合法，会有一些性能上的开销，但是开销不大，具体业务是否需要该检测取决于具体的应用场景。

## 服务注册

``` cpp

#include "trpc/common/trpc_app.h"

namespace examples::flatbuffers {
class DemoServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override;
};

int DemoServer::Initialize() {
  const auto& config = ::trpc::TrpcConfig::GetInstance()->GetServerConfig();
  // Set the service name, which must be the same as the value of the `server:service:name` configuration item
  // in the framework configuration file, otherwise the framework cannot receive requests normally
  std::string service_name = fmt::format("{}.{}.{}.{}", "trpc", config.app, config.server, "FbGreeter");

  TRPC_FMT_INFO("service name:{}", service_name);

  RegisterService(service_name, std::make_shared<DemoServiceImpl>());

  return 0;
}
}

int main(int argc, char** argv) {
  examples::flatbuffers::DemoServer demo_server;
  demo_server.Main(argc, argv);
  demo_server.Wait();

  return 0;
}

```

`RegisterService`函数为注册函数，使用该函数可以注册服务到框架中

## 配置文件

``` yaml
  service: #业务服务提供的service，可以有多个
    - name: trpc.test.helloworld.FbGreeter #service的路由名称
      network: tcp #网络监听类型  tcp udp
      protocol: trpc #应用层协议 trpc http
```

  由于生成的桩代码已经可以识别服务数据类型是`trpc::EncodeType::FLATBUFFER`，所以无需在配置中设置

# FAQ

## Q1：编译flatbuffers出现flatbuffers::trpc::Message未找到？

检测在对应的BUILD文件中是否包含 **"@trpc_cpp//trpc/util/flatbuffers:trpc_fbs"** 依赖。

## Q2：构建请求/回复过程中出现core了，怎么解决？

flatbuffers通过**flatbuffers::trpc::MessageBuilder**构建数据过程中默认大小是1024字节，如果用户构建的数据过大，需要在定义**flatbuffers::trpc::MessageBuilder**对象的时候指定大小，例如：**flatbuffers::trpc::MessageBuilder mb(new_size)**,这个`new_size`就是指定能构建数据的最大字节。

## Q3：按照例子定义了fbs文件，也指定了参数 `--trpc`，但是没有生成`trpc.fb.h/cc`文件？

检查一下定义的fbs文件中是否存在`rpc_service`字段

## Q4：采用bazel编译，出现`unable to load include file xxx`错误？

不同fbs文件之间的依赖，采用bazel编译，对依赖文件有一定的要求，参考：`2.2 定义有依赖的fbs文件`
