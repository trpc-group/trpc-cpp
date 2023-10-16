[中文](../zh/proto_management.md)

# Overview

tRPC uses `Protobuf` as the default interface definition language (IDL). When calling between services, the proto interface file is used as the external user interface of the service, and there should be a relatively unified management method.

Conventional practice: the proto file of each service is managed by itself. When the services need to communicate, copy a copy of the proto file, and then use the protoc tool to generate stub code for use. This approach results in multiple copies of the proto file for each service , the copies of different proto files are not updated in time, resulting in inconsistent upstream and downstream protocols, resulting in intercommunication problems.

Therefore, the proto file dependency management method recommended by tRPC-Cpp here is: for the service provider, put the proto files that need to be externally placed in a unified remote repositories (for example: use github repositories or internal private repositories); for service consumer, The consumer only needs to automatically pull the dependent proto files based on the bazel build tool and generate stub code.

For convenience, tRPC-Cpp defines a bazel rule named [trpc_proto_library](./bazel_or_cmake.md#trpc_proto_library) to manage proto files with complex dependency. We suggest you read it before continue.

# How to automatically pull remote proto files based on bazel

Step 1: Pull the proto file of the dependent service

For example, you have cread a github repo `your_group/proto`. A bazel rule inner may be like:

```python
# project: remote_proto
# file: test/BUILD
load("@trpc_cpp//trpc:trpc.bzl', 'trpc_proto_library")
trpc_proto_library(
    name = "helloworld_proto",
    srcs = ["helloworld.proto"], # test/helloworld.proto
    use_trpc_plugin = True,
    rootpath = "@trpc_cpp",
)
```

At WORKSPACE file, you can add below rules to import remote repo `your_group/proto` into your `local_project`:

```python
# project: local_project
# file: WORKSPACE
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
git_repository(
    name = "remote_proto_repo",
    branch = "main",
    remote = "https://github.com/your_group/proto.git",
)
```

Step 2: Add the proto target to the user's target dependencies

First add below bazel rule in `local_project` to import trpc stub code so that you can invoke HelloWorld service.

```python
# project: local_project
# file: service/BUILD
cc_library(
    name = "local_service",
    hdrs = ["local_service.h"],
    srcs = ["local_service.cc"],
    deps = [
      "@remote_proto_repo//test:helloworld_proto", # 添加此依赖
    ]
)
```

Step 3: Invoke RPC remote interface

Now, you can do RPC calls to remote service via using ServiceProxy as below:

```c++
// project: local_project
// file: service/local_service.cc

#include "test/helloworld.trpc.pb.h"

// HelloWorldServiceProxy type got from file "test/helloworld.trpc.pb.h"
auto proxy = ::trpc::GetTrpcClient()->GetProxy<HelloWorldServiceProxy>("client->service->name defined in yaml file");
proxy->SayHello(req, rsp);
```
