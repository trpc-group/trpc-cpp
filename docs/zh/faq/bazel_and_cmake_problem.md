### Bazel常见问题

#### 三方代码库拉取失败

- 场景：您开发了一些私有的三方库，希望使用它们，但发现拉取失败。
- 原因：git凭证不对。
- 解决办法：请检查私有三方仓库是否能通过 git 拉取下来，拉不下来的话，请检查用户家目录下 `~/.gitconfig` 以及 `~/.git-credentials` git配置和凭证是否有问题。

#### 之前编译成功，没做什么太大变更但现在编译失败

- 原因：极大可能是bazel缓存导致的问题。
- 解决办法：执行命令 `bazel clean --expunge` 清空bazel缓存，再重新编译，重新拉取三方依赖。

#### std标准库找不到的问题

- 原因：没有使用 C++17 标准
- 解决办法：在您项目根目录下的 `.bazelrc` 文件里，添加 `build --cxxopt="--std=c++17"`。

#### 引入框架头文件但却找不到

- 原因：bazel相关target没有引入对应框架的依赖项。
- 解决办法：比如，出现 `#include "trpc/common/trpc_plugin.h"` 头文件找不到的错误，需要在用户项目出现此错误的 `cc_library` 规则的 `deps` 字段里，加上 `@trpc_cpp://trpc/common:trpc_plugin`。
- 引入的trpc依赖bazel依赖规则，和文件名保持良好的对应关系，一般情况下，您只需要参考按照解决办法处理编写引入bazel的规则即可。

#### ccache: error: Failed to create temporary file for xxx: Read-only file system

- 原因：执行bazel命令的用户没有 `~/.ccache` 某些特定文件的写权限，常见于环境被因用root用户权限执行了其他命令污染了环境。
- 解决办法：
  - 可以加上bazel构建选项 `--sandbox_writable_path ~/.ccache`。
  - 或者直接删掉目录 `sudo rm -rf ~/.ccache`，再重新编译。

#### 如何静态编译

- 场景：服务部署机器gcc版本低，且很难升级，通过静态编译，可以让在高版本gcc上编译的程序运行在低版本的机器上。
- 方法：执行命令前，设置bazel相关环境变量: `BAZEL_LINKOPTS=-static-libstdc++ BAZEL_LINKLIBS=-l%:libstdc++.a bazel build //your_server_bin`。

#### 静态编译时报错缺少数学库

- 原因：静态编译时没有链math库
- 解决方法：编译前，加上环境变量即可 `BAZEL_LINKOPTS=-lm`。

#### 如何定义框架三方版本

- 场景：集成框架到您的系统时，但希望框架依赖的三方与您的三方版本保持一致。
- 解决办法：在WORKSPACE文件里引入trpc依赖函数传入参数 `trpc_workspace(xxx_ver="xxx", xxx_sha256="xxx")`，xxx_ver定义指定三方版本，xxx_sha256防止文件被篡改，您可以参考 [workspace.bzl](trpc/workspace.bzl) 查阅可以指定哪些三方的版本。

### CMake常见问题

#### 使用CMake编译后，再使用Bazel编译，导致报错

- 原因：CMake的三方依赖会下载到项目根目录下 `cmake_third_party`，执行 `bazel build ...` 时，会导致bazel扫描并编译 `cmake_third_party` 下的文件夹，可能导致编译失败。
- 解决办法：
  - CMake和Bazel二选一，不要混用
  - Bazel编译时，总是带上要构建的目标。比如：`bazel build //trpc/...` 就只会编译trpc及其子目录下的所有编译目标。

#### 执行CMake报错：Could NOT find CURL (missing: CURL_LIBRARY CURL_INCLUDE_DIR)

- 原因：环境中缺少 `curl` 的开发库
- 解决办法：安装 `libcurl-devel`。

```bash
# centos下
yum install libcurl-devel 
# ubuntu下
apt-get install libcurl4-openssl-dev
```

#### make install执行失败

- 原因：常见于执行命令的用户无 `/usr/local/trpc-cpp` 目录的读写权限，执行 `make install` 将会在该目录下安装头文件及库。
- 解决办法：
  1. 给予执行命令的用户读写权限，或者切换成root用户。
  2. 采用CMake源码依赖的引入方式。

#### cannot find -lxxx

- 原因：常见于通过 `make install` 的方式使用tRPC-Cpp，在编译tRPC-Cpp时开启了某些编译选项，但用户项目里没有包含相应的依赖。
- 解决办法：比如，开启Prometheus插件编译，`cmake -DTRPC_BUILD_WITH_METRICS_PROMETHEUS=ON ..`，并安装了框架后，在链接用户的程序时，需要加上 `$LIB_METRICS_PROMETHEUS` ，其他依赖库的定义见[trpc_config.cmake](https://github.com/trpc-group/trpc-cpp/blob/main/cmake/config/trpc_config.cmake)。

#### No rule to make target 'bin/trpc_cpp_plugin'

- 原因：常见于通过 `make install` 的方式使用tRPC-Cpp，在使用tRPC-Cpp提供的CMake方法 `TRPC_COMPILE_PROTO` 时，传入了错误的桩代码生成工具二进制路径，一般是在 `/usr/local/trpc-cpp/trpc/bin` 里。
- 解决办法：定位到安装的位置，然后设置对，对于CMake方法需要 `protoc` 也需要设置对。

#### 如何定义框架依赖三方库的版本

- 场景：集成框架到您的系统时，但希望框架依赖的三方与您的三方版本保持一致。
- 解决办法：编译时，提前引入自己的三方版本，在编译框架时，检测到三方已经被引入之后，就不会重复引入，可以参考 [cmake/protobuf.cmake](https://github.com/trpc-group/trpc-cpp/blob/main/cmake/protobuf.cmake) 替换成为您自己的版本。
