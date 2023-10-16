[English](../en/proto_management.md)

# 前言

tRPC-Cpp 框架使用 `protobuf` 作为缺省的接口定义语言（IDL）, 服务之间调用时, proto 接口文件作为服务的对外用户界面, 应该有比较统一的管理方式。

常规的做法: 每个服务的 proto 文件自己管理, 服务需要互通时, 将 proto 文件 copy 一份, 然后通过 protoc 工具临时生成桩代码来使用, 这样的做法导致每个服务的 proto 文件出现多个副本, 不同 proto 文件的副本因为更新不及时, 导致上下游协议不一致，从而出现互通问题.

因此, tRPC-Cpp 框架这里推荐的 proto 文件依赖管理方式是: 对于服务的提供方, 把需要对外的 proto 文件放到统一的远程仓库管理(比如: 使用 github 的仓库或者内部私有的仓库); 对于服务的调用方, 只需要基于 bazel 构建工具自动拉取依赖的 proto 文件并生成桩代码即可.

tRPC-Cpp借用bazel的机制，实现了 [trpc_proto_library](./bazel_or_cmake.md#trpc_proto_library的使用) 来管理带复杂依赖的 proto 文件，您可以先阅读该文档，以初步认识它。

# 基于bazel 自动拉取远程 proto 文件的使用方式

步骤1: 拉取依赖服务的 proto 文件

比如，有个您已经在项目组里创建一个了github仓库 `your_group/proto` 里，定义如下bazel规则。

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

在本项目 `local_project` 里的 WORKSPACE 文件中, 添加拉取 proto 文件统一管理的远程仓库 `your_group/proto` 的目标规则, 如下:

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

步骤2: 将 proto 目标加入到使用者的目标依赖中

添加如下bazel规则，以让 `local_project` 里实现调用逻辑的代码能引入调用 HelloWorld 服务的桩代码：

```python
# project: local_project
# file: service/BUILD
cc_library(
    name = "local_service",
    hdrs = ["local_service.h"],
    srcs = ["local_service.cc"],
    deps = [
      "@remote_proto_repo//test:helloworld_proto", # add here
    ]
)
```

步骤3：代码中调用远程接口

此时，即可在本项目里，创建对应类型对service的Proxy后，可直接调用服务的接口，参考如下示例代码：

```c++
// project: local_project
// file: service/local_service.cc

#include "test/helloworld.trpc.pb.h"

// HelloWorldServiceProxy type got from file "test/helloworld.trpc.pb.h"
auto proxy = ::trpc::GetTrpcClient()->GetProxy<HelloWorldServiceProxy>("client->service->name defined in yaml file");
proxy->SayHello(req, rsp);
```
