[中文版](/docs/zh/flatbuffers_protocol_service.md)

[TOC]

# Overview

[flatbuffers](https://google.github.io/flatbuffers/usergroup0.html) is a protocol similar to protobuf, which has efficient memory access characteristics. For specific performance requirements, such as demanding scenarios like gaming, it is recommended to refer to the [flatbuffers_benchmarks](https://google.github.io/flatbuffers/flatbuffers_benchmarks.html) for detailed performance parameters.
The tRPC-Cpp framework has a built-in stub code generation tool that can generate corresponding stub code based on flatbuffers IDL files (with the `.fbs` extension). The underlying principle is similar to how the tRPC-Cpp framework generates stub code for protobuf types through plugins.

# Writing an IDL file of flatbuffers

Below are two common IDL files corresponding to flatbuffers. Currently, the convention is to use the `.fbs` extension for IDL files, and they will be referred to as fbs files.

## Defining a dependency-free fbs file

``` fbs
namespace trpc.test.helloworld;

# Defining request struct.
table FbRequest {
  message:string;
}

# Defining response struct.
table FbReply {
  message:string;
}

# Defining rpc service
rpc_service FbGreeter {
  SayHello(FbRequest):FbReply;
}
```

The field definitions used in this fbs file are relatively simple and commonly used. If you want to learn more about field definitions in flatbuffers, you can refer to the documentation [flatbuffers官网](https://google.github.io/flatbuffers/flatbuffers_guide_writing_schema.html)。

## Defining a dependent fbs file - forward.fbs

``` fbs
include "greeter.fbs";

namespace trpc.test.forward;

rpc_service FbForward {
  Route(trpc.test.helloworld.FbRequest):trpc.test.helloworld.FbReply;
}
```

There are two points to note when dealing with dependent files:

- The file being depended on **include "greeter.fbs"** (line 1), must be placed before the **namespace trpc.test.forward**, otherwise it will fail to compile.
- The file being depended on and the dependent file must be in the same directory or the former should be in a subdirectory of the latter. For example:

   ```txt
   ├── b
   │   └── greeter.fbs
   ├── helloworld.fbs
   └── forward.fbs
   ```

  Since `b` is a subdirectory of the directory where `forward.fbs` is located, `forward.fbs` can depend on `greeter.fbs` in `b`. However, the reverse is not possible.`forward.fbs` and `helloworld.fbs` are in the same directory, so `forward.fbs` can also depend on `helloworld.fbs`.

# To generate stub code using the fbs code generation tool

To provide a better explanation, here is the directory structure of the project being discussed:

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

The tRPC-Cpp framework comes with a built-in fbs stub code generation tool that supports two compilation methods: `bazel` and `cmake`. Below, we will provide an introduction to each method separately.

## Bazel Compilation Method

`Bazel` is the recommended compilation method for the framework. This method requires configuring the `WORKSPACE` and `BUILD` files. Only one `WORKSPACE` file needs to be configured for the entire project, and a `BUILD` file needs to be configured in each folder of the project.

### The contents of the WORKSPACE file are as follows

```txt
workspace(name = "fbs_demo")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

# Bazel will automatically fetch the tRPC code and compile it.
git_repository(
    name = "trpc_cpp",
    tag = "{trpc_ver}", # Specify the version, usually the latest tag is recommended.
    remote = "{trpc_cpp_addr}", # Specify the git address for tRPC-Cpp.
    # for example
    # tag = "v1.0.0",
    # remote = "https://github.com/trpc-group/trpc-cpp.git",
)

load("@trpc_cpp//trpc:workspace.bzl", "trpc_workspace")

trpc_workspace()
```

The `WORKSPACE` file contains the following information:

- git_repository：Specify the address and version of the `trpc-cpp` project.
- trpc_workspace：Load the dependencies required for `trpc-cpp`. This function can specify the versions of third-party packages that the framework depends on. For example: `trpc_workspace(fbs_ver="23.5.26")`.

### The BUILD file for greeter.fbs with no dependencies is as follows

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

In the `BUILD` file, please pay attention to the following lines:

- line1: Load the **trpc_fbs_library** function using the **load("@trpc_cpp//trpc:flatbuffers.bzl", "trpc_fbs_library")** function. This function is crucial for generating stub code based on the `.fbs` files.
- line7: The parameter **--no-includes** indicates that there are no dependencies on other `.fbs` files. If there are dependencies, this parameter should not be added.
- line8: The parameter **--trpc** indicates the generation of files with the `.trpc.fb.h/.trpc.fb.cc` suffix. If this parameter is not present, the files will not be generated (provided that the `.fbs` file contains the rpc_service field).

The three files, which are `greeter_generated.h`, `greeter.trpc.fb.cc`, and `greeter.trpc.fb.h`, are compiled and generated in the **bazel-bin/server directory**. Their purposes and contents are as follows:

- **greeter_generated.h:** This file is generated by the FlatBuffers compiler and contains the generated code for the `greeter.fbs` schema. It includes the necessary definitions and functions for working with the serialized data. If you are interested, you can generate the files yourself and refer to the [flatbuffers guide](https://google.github.io/flatbuffers/flatbuffers_guide_use_cpp.html) for better understanding.
- **greeter.trpc.fb.h/greeter.trpc.fb.cc**: The prerequisite for generating these two files is to include the **--trpc** parameter in the `BUILD` file mentioned above.
- **greeter.trpc.fb.h** source code

  ``` cpp
  // greeter.trpc.fb.h
  #pragma once
  
  #include "greeter_generated.h"
  
  // ... Omitted code.
  
  namespace trpc {
  namespace test {
  namespace helloworld {
  
  // Base class for rpc service
  class FbGreeter: public ::trpc::RpcServiceImpl {
   public:
    FbGreeter();
  
    // Service methods. Business logic can be inherited and overridden as needed.
    virtual ::trpc::Status SayHello(const ::trpc::ServerContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>* request, flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>* response);
  };
  
  class FbGreeterServiceProxy : public ::trpc::RpcServiceProxy {
   public:
    // Synchronous interface.
    virtual ::trpc::Status SayHello(const ::trpc::ClientContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>& request, flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>* response);
  
    // Asynchronous interface.
    virtual ::trpc::Future<flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>> AsyncSayHello(const ::trpc::ClientContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>& request);
  };
  
  }  // namespace helloworld
  }  // namespace test
  }  // namespace trpc
  ```

  The generated service stub code includes the server-side `FbGreeter` and the client-side `FbGreeterServiceProxy`. All class methods are virtual functions, making it convenient for users to inherit and override the business logic.

- **greeter.trpc.fb.cc** source code:

  ``` cpp
  
  #include "greeter.trpc.fb.h"
  
  // ... Omitted code.
  
  namespace trpc {
  namespace test {
  namespace helloworld {
  
  static const char* FbGreeter_method_names[] = { 
    "/trpc.test.helloworld.FbGreeter/SayHello",
  };
  
  FbGreeter::FbGreeter() {
  // ... Omitted code.
  }
  
  ::trpc::Status FbGreeter::SayHello(const ::trpc::ServerContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>* request, flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>* response) {
    (void) context;
    (void) request;
    (void) response;
    return ::trpc::Status(-1, "");
  }
  
  ::trpc::Status FbGreeterServiceProxy::SayHello(const ::trpc::ClientContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>& request, flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>* response) {
    context->SetFuncName(FbGreeter_method_names[0]);
    context->SetEncodeType(trpc::EncodeType::FLATBUFFER);
    return UnaryInvoke<flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>, flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>>(context, request, response);
  }
  
  ::trpc::Future<flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>> FbGreeterServiceProxy::AsyncSayHello(const ::trpc::ClientContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>& request) {
    context->SetFuncName(FbGreeter_method_names[0]);
    context->SetEncodeType(trpc::EncodeType::FLATBUFFER);
    return AsyncUnaryInvoke<flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>, flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>>(context, request);
  }
  
  }  // namespace trpc
  }  // namespace test
  }  // namespace helloworld
  ```

  The server-side method `FbGreeter::SayHello` has a default empty implementation. Users must inherit and override this method; otherwise, an error message will be returned.
  The client-side methods provide two options: `FbGreeterServiceProxy::SayHello` for synchronous calls and `FbGreeterServiceProxy::AsyncSayHello` for asynchronous calls. For the client, these methods can be used directly without the need for inheritance and overriding. The line `context->SetEncodeType(trpc::EncodeType::FLATBUFFER)` indicates setting the data encoding type to `flatbuffer`.

### The BUILD file for forward.fbs with dependencies is as follows

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

Compared to the dependency-free `greeter.fbs` file, the parameters have the following differences:

- line3: The parameter **include_paths** represents the directory where the dependent files are located.
- line4：The parameter **includes** represents the specific names of the dependent files.
- The parameter **--no-includes** was removed.

The corresponding compiled generated files are: `forward_generated.h`, `forward.trpc.fb.cc` and `forward.trpc.fb.h`, The specific source code is similar to the stub code generated for `greeter.fbs` mentioned abovend and will not be repeated here.

## CMake build method

When using `CMake` for compilation, you can directly reference the built-in stub code generation tool provided by the framework. The `CMakeLists.txt` file can be written as follows:

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
    GIT_REPOSITORY    ${trpc_cpp_addr}, # Specify the version, usually the latest tag is recommended.
    GIT_TAG           ${trpc_ver}, # # Specify the git address for tRPC-Cpp.
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

生成的`greeter_generated.h`, `greeter.trpc.fb.cc` and `greeter.trpc.fb.h` can be found in the `server` directory of the current project. The generation of these files has been described earlier and will not be repeated here.

# Building server-side code based on stub code

After generating the stub code, you can follow the steps below to write a complete server-side code

## Write server-side business logic code

Let's take the example of generating code from `greeter.fbs` to write server-side business logic code.

``` cpp

#include "server/demo_service.h"

#include <memory>
#include <string>

#include "trpc/log/trpc_log.h"
#include "trpc/util/flatbuffers/trpc_fbs.h"

#include "server/greeter.trpc.fb.h"
#include "server/greeter_generated.h"

namespace examples::flatbuffers {

// Inherit the service object generated from the stub code.
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

From the above business code, it can be seen that the API interface generated by FlatBuffers is not as convenient as Protocol Buffers (protobuf). Therefore, it is important to note the following points:

- Must include the **trpc/util/flatbuffers/trpc_fbs.h** header file.
- The request and response object types must be templates. For example, in the code, the type of the request object is **flatbuffers::trpc::Message<::trpc::test::helloworld::FbRequest>**, which is a combination of the **FbRequest** type from `greeter.fbs` and the **flatbuffers::trpc::Message** type.
- To obtain the request/reply, taking the request as an example, **::trpc::test::helloworld::FbRequest is privately inherited**, and users cannot directly instantiate it. Getting the fields of the request involves two steps:
a.Use **request_msg->GetRoot()** to obtain a pointer to the **trpc::test::helloworld::FbRequest** object (line 20).
b.Use the pointer to the **trpc::test::helloworld::FbRequest** object to retrieve the field value (line 23).
- To construct the request/reply object, taking the reply as an example, you need to use **flatbuffers::trpc::MessageBuilder** to build the request or reply data. The code from line 30 to line 37 is the entire process of constructing the reply (constructing the request is similar, just replace **CreateFbReply** with **CreateFbRequest**). It is a complex process, and you can refer to the [FlatBuffers guide](https://google.github.io/flatbuffers/flatbuffers_guide_use_cpp.html) for more details on the underlying principles.
- Data validation is performed using **response_msg->Verify()** (line 38) to check if the constructed data is valid. This verification process incurs some performance overhead, but it is generally minimal. Whether or not this validation is necessary depends on the specific application scenario and business requirements.

## Service registration

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

The `RegisterService` function is a registration function that can be used to register a service with the framework.

## 配置文件

``` yaml
  service: #A business service provider can offer multiple services.
    - name: trpc.test.helloworld.FbGreeter #service name
      network: tcp #Network listener types  tcp udp
      protocol: trpc #Application layer protocolstrpc http
```

Since the generated stub code is already capable of recognizing the service data type as **FlatBuffer**, there is no need to configure it in the settings.

# FAQ

## Q1: "flatbuffers::trpc::Message not found"" error occurred during the compilation of FlatBuffers?

Check if the corresponding BUILD file includes the dependency **"@trpc_cpp//trpc/util/flatbuffers:trpc_fbs"**.

## Q2: If a core dump occurs during the request/reply process, how can it be resolved?

flatbuffers通过**flatbuffers::trpc::MessageBuilder**构建数据过程中默认大小是1024字节，如果用户构建的数据过大，需要在定义flatbuffers::trpc::MessageBuilder对象的时候指定大小，例如：**flatbuffers::trpc::MessageBuilder mb(new_size)**,这个`new_size`就是指定能构建数据的最大字节。

During the process of building data using **flatbuffers::trpc::MessageBuilder** in FlatBuffers, the default size is 1024 bytes. If the data being built by the user exceeds this default size, it is necessary to specify the size when defining the **flatbuffers::trpc::MessageBuilder object**. For example, **flatbuffers::trpc::MessageBuilder mb(new_size)**, where `new_size` specifies the maximum number of bytes that can be used to build the data.

## Q3: Even though the example fbs file was defined and the `--trpc` parameter was specified, the `trpc.fb.h/cc files` were not generated?

Please check if the defined fbs file contains the `rpc_service` field.

## Q4: When using Bazel to compile, if you encounter the error "unable to load include file xxx", it means that Bazel is unable to find the specified include file?

When using Bazel to compile, there are certain requirements for the dependencies between different fbs files. Please refer to section `Defining a dependent fbs file - forward.fbs` for more information.
