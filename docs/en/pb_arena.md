[中文](../zh/pb_arena.md)

# Overview

This article mainly introduces how to optimize RPC services with arena. Developers can learn the following contents:

* The principle of arena.
* How to use arena in RPC server.
* How to use arena when calling RPC services.
* FAQ.

# The principle of arena

[Arena](https://developers.google.com/protocol-buffers/docs/reference/overview)
is a feature supported by protobuf starting from version 3.0, which is used to take over the lifecycle of PB objects.
Using it can help you optimize memory usage and improve performance in your program.

It's recommended to read:
[Why Use Arena Allocation?](https://developers.google.com/protocol-buffers/docs/reference/arenas#why)

Arena is used by the protobuf library to take over the memory management of PB objects. Its principle is simple:

* protobuf pre-allocates a memory block, and PB objects created multiple times reuse this memory
block.
* When parsing and constructing messages, objects are "placement new" on the allocated memory block. 
* When the objects on the arena are destructed, all memory is released, and ideally, no destructor of the contained objects needs to be called.

Benefits:

* Reduce the system overhead caused by multiple malloc/free and destructor calls in complex PB objects.
* Reduce memory fragmentation.
* PB objects have contiguous memory, are cache line friendly, and have high read performance.

Applicable scenarios: PB structures are complex, and repeated type fields contain a large number of data.

# How to use arena in RPC server

The lifecycle of PB objects is managed by the framework and can be optimized by enabling arena through compilation
options.

## Enabling arena on server-side

* **Enable arena using proto option**

  We need to enable arena allocation in each `.proto` file, so we need to add the following `option` to each `.proto`
file:

  ```protobuf
  option cc_enable_arenas = true;
  ```

* **Enable arena option during compilation**

  Add the following compilation option to `.bazelrc`:

  ```sh
  build --define trpc_proto_use_arena=true
  ```

Performing the above two steps will enable arena optimization.

## Setting a custom arena creation function (optional)

By default, the framework calls the default constructor of the arena to create an arena object. If you want to set a
custom arena creation function, you can refer to the following method.
*Tip: Not setting a custom arena creation function does not affect the use of arena features.*

``` c++
/// @brief Set the ArenaOptions
///        the user can set this function to realize the function of customizing Arena's Options
///        if not set, use default option
void SetGlobalArenaOptions(const google::protobuf::ArenaOptions& options);
```

The function needs to return a `google::protobuf::Arena` object (without performance loss through RVO), and we can
construct an arena object and specify initial memory size and other parameters through `google::protobuf::ArenaOptions`.

Register the global `arena` configuration in the `Initialize()` function. Here is a demo (specifying an initial
memory size of 10K each time an arena is created):

``` c++
int XxxxServer::Initialize() {
    // Set the parameters of options on demands. 
    // Only set the start_block_size here.
    google::protobuf::ArenaOptions options;
    options.start_block_size = 10240;

    SetGlobalArenaOptions(options);
}
```

# How to use arena when calling RPC services

In this case, PB objects are created by the user, and the lifecycle of PB objects is managed by user code. The framework
cannot intervene in the lifecycle of PB objects. Therefore, users need to use arena to optimize memory by themselves.

## Enabling arena on client-side

* **Enable arena using proto option**

  We need to enable arena allocation in each `.proto` file, so we need to add the following `option` to each `.proto`
file:

  ```protobuf
  option cc_enable_arenas = true;
  ```

* **Creating a PB object**

  When calling an RPC service on the client side, if you need to use an arena, you need to actively invoke the `google::protobuf::Arena::CreateMessage<MyMessage>(...)` interface. On the RPC server side, you don't need the above steps; you just need to enable the arena through compilation options.

  ```c++
  // include arena.h
  #include "google/protobuf/arena.h"
  
  google::protobuf::Arena arena_req;
  auto* req = google::protobuf::Arena::CreateMessage<RequestType>(&arena_req);
  
  google::protobuf::Arena arena_rsp;
  auto* rsp = google::protobuf::Arena::CreateMessage<ResponseType>(&arena_rsp);
  ```
  
  Line 2: Include the header file required for arena.
  
  Lines 4-7: Define two arena objects to manage the memory of `req` and `rsp`, respectively.
  
  Lines 5 and 8: Use two arenas to create two PB objects.

## Precautions

All objects maintained by arena are released uniformly when arena is destroyed. During use, the internally maintained
memory block of arena will only be continuously appended and will not be deleted. Therefore, the objects maintained by
arena can only be either local objects or immutable. Otherwise, its memory will continue to grow indefinitely.

* It is recommended not to reuse arena objects, and one arena manages the lifecycle of one PB object.
  * Reusing may cause arena expansion and affect performance.
  * One arena generates multiple PB objects, and the lifecycle is difficult to manage.
* The lifecycle of arena objects should be either local or immutable.
* The lifecycle of arena is longer than that of the objects it manages.
* PB objects created using CreateMessage cannot be deleted.

Improper use may cause OOM or core dump in the service, so you need to understand the principle before using it.

# Performance data

After testing, when the PB structure is large and complex, using arena can improve performance by about `14%`.

## Test device configuration

```txt
// Cloud server.
Architecture:          x86_64
CPU(s):                8
On-line CPU(s) list:   0-7
Thread(s) per core:    1
Model name:            AMD EPYC 7K62 48-Core Processor
Stepping:              0
CPU MHz:               2595.124
Hypervisor vendor:     KVM
L1d cache:             32K
L1i cache:             32K
L2 cache:              4096K
L3 cache:              16384K
```

## Test logic

To simulate a real scenario, we use the proto file definition of a business.
Test logic:

* Concurrently start 4 threads, and each thread executes 500 tasks in a loop.
* In each task, create two complex PB structures, Request and Response, and fill in the repeated fields so that the
  ByteSizeLong() of the structures is about 200K.

## Compilation options

Enable `O2` compilation and distinguish whether to use the tc_malloc library.

## Test results

|                            | Not linked with tc_malloc | Linked with tc_malloc |
|----------------------------|---------------------------|-----------------------|
| Without using arena        | 6024ms                    | 5129ms                |
| Using arena optimization   | 5198ms                    | 4510ms                |
| Improvement ratio of arena | 13.8%                     | 14.1%                 |

# FAQ

## The program crashes with the message: `CHECK failed: GetArenaNotVirtual() == nullptr`

PB objects created using CreateMessage are `not allowed to be deleted`. Check whether the PB object has been destroyed.
Only destroying the object created by CreateMessage will trigger this error.
