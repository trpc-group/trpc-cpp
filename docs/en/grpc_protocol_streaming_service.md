[中文](../zh/grpc_protocol_streaming_service.md)

# Overview

Some services developed using the gRPC framework need to communicate with the tRPC-Cpp framework using the gRPC
streaming protocol.

This article introduces how to develop gRPC streaming services based on tRPC-Cpp. Developers can learn the following
contents:

* How to develop gRPC streaming services.
* FAQ.

# How to develop gRPC streaming services

## Quick start

### Experience a gRPC streaming service

Example: [grpc_stream](../../examples/features/grpc_stream)

Go to the main directory of the tRPC code repository and run the following command.

```shell
sh examples/features/grpc_stream/run.sh
```

The content of output from the client program is as follows:

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

### Feature support status

| Feature                            | Support Status  |
|------------------------------------|-----------------|
| Server streaming                   | Supported       |
| Client streaming                   | Supported       |
| Bidirectional streaming            | Supported       |
| Serialization/deserialization type | ProtoBuffers    |
| Compression                        | Planned support |

Limitations:

* Currently, only the fiber thread model (i.e., synchronous streaming service) is supported, and the merge and separate
  thread models are not yet supported.
* Currently available on the server side, but not yet available on the client side.

### Basic steps for developing gRPC streaming service

Example: [grpc_stream](../../examples/features/grpc_stream)

Reference to [tRPC Streaming Protocol Service Development Guide](./trpc_protocol_streaming_service.md)

The gRPC streaming protocol implementation in tRPC reuses the stream interface design of tRPC, and the corresponding
stream programming interface reuses the stream reader/writer interface of tRPC. The development process is also
consistent with that of tRPC streaming protocol services, except that the yaml configuration item is changed
from `protocol: trpc` to `protocol: grpc`.

```yaml
global:
  # fiber runtime is used.
  threadmodel:
    fiber:
      - instance_name: fiber_instance

server:
  service:
    - name: xx_service_name
      # grpc protocol is used. 
      protocol: grpc
      ...
```

### Example of streaming service on server side

In order to facilitate the use of the stream reader/writer interface provided by tRPC, the sample code rewrites the server streaming example provided by the gRPC framework for easy comparison and reference.

* [Example of streaming service in gRPC framework](https://github.com/grpc/grpc/blob/master/examples/cpp/route_guide/route_guide_server.cc)
* [Example of streaming service in tRPC framework](../../examples/features/grpc_stream/server/stream_service.cc)

#### Server streaming

* Example code of gRPC framework

  ```cpp
  // Example code of gRPC framework. 
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

* Example code of tRPC framework

  ```cpp
  // Example code of tRPC framework. 
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

Differences:

* The return values of the `Write` interface to the stream are different: the `Write` interface of the gRPC framework returns a bool indicating success or failure, while the `Write` interface of tRPC returns a `Status`. In addition to representing whether the writing was successful or not, it also includes the reason for the write failure (`status.ToString()` or `status.ErrorString()`).

#### Client streaming

* Example code of gRPC framework

  ```cpp
  // Example code of gRPC framework. 
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

* Example code of tRPC framework

  ```cpp
  // Example code of tRPC framework. 
  // @file:stream_service.cc 
  
  ::trpc::Status RouteGuideImpl::RecordRoute(const ::trpc::ServerContextPtr& context,
                                             const ::trpc::stream::StreamReader<::routeguide::Point>& reader,
                                             ::routeguide::RouteSummary* summary) {
    ::trpc::Status status{};
    // ...
    for (;;) {
      // Read timeout: 3s.
      status = reader.Read(&point, 3000);
      // Reads a Point successfully.
      if (status.OK()) {
        // ...
        continue;
      }
  
      // The server receives an EOF, indicating that the client has sent all Points.
      if (status.StreamEof()) {
        // Receiving an EOF is ok.
        status = ::trpc::Status{0, 0, "OK"};
        break;
      }
  
      // Error.
      TRPC_FMT_ERROR("RecordRoute stream got error: {}", status.ToString());
      break;
    }
  
    // After reading all the Points send by the client, the server sets the response result.
    if (status.OK()) {
      uint64_t end_time_ms = ::trpc::time::GetMilliSeconds();
      summary->set_point_count(point_count);
      // ...
    }
    return status;
  }
  ```

Differences:

* The return value of the `Read` interface from the stream is different: the `Read` interface of the gRPC framework
  returns a bool type, and the end of the stream (the client has sent all request data) cannot be distinguished from the
  stream exception. The `Read` interface of the tRPC framework returns a Status type, and the end of the stream can be
  checked using `status.StreamEof()`. If the Status is not the end of the stream, then it is a stream exception. Note
  that the end of the stream does not represent an error, and it is recommended to reset the status to a successful
  state.
* The parameter differences of the `Read` interface from the stream: the gRPC framework does not provide a timeout
  mechanism for `Read`, while the tRPC framework provides a timeout control mechanism for `Read`. Users can pass a time
  in milliseconds to control whether the read times out. When the read times out, the user can choose to end the stream
  request directly or continue reading. The following is an example of ending the stream request directly after a read
  timeout. If you want to continue reading after the timeout,
  use `status.GetFrameworkRetCode()==trpc::GrpcStatus::kGrpcDeadlineExceeded` as the judgment basis.

#### Bidirectional streaming

* Example code of gRPC framework

  ```cpp
  // Example code of gRPC framework. 
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

* Example code of tRPC framework

  ```cpp
  // Example code of tRPC framework. 
  // @file:stream_service.cc 
  
  ::trpc::Status RouteGuideImpl::RouteChat(const ::trpc::ServerContextPtr& context,
                                           const ::trpc::stream::StreamReader<::routeguide::RouteNote>& reader,
                                           ::trpc::stream::StreamWriter<::routeguide::RouteNote>* writer) {
    ::trpc::Status status{};
    // ...
    for (;;) {
      // Read timeout: 3s.
      status = reader.Read(&note, 3000);
      if (status.OK()) {
        std::unique_lock<std::mutex> lock(mu_);
        for (const ::routeguide::RouteNote& n : received_notes_) {
          if (n.location().latitude() == note.location().latitude() &&
              n.location().longitude() == note.location().longitude()) {
            // if the RouteNode that was read has already been received before, return the same RouteNode to the client.
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

Differences:

* The gRPC framework passes in a reader/writer that can both read and write, while the tRPC framework separates the
  reading and writing classes. In addition, the interface differences when reading and writing are consistent with the
  previous server streaming/client streaming descriptions.

Note that a `mutex` mutex is used in the example mainly for comparison with the example provided by the gRPC framework.
It is not necessary to set it when actually using it.

# FAQ

## Does gRPC support h2 (HTTP2 over SSL)?

It is not currently supported. The gRPC protocol used in tRPC uses h2c at the underlying level, and SSL is not currently
supported(but is currently being developed).
