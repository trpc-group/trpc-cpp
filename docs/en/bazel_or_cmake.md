[中文版](../zh/bazel_or_cmake.md)

# Use Bazel

[bazel](https://bazel.build/) is one of popular morden build tools for C++ project. tRPC-Cpp use bazel to manage the building process from start. And we strongly recommend you use it.

## Bazel install and tips

- See [setup_env](setup_env.md) for installation details.
- Commonly used command:
  - Use `bazel build ...` to build all the libraries and binaries.
  - Use `bazel build //relative-path-from-root:bazel_target` to build a specified library or binary.
  - Add `build --copt=-g --strip=never` to .bazelrc or as command argument to enable debug.
- Useful learning resources:
  - [Bazel concepts](https://bazel.build/concepts/build-ref).
  - [Bazel cpp use caces](https://bazel.build/tutorials/cpp-use-cases).

## trpc_proto_library

### Overview

You can define a service in proto file using protobuf IDL code. tRPC-Cpp facilitates bazel system to provide a bazel rule `trpc_proto_library`. It can generate tRPC RPC stub code and protobuf stub code.

In your project, You can import any proto file and manage their dependency as below:

```python
# use trpc_proto_library to import proto
trpc_proto_library(
    name = "hello_proto",
    srcs = ["hello.proto"],
    use_trpc_plugin = True,
    rootpath = "@trpc_cpp",
    deps = [
      # other bazel target defined using trpc_proto_library
    ],
)

# Use cc_library, cc_binary, other cpp rules to import trpc_proto_library rule
cc_library(
    name = "user_lib",
    srcs = ["xxx.cc"],
    hdrs = ["xxx.h"],
    deps = [":hello_proto"]
)
```

PS: *You can find stub code at bazel-bin directory after building.*

### Full options

You can check full options of trpc_proto_library as below:

```python
trpc_proto_library(
    name = "xxx_proto",
    srcs = ["xxx.proto"],
    use_trpc_plugin = True,
    rootpath = "@trpc_cpp",
    generate_new_mock_code = False,
    enable_explicit_link_proto = False,
    deps = [
      # other target defined using trpc_proto_library
    ],
    native_proto_deps = [
      # target defined using proto_library
    ],
    native_cc_proto_deps = [
      # target defined using cc_proto_library
    ],
)
```

- name: Bazel target name. We suggest you define it with `_proto` suffix to indicate by imporing this target you can import proto file.
- srcs: source file list. You can put a bunch of proto files here although they are irrelevant. But we strongly suggest you define target with file one by one so that the dependency will be clear.
- use_trpc_plugin: True will generate tRPC-Cpp RPC stub code and protobuf stub code(\*.trpc.pb.h, \*.trpc.pb.cc, \*.pb.h, \*.pb.cc). False will generate only protobuf stub code(\*.pb.h, \*.pb.cc). Default to be False.
- rootpath: Required. Keep it as `@trpc_cpp` the same name as you imported in WORKSPACE file.
- generate_new_mock_code: Optional(Default False). Set True to get user metadata defined using protobuf option.
- deps: This target will depend on others defined using trpc_proto_library.
- native_proto_deps: This target will depend on others defined by [proto_library](https://bazel.build/reference/be/protocol-buffer#proto_library).
- native_cc_proto_deps: This target will depend on others defined by [cc_proto_library](https://bazel.build/reference/be/c-cpp#cc_proto_library).

### Bael compile options

You can add `--define xxx=true` to enable tRPC-Cpp compile options. You can check [support options in tRPC-Cpp](https://github.com/trpc-group/trpc-cpp/blob/main/trpc/BUILD). Please switch to specifed tag you use when you check.

### Protobuf option support

We use [protobuf options](https://protobuf.dev/programming-guides/proto3/#options) to support extension in tRPC. You should import the conresponding proto file to use it.

tRPC-Cpp currently supports below features:

- alias

#### Extension alias

Scenario: When a service defined in rpc, and you want to access it using http. Need to customize the url.

First, import releative proto file defined by tRPC:

```proto
// use `alias` need import trpc_options.proto file
import "trpc/proto/trpc_options.proto";

service Greeter {
  rpc SayHello (HelloRequest) returns (HelloReply) {
    option (trpc.alias) = "/cgi-bin/hello"; // You can access this interface via: http://domain/cgi-bin/hello
  }
}
```

Then, add dependency at `trpc_options.proto`:

```python
trpc_proto_library(
    name = "helloworld_proto",
    srcs = ["helloworld.proto"],
    native_proto_deps = [
        # depend on trpc_options.proto
        "@com_github_trpc_group_trpc//trpc/proto:trpc_options_proto",
    ],
    use_trpc_plugin = True,
)
```

At last, in the file WORKSPACE located at project root direcotry, import remote repo com_github_trpc_group_trpc:

```python
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
git_repository(
    name = "com_github_trpc_group_trpc",
    remote = "https://github.com/trpc-group/trpc.git",
    branch = "main",
)
```

### Other advanced features

#### Directly generated stub code via proto_library or cc_proto_library

Scenario: When a remote repo defined only using proto_library or cc_proto_library and don't want to include tRPC-Cpp. It needs to Directly generated stub code via proto_library or cc_proto_library.

You can do this by referencing below code:

```python
# BUILD file path: test/server/BUILD
trpc_proto_library(
    name = "rpc_service_proto", # filename of the gerneated stub code
    srcs = [], # keep it empty
    deps = [], # keep it empty
    use_trpc_plugin = True,
    rootpath = "@trpc_cpp",
    native_proto_deps = [
      # Only one target exist here or native_cc_proto_deps
      "@remote_pb_lib//proto/common:basic_proto",
    ],
    native_cc_proto_deps = [],
)
# Gerneated trpc stub code path: bazel-bin/test/server/rpc_service_proto.trpc.pb.h
# You can use it in your code: #include "test/server/rpc_service_proto.trpc.pb.h"
```

#### Import proto file from relative path

Scenario: Not recommend. This is a scenarion that proto file import others from current directory. You should consider using relative path from project root direcotry before this way.

For example, there exists below proto files:

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

You should fill `strip_import_prefix` option of `proto_library` to replace import prefix into empty string("") in proto file.

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
    strip_import_prefix = "",  # Keep empty string to replace "proto/svc/basic.proto" into "basic.proto".
)
```

### Get metadata from proto file

Scenario: Though [protobuf options](https://protobuf.dev/programming-guides/proto3/#options), one can get custom metadata their defined in proto file. And they may want to get this option using source code.

Enable `enable_explicit_link_proto` of `trpc_proto_library` in bazel BUILD file as below:

```python
trpc_proto_library(
    name = "xxx_proto",
    srcs = ["xxx.proto"],
    use_trpc_plugin = True,
    enable_explicit_link_proto = True, # Enable this option
)
```

There exists two ways to get meta ata:

- Get service metadata: You can get [ServiceOptions](https://protobuf.dev/reference/java/api-docs/com/google/protobuf/DescriptorProtos.ServiceOptions) from [ServiceDescriptor](https://protobuf.dev/reference/java/api-docs/com/google/protobuf/Descriptors.ServiceDescriptor) by invoking static method `XXXImplService::GetServiceDescriptor()` defined in `*.trpc.pb.h`.
- Get RPC method metadata after RPC invoke at client: You can get [MethodOptions](https://protobuf.dev/reference/java/api-docs/com/google/protobuf/DescriptorProtos.MethodOptions) from [MethodDescriptor](https://protobuf.dev/reference/java/api-docs/com/google/protobuf/Descriptors.MethodDescriptor) by invoking `client_ctx->GetProtobufMethodDescriptor()` defined in `ClientContext`. You can check below code for details.

  ```c++
  / 发起下游调用前，获取不到
  TRPC_ASSERT(client_ctx->GetProtobufMethodDescriptor() == nullptr);
  // 发起下游调用，在桩代码里赋值MethodOptions
  ::trpc::Status status = proxy->YourInvokeMethod(client_ctx, request, &reply);
  // 业务侧，在发起下游调用后，才可获取到
  context->GetProtobufMethodDescriptor() != nullptr;
  ```

# Use CMake

[CMake](https://cmake.org/) is a common used build tool in C++ project. tRPC-Cpp support this way to help you intergate into your CMake project.

## CMake install and tips

- Check [setup_env](setup_env.md) for installation details.
- Commonly used command:
  - You can start building project by `mkdir build && cd build && cmake .. && make -j8`. -j8 represents using 8 cpu cores to compile.
  - Build Debug version: `cmake -DCMAKE_BUILD_TYPE=Debug ..`.
  - Build Release version: `cmake -DCMAKE_BUILD_TYPE=Release ..`.
- See more usage at [CMake tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html).

## How to use tRPC-Cpp

### (Recommend) Use as external source code

We recommend this as it can easily switch tRPC-Cpp version to the lastest and the libs framework depends on can also be imported via a simple SDK target named `trpc`.

For example, you can import in CMakeLists.txt in this way:

```shell
# Fetch tRPC-Cpp and add as library
include(FetchContent)
FetchContent_Declare(
    trpc-cpp
    GIT_REPOSITORY    https://git.woa.com/trpc-cpp/open-source/trpc-cpp.git
    GIT_TAG           recommanded_always_use_latest_tag
    SOURCE_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/cmake_third_party/trpc-cpp
)
FetchContent_MakeAvailable(trpc-cpp)

# Set path of stub code genretated tool(PROTOBUF_PROTOC_EXECUTABLE/TRPC_TO_CPP_PLUGIN will be filled after you import tRPC-Cpp)
set(PB_PROTOC ${PROTOBUF_PROTOC_EXECUTABLE})
set(TRPC_CPP_PLUGIN ${TRPC_TO_CPP_PLUGIN})

# link lib trpc to your target
target_link_libraries(your_cmake_target trpc)
```

### Use via make install

Execute below commands, install tRPC-Cpp to your machine first:

```shell
# You can install the lastest verion by: git checkout tags/vx.x.x
git clone https://github.com/trpc-group/trpc-cpp.git
cd trpc-cpp
mkdir build && cd build

# By default, tRPC-Cpp will build as static lib. If you need dynamic lib, add cmake option: -DTRPC_BUILD_SHARED=ON
cmake ..
make -j8
make install # install at /usr/local/trpc-cpp/trpc
```

Then, import trpc lib in your CMakeLists.txt as below:

```shell
# set install path of tRPC-Cpp
set(TRPC_INSTALL_PATH /usr/local/trpc-cpp/trpc)

# Load hearders and libs
include(${TRPC_INSTALL_PATH}/cmake/config/trpc_config.cmake)
include_directories(${INCLUDE_INSTALL_PATHS})
link_directories(${LIBRARY_INSTALL_PATHS})

# Set path of stub code genretated tool
include(${TRPC_INSTALL_PATH}/cmake/tools/trpc_utils.cmake)
set(PB_PROTOC ${TRPC_INSTALL_PATH}/bin/protoc)
set(TRPC_CPP_PLUGIN ${TRPC_INSTALL_PATH}/bin/trpc_cpp_plugin)

# add trpc and it's dependent libs
set(LIBRARY trpc ${LIBS_BASIC})

# link lib trpc to your target
target_link_libraries(your_cmake_target trpc)
```

### Gerneate stub code for proto file

You can use below CMake function we defined to generate stub code after you importing tRPC-Cpp.

- `COMPILE_PROTO`: Will generate files \*.pb.h and \*.pb.cc which contain protobuf message definition in c++.
- `TRPC_COMPILE_PROTO`: Will generate files \*trpc.pb.h and \*.trpc.pb.cc which contain RPC stub code.

You can use them as belows:

```shell
# You should set tools path as before: PB_PROTOC and TRPC_CPP_PLUGIN

# set proto file lists, you can set more at once.
set(PB_SRCS ${CMAKE_SOURCE_DIR}/xxx/aaa.proto
           ${CMAKE_SOURCE_DIR}/yyy/bbb.proto)

# @brief Generate corresponding .pb.cc and .pb.h stub code based on proto files
# @param 1: [Output]The generated .pb.cc files from proto files
# @param 2: [Input]The collection of proto files
# @param 3: [Input]Protoc executable program generated during the compilation process in the build/bin directory
# @param 4: [Input]The directory where the CMakeLists.txt of the project is located
COMPILE_PROTO(OUT_PB_SRCS "${PB_SRCS}" ${PB_PROTOC} ${CMAKE_SOURCE_DIR})

# @brief  Generate corresponding .trpc.pb.cc and .trpc.pb.h stub code based on proto files
# @param 1: [Output]The generated .trpc.pb.cc files from proto files
# @param 2: [Input]The collection of proto files
# @param 3: [Input]Protoc executable program generated during the compilation process in the build/bin directory
# @param 4: [Input]trpc_cpp_plugin executable program generated during the compilation process in the build/bin directory
# @param 5: [Input]The directory where the CMakeLists.txt of the project is located
TRPC_COMPILE_PROTO(OUT_TRPC_PB_SRCS "${PB_SRCS}" ${PB_PROTOC} ${TRPC_CPP_PLUGIN} ${CMAKE_SOURCE_DIR})

# Add the generated stub code to your build target which depend on them
add_executable(${TARGET} xxx.cc
                        ${OUT_PB_SRCS}
                        ${OUT_TRPC_PB_SRCS})
```

### CMake compile option

Add `-DXXX=ON/OFF` to enable or disable [CMake compile option that tRPC-Cpp supports](https://github.com/trpc-group/trpc-cpp/blob/main/CMakeLists.txt). Please refer to sepcified framework version your use when you check.

If you import via make install, you should add relative library we defined in [trpc_config.cmake](https://github.com/trpc-group/trpc-cpp/blob/main/cmake/config/trpc_config.cmake).

For example, to enable SSL, you should execute `cmake .. -DTRPC_BUILD_WITH_SSL=ON`. And add `${LIB_SSL}` defined in trpc_config.cmake to `target_link_libraries`.

You don't need to do this if you import via external source code. That's one of reason why we recommond to import using this way.
