[English](../en/setup_env.md)


# 前言

这篇文章介绍如何搭建tRPC-Cpp 框架的编译运行环境。

# 环境要求总述

操作系统及版本:目前推荐操作系统为 Linux 内核，推荐**Linux内核版本在2.6及以上**。推荐使用 Linux 中 CentOS 及 Ubuntu 发行版本。另外暂不支持如 Windows、MacOS 等其他操作系统。

指令集：目前推荐在**x86-64指令集及ARM指令集**下运行，不推荐在其他指令集下运行。

GCC 版本：因为 tRPC-Cpp 使用了 C++17 标准特性，推荐使用**GCC 7及以上版本**。

# 具体环境指引

tRPC-Cpp 框架**源码依赖**所需的第三方库(如：protobuf、gflags、fmtlib等)，因此**通常不需要额外安装依赖库**。在搭建好编译环境之后，通常只需要拉取代码直接编译运行即可。

下面详细介绍如何在 CentOS 及 Ubuntu 编译运行 tRPC-Cpp 框架。

## CentOS

推荐**CentOS版本在7.9及以上版本**。

目前支持两种编译方式：bazel 或者 cmake。

### 使用 bazel 编译运行

bazel 推荐使用 3.5.1及以后版本。

1. **bazel 安装(仅供参考，还有其他方式)**

   ```sh
   # bazel 安装依赖jdk，先安装jdk
   yum install java-11-openjdk
   yum install java-11-openjdk-devel
   
   # 安装bazel
   mkdir -p /root/env/bazel
   wget https://github.com/bazelbuild/bazel/releases/download/3.5.1/bazel-3.5.1-installer-linux-x86_64.sh -O /root/env/bazel/bazel.sh
   chmod +x /root/env/bazel/bazel.sh
   /root/env/bazel/bazel.sh > /dev/null
   rm -rf /root/env
   
   # 查看版本
   bazel --version
   ```

2. **example 编译并运行**

   仓库 clone 到本地之后，直接编译运行即可，具体如下：

   ```sh
   # 进入根目录trpc-cpp
   cd trpc-cpp
   
   # 用bazel编译并运行框架提供的example示例
   ./run_examples.sh
   ```

### 使用 cmake 编译运行

因为需要使用 cmake 的 FetchContent 特性来源码拉取第三方依赖，所以**cmake推荐使用3.14及以后版本**。

1. **cmake 安装(仅供参考，还有其他方式)**

   ```sh
   yum install -y cmake3
   ln -s /usr/bin/cmake3 /usr/bin/cmake
   
   # 查看版本
   cmake -version
   ```

2. **编译并运行examples**

   仓库clone到本地之后，直接编译运行即可，具体如下：

   ```sh
   # 进入根目录trpc-cpp
   cd trpc-cpp
   
   # 用cmake编译并运行框架提供的examples
   ./run_examples_cmake.sh
   ```

3. **如何引入tRPC-Cpp**

a. （推荐）以外部库形式源码引入

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

b. 通过make install引入

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

## Ubuntu

推荐**Ubuntu 版本在 20.04 LTS 及以上版本**。

下面介绍如何在 Ubuntu 编译运行 tRPC-Cpp 框架。

目前支持两种编译方式:

* bazel
* cmake。

### 使用 bazel

bazel 推荐使用3.5.1及以后版本。

1. **bazel 安装(仅供参考，还有其他方式)**

   ```sh
   # bazel 安装依赖jdk，先安装jdk
   apt install g++ unzip zip
   apt-get install default-jdk
   
   # 安装bazel
   mkdir -p /root/env/bazel
   wget https://github.com/bazelbuild/bazel/releases/download/3.5.1/bazel-3.5.1-installer-linux-x86_64.sh -O /root/env/bazel/bazel.sh
   chmod +x /root/env/bazel/bazel.sh
   /root/env/bazel/bazel.sh > /dev/null
   rm -rf /root/env
   
   # 查看版本
   bazel --version 
   ```

2. **编译并运行 examples**

   仓库 clone 到本地之后，直接编译运行即可，具体如下：

   ```sh
   # 进入根目录trpc-cpp
   cd trpc-cpp
   
   # 用bazel编译并运行框架提供的example示例
   ./run_examples.sh
   ```

### 使用 cmake

因为需要使用 cmake 的 FetchContent 特性来源码拉取第三方依赖，所以**cmake 推荐使用3.14及以后版本**。

1. **cmake 安装(仅供参考，还有其他方式)**

   ```sh
   apt install cmake
   
   # 查看版本
   cmake -version
   ```

2. **编译并运行 examples**

   仓库 clone 到本地之后，直接编译运行即可，具体如下：

   ```sh
   # 进入根目录trpc-cpp
   cd trpc-cpp
   
   # 用cmake编译并运行框架提供的example示例
   ./run_examples_cmake.sh
   ```

3. **安装及使用**

同CentOS 部分。


# FAQ

## bazel --version 出现找不到 bazel 命令?

需要将 bazel 可执行文件的目录加到环境变量 PATH 上，比如：在 `.bash.rc` 加入 `export PATH=$PATH:/usr/local/bin/`

## GCC 如何安装?

* 方式1 yum

  ```sh
  yum install centos-release-scl-rh centos-release-scl
  yum install devtoolset-7-gcc devtoolset-7-gcc-c++
  scl enable devtoolset-7 bash
  ```

* 方式2 源码编译

  ```sh
  // 下载源码
  git clone git://gcc.gnu.org/git/gcc.git yourSomeLocalDir
  ./configure 
  make 
  make install
  ```

  源码编译可能时间会比较久一些。
  
## 偶现编译失败?

可能是网络问题导致拉取失败，考虑清理编译缓存(bazel clean --expunge)再次重试编译。

## 编译提示 std 标准库成员找不到?

C++ 可能没有指定 c++17 标准

```sh
# 编译加上
build --cxxopt="--std=c++17"
```

也推荐在 `.bazelrc` 文件中加上，这样即可默认生效。

更多涉及编译的问题可以参考 [bazel cmake faq](../zh/faq/bazel_and_cmake_problem.md)。
