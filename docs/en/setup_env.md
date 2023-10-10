[TOC]
# Setup Environment 

## Overview

This article introduces how to setup the development environment for tRPC-Cpp.

## Summary of Environmental Requirements

Operating System and Version: Currently, the recommended operating system is the Linux kernel, preferably with a **Linux kernel version of 2.6 or above**. CentOS and Ubuntu distributions within Linux are recommended. Other operating systems such as Windows and MacOS are currently not supported.

Instruction Set: It is currently recommended to run on the **x86-64 instruction set and ARM instruction set** and not recommended to run on other instruction sets.

GCC Version: Since tRPC-Cpp utilizes C++17 standard features, it is recommended to use **GCC version 7 or above**.


## Specific Environment Guidelines

tRPC-Cpp relies on third-party libraries (such as protobuf, gflags, fmtlib) for its source code dependencies. Therefore, additional installation of dependency libraries is usually not required. Once the compilation environment is set up, it is typically sufficient to clone the code repository and compile it directly for execution.


Below is a detailed guide on how to compile and run tRPC-Cpp on CentOS and Ubuntu.

### CentOS

It recommend using **CentOS version 7.9 or above** for compiling and running tRPC-Cpp.

Currently, there are two supported compilation methods for tRPC-Cpp: Bazel and CMake.

#### Compile and run using Bazel

It is recommended to use Bazel version 3.5.1 or later when compiling and running tRPC-Cpp.

**1. Install bazel(For reference only, there are other methods available)**

``` 
## install jdk
yum install java-11-openjdk
yum install java-11-openjdk-devel

## installbazel
mkdir -p /root/env/bazel
wget https://github.com/bazelbuild/bazel/releases/download/3.5.1/bazel-3.5.1-installer-linux-x86_64.sh -O /root/env/bazel/bazel.sh
chmod +x /root/env/bazel/bazel.sh
/root/env/bazel/bazel.sh > /dev/null
rm -rf /root/env

## check version
bazel --version
```

**2. build and run tRPC-Cpp examples**

After cloning the repository to your local machine, you can compile and run it directly. Here are the specific steps:
``` 
cd trpc-cpp

## Compile and run the examples provided by the framework using Bazel
./run_examples.sh
```

#### Compile and run using Cmake

Since the FetchContent feature of CMake is required for fetching third-party dependencies from source code, it is recommended to use ** CMake version 3.14 or later**.

**1. Install cmake(For reference only, there are other methods available)**

``` 
yum install -y cmake3
ln -s /usr/bin/cmake3 /usr/bin/cmake

## check version
cmake -version
```

**2. build and run tRPC-Cpp examples**

After cloning the repository to your local machine, you can compile and run it directly. Here are the specific steps:
``` 
cd trpc-cpp

## Compile and run the examples provided by the framework using Cmake
./run_examples_cmake.sh
```

### Ubuntu

It recommend using **Ubuntu version 20.04 LTS or above** for compiling and running tRPC-Cpp.

Currently, there are two supported compilation methods for tRPC-Cpp: Bazel and CMake.

#### Compile and run using Bazel

It is recommended to use Bazel version 3.5.1 or later when compiling and running tRPC-Cpp.

**1. Install bazel(For reference only, there are other methods available)**

``` 
## install jdk
apt install g++ unzip zip
apt-get install default-jdk

## install bazel
mkdir -p /root/env/bazel
wget https://github.com/bazelbuild/bazel/releases/download/3.5.1/bazel-3.5.1-installer-linux-x86_64.sh -O /root/env/bazel/bazel.sh
chmod +x /root/env/bazel/bazel.sh
/root/env/bazel/bazel.sh > /dev/null
rm -rf /root/env

## check version
bazel --version  
```
**2. build and run tRPC-Cpp examples**

After cloning the repository to your local machine, you can compile and run it directly. Here are the specific steps:
``` 
cd trpc-cpp

## Compile and run the examples provided by the framework using Bazel
./run_examples.sh
```

#### Compile and run using Cmake

Since the FetchContent feature of CMake is required for fetching third-party dependencies from source code, it is recommended to use ** CMake version 3.14 or later**.

**1. Install cmake(For reference only, there are other methods available)**

``` 
apt install cmake

## check version
cmake -version
```

**2. build and run tRPC-Cpp examples**

After cloning the repository to your local machine, you can compile and run it directly. Here are the specific steps:
``` 
cd trpc-cpp

## Compile and run the examples provided by the framework using Cmake
./run_examples_cmake.sh
```

## FAQ
### 1. bazel --version" shows a "command not found" error？
You need to add the directory of the bazel executable to the PATH environment variable. For example, you can add the following line to your .bashrc file: "export PATH=$PATH:/usr/local/bin/

### 2. How to Install GCC?
1 yum

``` 
yum install centos-release-scl-rh centos-release-scl
yum install devtoolset-7-gcc devtoolset-7-gcc-c++
scl enable devtoolset-7 bash
``` 
2 Compile use Source Code 

```
git clone git://gcc.gnu.org/git/gcc.git yourSomeLocalDir
./configure 
make 
make install
```
Compiling from source code may take a longer time.

### 3. Sometimes compilation fails?
It could be due to a network issue causing the fetch to fail. Consider cleaning the build cache (bazel clean --expunge) and retrying the compilation.

### 4. Compilation error: missing members of the std standard library?
It's possible that C++ has not been specified with the C++17 standard.
```
// add this
build --cxxopt="--std=c++17"
```
It is also recommended to add it to the .bazelrc file so that it can take effect by default


See more about [compile use bazel](./bazel_faq.md)、[compile use cmake](./cmake_faq.md)
