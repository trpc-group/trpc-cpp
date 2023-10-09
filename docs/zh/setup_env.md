[TOC]
# 环境搭建

## 前言

这篇文章介绍如何搭建tRPC Cpp的编译运行环境。

## 环境要求总述

操作系统及版本:目前推荐操作系统为Linux内核，推荐**Linux内核版本在2.6及以上**。推荐使用Linux中CentOS及Ubuntu发行版本。另外暂不支持如Windows、MacOS等其他操作系统。

指令集:目前推荐在**x86-64指令集及ARM指令集**下运行，不推荐在其他指令集下运行。

GCC版本:因为tRPC Cpp使用了C++17标准特性，推荐使用**GCC 7及以上版本**。


## 具体环境指引

tRPC Cpp**源码依赖**所需的第三方库(如protobuf、gflags、fmtlib等)，因此**通常不需要额外安装依赖库**。在搭建好编译环境之后，通常只需要拉取代码直接编译运行即可。


下面详细介绍如何在CentOS及Ubuntu编译运行tRPC Cpp。

### CentOS
推荐**CentOS版本在7.9及以上版本**。


目前支持两种编译方式:bazel或者cmake。

#### 使用bazel编译运行

bazel 推荐使用3.5.1及以后版本。

**1. bazel安装(仅供参考，还有其他方式)**

``` 
## bazel 安装依赖jdk，先安装jdk
yum install java-11-openjdk
yum install java-11-openjdk-devel

## 安装bazel
mkdir -p /root/env/bazel
wget https://github.com/bazelbuild/bazel/releases/download/3.5.1/bazel-3.5.1-installer-linux-x86_64.sh -O /root/env/bazel/bazel.sh
chmod +x /root/env/bazel/bazel.sh
/root/env/bazel/bazel.sh > /dev/null
rm -rf /root/env

## 查看版本
bazel --version
```

**2. example编译并运行**

仓库clone到本地之后，直接编译运行即可，具体如下：
``` 
## 进入根目录trpc-cpp
cd trpc-cpp

## 用bazel编译并运行框架提供的example实例
./run_examples.sh
```

#### 使用cmake编译运行
因为需要使用cmake的FetchContent特性来源码拉取第三方依赖，所以**cmake推荐使用3.14及以后版本**。

**1. cmake安装(仅供参考，还有其他方式)**

``` 
yum install -y cmake3
ln -s /usr/bin/cmake3 /usr/bin/cmake

## 查看版本
cmake -version
```

**2. example编译并运行**

仓库clone到本地之后，直接编译运行即可，具体如下：
``` 
## 进入根目录trpc-cpp
cd trpc-cpp

## 用cmake编译并运行框架提供的example实例
./run_examples_cmake.sh
```

### Ubuntu
推荐**Ubuntu版本在20.04 LTS及以上版本**。


下面介绍如何在Ubuntu编译运行tRPC Cpp。

目前支持两种编译方式:bazel或者cmake。

#### 使用bazel编译运行

bazel 推荐使用3.5.1及以后版本。

**1. bazel安装(仅供参考，还有其他方式)**

``` 
## bazel 安装依赖jdk，先安装jdk
apt install g++ unzip zip
apt-get install default-jdk

## 安装bazel
mkdir -p /root/env/bazel
wget https://github.com/bazelbuild/bazel/releases/download/3.5.1/bazel-3.5.1-installer-linux-x86_64.sh -O /root/env/bazel/bazel.sh
chmod +x /root/env/bazel/bazel.sh
/root/env/bazel/bazel.sh > /dev/null
rm -rf /root/env

## 查看版本
bazel --version 
```

**2. example编译并运行**

仓库clone到本地之后，直接编译运行即可，具体如下：
``` 
## 进入根目录trpc-cpp
cd trpc-cpp

## 用bazel编译并运行框架提供的example实例
./run_examples.sh
```

#### 使用cmake编译运行
因为需要使用cmake的FetchContent特性来源码拉取第三方依赖，所以**cmake推荐使用3.14及以后版本**。

**1. cmake安装(仅供参考，还有其他方式)**

``` 
apt install cmake

## 查看版本
cmake -version
```

**2. example编译并运行**

仓库clone到本地之后，直接编译运行即可，具体如下：
``` 
## 进入根目录trpc-cpp
cd trpc-cpp

## 用cmake编译并运行框架提供的example实例
./run_examples_cmake.sh
```
## FAQ
### 1. bazel --version出现找不到bazel命令?
需要将bazel可执行文件的目录加到环境变量PATH上，比如：在.bash.rc加入export PATH=$PATH:/usr/local/bin/

### 2. GCC如何安装?
方式1 yum

``` 
yum install centos-release-scl-rh centos-release-scl
yum install devtoolset-7-gcc devtoolset-7-gcc-c++
scl enable devtoolset-7 bash
``` 
方式2 源码编译

```
// 下载源码
git clone git://gcc.gnu.org/git/gcc.git yourSomeLocalDir
./configure 
make 
make install
```
源码编译可能时间会比较久一些。

### 3. 偶先编译失败?
可能是网络问题导致拉取失败，考虑清理编译缓存(bazel clean --expunge)再次重试编译。

### 4. 编译提示std标准库成员找不到?
c++可能没有指定c++17标准
```
// 编译加上
build --cxxopt="--std=c++17"
```
也推荐在.bazelrc文件中加上，这样即可默认生效。


更多涉及编译的可以参考[bazel编译](./bazel_faq.md)、[cmake编译](./cmake_faq.md)