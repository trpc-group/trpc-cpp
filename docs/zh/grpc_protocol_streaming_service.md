[English](../en/grpc_protocol_streaming_service.md)

# gRPC 流式协议服务开发指南

**主题：如何基于 tRPC-Cpp 开发 gRPC 流式服务**

背景：一些使用 gRPC 框架开发的服务需要和 tRPC-Cpp 框架以 gRPC 流式协议进行通信。

本文介绍如何基于 tRPC-Cpp （下面简称 tRPC）开发 gRPC 流式服务，开发者可以了解到如下内容：

* 如何开发 gRPC 流式服务。
* FAQ。

# 如何开发 gRPC 流式服务

## 快速上手

### 体验 gRPC 流式服务

示例 [grpc_stream](../../examples/features/grpc_stream)

方法：进入 tRPC-Cpp 代码仓库主目录，然后运行下面的命令。

```shell
sh examples/features/grpc_stream/run.sh
```

示例程序输出：

``` text
DB parsed, loaded 100 features.
-------------- GetFeature --------------
Found feature called BerkshireValleyManagementAreaTrail,Jefferson,NJ,USA at 40.9146, -74.6189
Found no feature at 0, 0
-------------- ListFeatures --------------
Looking for features between 40, -75 and 42, -73
Found feature called PatriotsPath,Mendham,NJ07945,USA at 40.7838, -74.6144
Found feature called 101NewJersey10,Whippany,NJ07981,USA at 40.8123, -74.3999
...
ListFeatures rpc succeeded.
-------------- RecordRoute --------------
Visiting point 41.2676, -74.2607
Visiting point 40.8123, -74.3999
...
Finished trip with 11 points
Passed 8 features
Travelled 817791 meters
It took 9 seconds
-------------- RouteChat --------------
Sending message First message at 0, 0
Sending message Second message at 0, 1
...
Got message First message at 0, 0
```

### 特性支持情况说明

| 特性         | 支持情况         |
|------------|--------------|
| 服务端流       | 已支持          |
| 客户端流       | 已支持          |
| 双端流        | 已支持          |
| 序列化/反序列化类型 | ProtoBuffers |
| 解压缩        | 计划支持         |

限制：

- 当前仅支持 fiber 线程模型（即 同步流式服务），merge 和 separate 线程模型暂未支持。
- 当前在服务端上可用，客户端暂不可用（支持中）。

### 开发步骤

示例 [grpc_stream](../../examples/features/grpc_stream)

参考 [tRPC 流式协议服务开发指南](./trpc_protocol_streaming_service.md)

tRPC 中的 gRPC 流式协议实现复用了 tRPC 的流式接口设计，相应的流式编程接口复用了 tRPC
的流读写器接口。开发步骤也和 tRPC 流式协议服务开发步骤一致，不同之处在于 yaml 配置项 `protocol: trpc` -> `protocol: grpc`。

```yaml
global:
  # 线程模型需要使用 fiber
  threadmodel:
    fiber:
      - instance_name: fiber_instance

server:
  service:
    - name: xx_service_name
      # 使用 grpc 协议
      protocol: grpc
      ...
```

### 流式服务端示例

- 为了方便用户使用 tRPC 提供的流式读写器接口，示例代码重写了 gRPC 框架提供的服务端流式示例，方便对照参考。
    - [gRPC 框架流式服务端示例代码](https://github.com/grpc/grpc/blob/master/examples/cpp/route_guide/route_guide_server.cc)
    - [tRPC 框架流式服务端示例代码](../../examples/features/grpc_stream/server/stream_service.cc)

*服务端流式*

差异点：

- 向流中 `Write` 接口返回值差异：gRPC 框架的 `Write` 接口返回 bool 代表成功或失败，而 tRPC 的 `Write` 接口返回 `Status`
  ，除了能代表写成功还是写失败，还包含写失败的原因（`status.ToString()` 或者 `status.ErrorString()`）。

```cpp
// gRPC 框架示例代码
// @file:route_guide_server.cc 

Status ListFeatures(ServerContext* context,
                    const routeguide::Rectangle* rectangle,
                    ServerWriter<Feature>* writer) override {
  auto lo = rectangle->lo();
  auto hi = rectangle->hi();
  long left = (std::min)(lo.longitude(), hi.longitude());
  long right = (std::max)(lo.longitude(), hi.longitude());
  long top = (std::max)(lo.latitude(), hi.latitude());
  long bottom = (std::min)(lo.latitude(), hi.latitude());
  for (const Feature& f : feature_list_) {
    if (f.location().longitude() >= left &&
        f.location().longitude() <= right &&
        f.location().latitude() >= bottom && f.location().latitude() <= top) {
      writer->Write(f);
    }
  }
  return Status::OK;
}
```

```cpp
// tRPC 框架 gRPC 流式服务示例代码
// @file:stream_service.cc 

::trpc::Status RouteGuideImpl::ListFeatures(const ::trpc::ServerContextPtr& context,
                                            const ::routeguide::Rectangle& rectangle,
                                            ::trpc::stream::StreamWriter<::routeguide::Feature>* writer) {
  ::trpc::Status status{};
  auto lo = rectangle.lo();
  auto hi = rectangle.hi();
  int32_t left = (std::min)(lo.longitude(), hi.longitude());
  int32_t right = (std::max)(lo.longitude(), hi.longitude());
  int32_t top = (std::max)(lo.latitude(), hi.latitude());
  int32_t bottom = (std::min)(lo.latitude(), hi.latitude());
  for (const ::routeguide::Feature& f : feature_list_) {
    if (f.location().longitude() >= left && f.location().longitude() <= right && f.location().latitude() >= bottom &&
        f.location().latitude() <= top) {
      status = writer->Write(f);
      if (status.OK()) {
        TRPC_FMT_INFO("ListFeatures write feature, name= {}, location.latitude: {}, location.longitude: {}", f.name(),
                      f.location().latitude(), f.location().longitude());
        continue;
      }
      TRPC_FMT_ERROR("ListFeatures stream got error: {}", status.ToString());
      break;
    }
  }
  return status;
}
```

*客户端流式*

差异点：

- 从流中 `Read` 接口返回值差异：gRPC 框架的 `Read` 返回 bool 类型，流结束（客户端已经发完所有请求数据）不能与流异常显示地区分，而
  tRPC 框架的 `Read` 返回 Status 类型，流结束可以用 `status.StreamEof()` 检查，不是流结束的 Status，就是流异常，注意流结束并不代表错误，建议把
  status 重置为成功状态。
- 从流中 `Read` 接口参数差异：gRPC 框架的 `Read` 没有提供超时时间，而 tRPC 框架的 `Read`
  提供了超时时间控制机制，用户可以传递以毫秒为单位的时间来控制读是否超时，当读超时之后，用户可以选择直接结束该流请求，或者继续读，下面是读超时之后直接结束流请求的例子，如果想超时之后想继续读，请使用 `status.GetFrameworkRetCode()==trpc::GrpcStatus::kGrpcDeadlineExceeded`
  作为判断依据。

```cpp
// gRPC 框架示例代码
// @file:route_guide_server.cc 

Status RecordRoute(ServerContext* context, ServerReader<Point>* reader,
                   RouteSummary* summary) override {
  Point point;
  int point_count = 0;
  int feature_count = 0;
  float distance = 0.0;
  Point previous;

  system_clock::time_point start_time = system_clock::now();
  while (reader->Read(&point)) {
    point_count++;
    if (!GetFeatureName(point, feature_list_).empty()) {
      feature_count++;
    }
    if (point_count != 1) {
      distance += GetDistance(previous, point);
    }
    previous = point;
  }
  system_clock::time_point end_time = system_clock::now();
  summary->set_point_count(point_count);
  summary->set_feature_count(feature_count);
  summary->set_distance(static_cast<long>(distance));
  auto secs =
      std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
  summary->set_elapsed_time(secs.count());

  return Status::OK;
}
```

```cpp
// tRPC 框架示例代码
// @file:stream_service.cc 

::trpc::Status RouteGuideImpl::RecordRoute(const ::trpc::ServerContextPtr& context,
                                           const ::trpc::stream::StreamReader<::routeguide::Point>& reader,
                                           ::routeguide::RouteSummary* summary) {
  ::trpc::Status status{};
  // ...
  for (;;) {
    // 以3s为超时来读
    status = reader.Read(&point, 3000);
    // Reads a Point successfully.
    if (status.OK()) {
      // ...
      continue;
    }

    // 服务端收到 EOF，代表客户端已经发完所有 Point
    if (status.StreamEof()) {
      // 收到 EOF 是正常的，重置状态为成功
      status = ::trpc::Status{0, 0, "OK"};
      break;
    }

    // 流出错.
    TRPC_FMT_ERROR("RecordRoute stream got error: {}", status.ToString());
    break;
  }

  // 读完客户端发送的所有 Point，服务端设置响应结果
  if (status.OK()) {
    uint64_t end_time_ms = ::trpc::time::GetMilliSeconds();
    summary->set_point_count(point_count);
    // ...
  }
  return status;
}
```

*双向流式*

差异点：

- gRPC框架传入了一个读写器，即能读也能写，而 tRPC 框架将读和写的类分开，除此之外，读写时候的接口差异和前面服务端流/客户端流描述一致。

注意到，例子当中用了个 `mutex` 互斥量，这主要是为了与 gRPC 框架提供的例子做对照，实际使用时候并不需要设置。

```cpp
// gRPC 框架示例代码
// @file:route_guide_server.cc 

Status RouteChat(ServerContext* context,
                 ServerReaderWriter<RouteNote, RouteNote>* stream) override {
  RouteNote note;
  while (stream->Read(&note)) {
    std::unique_lock<std::mutex> lock(mu_);
    for (const RouteNote& n : received_notes_) {
      if (n.location().latitude() == note.location().latitude() &&
          n.location().longitude() == note.location().longitude()) {
        stream->Write(n);
      }
    }
    received_notes_.push_back(note);
  }

  return Status::OK;
}
```

```cpp
// tRPC 框架 gRPC 流式服务示例代码
// @file:stream_service.cc 

::trpc::Status RouteGuideImpl::RouteChat(const ::trpc::ServerContextPtr& context,
                                         const ::trpc::stream::StreamReader<::routeguide::RouteNote>& reader,
                                         ::trpc::stream::StreamWriter<::routeguide::RouteNote>* writer) {
  ::trpc::Status status{};
  // ...
  for (;;) {
    // 以3s为超时来读
    status = reader.Read(&note, 3000);
    if (status.OK()) {
      std::unique_lock<std::mutex> lock(mu_);
      for (const ::routeguide::RouteNote& n : received_notes_) {
        if (n.location().latitude() == note.location().latitude() &&
            n.location().longitude() == note.location().longitude()) {
          // 如果读到的 RouteNote 之前已经收到了，就返还同样的 RouteNote 给客户端
          status = writer->Write(n);
          if (!status.OK()) {
            TRPC_FMT_ERROR("RouteChat write got error: {}", status.ToString());
            return status;
          }
        }
      }
      received_notes_.push_back(note);
      continue;
    }

    if (status.StreamEof()) {
      // EOF is ok status.
      status = ::trpc::Status{0, 0, "OK"};
      break;
    }

    TRPC_FMT_ERROR("RouteChat read got error: {}", status.ToString());
    break;
  }

  return status;
}
```

# FAQ

## 1 gRPC 流式服务是否支持 h2 (HTTP2 over SSL)？

暂未支持。tRPC 中的 gRPC 协议底层使用的 h2c，暂未支持 SSL（支持中）。
