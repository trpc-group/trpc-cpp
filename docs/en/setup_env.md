[中文](../zh/setup_env.md)

# Overview

This article introduces how to setup the development environment for tRPC-Cpp framework.

# Summary of Environmental Requirements

Operating System and Version: Currently, the recommended operating system is the Linux kernel, preferably with a **Linux kernel version of 2.6 or above**. CentOS and Ubuntu distributions within Linux are recommended. Other operating systems such as Windows and MacOS are currently not supported.

Instruction Set: It is currently recommended to run on the **x86-64 instruction set and ARM instruction set** and not recommended to run on other instruction sets.

GCC Version: Since tRPC-Cpp utilizes C++17 standard features, it is recommended to use **GCC version 7 or above**.

# Specific Environment Guidelines

tRPC-Cpp framework relies on third-party libraries (such as protobuf, gflags, fmtlib) for its source code dependencies. Therefore, additional installation of dependency libraries is usually not required. Once the compilation environment is set up, it is typically sufficient to clone the code repository and compile it directly for execution.

Below is a detailed guide on how to compile and run tRPC-Cpp framework on CentOS and Ubuntu.

## CentOS

It recommend using **CentOS version 7.9 or above** for compiling and running tRPC-Cpp.

Currently, there are two supported compilation methods for tRPC-Cpp: Bazel and CMake.

### Compile and run using Bazel in CentOS

It is recommended to use Bazel version 3.5.1 or later when compiling and running tRPC-Cpp.

1. **Install bazel(For reference only, there are other methods available)**

     ```sh
     # install jdk
     yum install java-11-openjdk
     yum install java-11-openjdk-devel
     
     # installbazel
     mkdir -p /root/env/bazel
     wget https://github.com/bazelbuild/bazel/releases/download/3.5.1/bazel-3.5.1-installer-linux-x86_64.sh -O /root/env/bazel/bazel.sh
     chmod +x /root/env/bazel/bazel.sh
     /root/env/bazel/bazel.sh > /dev/null
     rm -rf /root/env
     
     # check version
     bazel --version
     ```

2. **build and run tRPC-Cpp examples**

    After cloning the repository to your local machine, you can compile and run it directly. Here are the specific steps:

    ```sh
    cd trpc-cpp

    # Compile and run the examples provided by the framework using Bazel
    ./run_examples.sh
    ```

### Compile and run using Cmake in CentOS

Since the FetchContent feature of CMake is required for fetching third-party dependencies from source code, it is recommended to use **CMake version 3.14 or later**.

1. **Install cmake(For reference only, there are other methods available)**

   ``` sh
   yum install -y cmake3
   ln -s /usr/bin/cmake3 /usr/bin/cmake
   
   # check version
   cmake -version
   ```

2. **build and run tRPC-Cpp examples**

   After cloning the repository to your local machine, you can compile and run it directly. Here are the specific steps:
 
    ```sh
    cd trpc-cpp
    
    # Compile and run the examples provided by the framework using Cmake
    ./run_examples_cmake.sh
    ```

3. **How to use tRPC-Cpp**

a. (Recommend) Use as external source code

We recommend this as it can easily switch tRPC-Cpp version to the lastest and the libs framework depends on can also be imported via a simple SDK target named `trpc`.

For example, you can import in CMakeLists.txt in this way:

```shell
# Fetch tRPC-Cpp and add as library
include(FetchContent)
FetchContent_Declare(
    trpc-cpp
    GIT_REPOSITORY    https://github.com/trpc-group/trpc-cpp.git
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

b.  Use via make install

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

## Ubuntu

It recommend using **Ubuntu version 20.04 LTS or above** for compiling and running tRPC-Cpp.

Currently, there are two supported compilation methods for tRPC-Cpp: Bazel and CMake.

### Compile and run using Bazel in Ubuntu

It is recommended to use Bazel version 3.5.1 or later when compiling and running tRPC-Cpp.

1. **Install bazel(For reference only, there are other methods available)**

   ```sh
   # install jdk
   apt install g++ unzip zip
   apt-get install default-jdk
   
   # install bazel
   mkdir -p /root/env/bazel
   wget https://github.com/bazelbuild/bazel/releases/download/3.5.1/bazel-3.5.1-installer-linux-x86_64.sh -O /root/env/bazel/bazel.sh
   chmod +x /root/env/bazel/bazel.sh
   /root/env/bazel/bazel.sh > /dev/null
   rm -rf /root/env
   
   # check version
   bazel --version
   ```

2. **build and run tRPC-Cpp examples**

    After cloning the repository to your local machine, you can compile and run it directly. Here are the specific steps:

    ```sh
    cd trpc-cpp

    # Compile and run the examples provided by the framework using Bazel
    /run_examples.sh
    ```

### Compile and run using Cmake in Ubuntu

Since the FetchContent feature of CMake is required for fetching third-party dependencies from source code, it is recommended to use **CMake version 3.14 or later**.

1. **Install cmake(For reference only, there are other methods available)**

   ```sh
   apt install cmake
   
   # check version
   cmake -version
   ```

2. **build and run tRPC-Cpp examples**

    After cloning the repository to your local machine, you can compile and run it directly. Here are the specific steps:

   ```sh
   cd trpc-cpp
   
   # Compile and run the examples provided by the framework using Cmake
   ./run_examples_cmake.sh
   ```

3. **How to use tRPC-Cpp**

Same as CentOS section

# FAQ

## bazel --version" shows a "command not found" error？

You need to add the directory of the bazel executable to the PATH environment variable. For example, you can add the following line to your .bashrc file: "export PATH=$PATH:/usr/local/bin/

## How to Install GCC?

- yum command

  ```sh
  yum install centos-release-scl-rh centos-release-scl
  yum install devtoolset-7-gcc devtoolset-7-gcc-c++
  scl enable devtoolset-7 bash
  ```

- Compile use Source Code

  ```sh
  git clone git://gcc.gnu.org/git/gcc.git yourSomeLocalDir
  ./configure 
  make 
  make install
  ```

Compiling from source code may take a longer time.

## Sometimes compilation fails?

It could be due to a network issue causing the fetch to fail. Consider cleaning the build cache (bazel clean --expunge) and retrying the compilation.

## Compilation error: missing members of the std standard library?

It's possible that C++ has not been specified with the C++17 standard.

```sh
# add this
build --cxxopt="--std=c++17"
```

It is also recommended to add it to the .bazelrc file so that it can take effect by default

See more about [bazel cmake faq](../zh/faq/bazel_and_cmake_problem.md).
