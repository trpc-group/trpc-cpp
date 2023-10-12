[中文](../zh/tRPC_streaming_service.md)

# Overview

When using Unary RPC, the client sends a single request to the server and waits for a single response from the server.
This can be simply divided into three steps:

* The client sends a request message to the server and waits for the RPC to return.
* The server receives the request message, executes the business logic, and then sends a response message to the client.
* The client receives the RPC return value, processes the return value and response message.

Compared with Unary RPC, in streaming RPC, after the stream channel is established between the client and the server,
both sides can continuously send and receive messages, and the request and response messages flow like water. The client
can continuously send request messages and receive response messages; the server can continuously receive request
messages and send response messages.

This article introduces how to develop tRPC streaming services based on tRPC-Cpp. Developers can learn the following
contents:

* Features of streaming RPC.
* How to define streaming RPC services.
* How to develop synchronous streaming RPC services.
* How to develop asynchronous streaming RPC services.
* FAQ.

# Features of streaming RPC

## Scenarios suitable for streaming RPC

Consider the following business scenario: Every day at 6 a.m., a batch of data sets with millions of records needs to be
synchronized from A to B, and a series of operations (archiving, data analysis, profiling, logging, etc.) will be
performed during the synchronization. After the synchronization is completed, users will immediately query this batch of
data, which will be displayed to users in different report forms after various calculations, and some data will be
pushed to subscribing users.

Scenario characteristics:

* Large amount of data involved at one time.
* Requires a certain degree of real-time performance.

Unary RPC has the following limitations:

* Instantaneous pressure caused by large data packets.
* When receiving data packets, the response callback and business processing can only be performed after all data
  packets have been successfully received and verified (it is impossible to process on the server side while the client
  is still sending data).

Streaming RPC has advantages in the following scenarios:

* Large-scale data packets.
  For example, if a large file needs to be transferred, the client can read the file content in chunks and write it
  directly to the stream when using streaming RPC. The server can read the file chunks in the order that the client
  wrote them and then execute the business logic. If unary RPC is used, multiple RPC method calls are required, which
  may encounter problems such as packet fragmentation, packet reassembly, packet disorder, and business logic context
  switching.
* Real-time scenarios.
  For example, stock market trends and information feeds. After the server receives a message, it needs to push
  real-time messages to multiple clients. Streaming RPC can push the complete message list in one RPC call.

## Streaming Protocol and Thread Model

tRPC streaming services support two thread models:

* The `fiber` thread model supports synchronous streaming RPC.
* The `merge` thread model supports asynchronous streaming RPC.

# How to define streaming RPC services

## Defining streaming RPC methods in the `proto` file

The definition of a streaming RPC interface adds a keyword `stream` compared to a unary RPC interface definition, which
is used to indicate the type of the streaming RPC method.

```protobuf
// Client Streaming RPC
    rpc ClientStreamSayHello (stream Request) returns (Reply) {}
// Server Streaming RPC
    rpc ServerStreamSayHello (Request) returns (stream Reply) {}
// Bidirectional Streaming RPC
    rpc BidiStreamSayHello(stream Request) returns (stream Reply) {}
```

## Naming of RPC methods

* Only one type of RPC method can be defined with one unique name.

  For example, we cannot use `SayHello` to define client streaming, server streaming, and unary RPC methods at the same time. The following definition is incorrect, as the same method name is used to define three types of RPC methods:

  ```protobuf
  // !!! The following definition is incorrect !!!
  
  // Unary RPC
      rpc SayHello (Request) returns (Reply) {}
  // Client Streaming RPC
      rpc SayHello (stream Request) returns (Reply) {}
  // Server Streaming RPC
      rpc SayHello (Request) returns (stream Reply) {}
  ```

* Unary RPC and streaming RPC are placed in different services.

  Although the implementation does not restrict how users place unary RPC and streaming RPC, it is **`recommended`** to place unary RPC and streaming RPC in different services and not mix them together. This will reduce the difficulty of problem localization. For example, from the perspective of network packet transmission, multiple streams in streaming RPC share the same connection, and the protocol frames in streaming RPC are more complex than those in unary RPC. Mixing them together will increase the difficulty of problem diagnosis. The recommended approach is as follows:
  
  ```protobuf
  // Service of unary RPC
  service Greeter {
    rpc  SayHello (Request) returns (Reply) {}
  }
  
  // Service of streaming RPC
  service StreamGreeter {
    // Client Streaming RPC
    rpc  ClientStreamSayHello (stream Request) returns (Reply)  {}
    // Server Streaming RPC
    rpc  ServerStreamSayHello (Request) returns (stream Reply) {}
  }
  ```

# How to develop synchronous streaming RPC services

Note: The synchronous streaming interface needs to run in a `fiber` coroutine environment.

## Experience a synchronous streaming RPC service

Example: [trpc_stream](../../examples/features/trpc_stream)

Go to the main directory of the tRPC code repository and run the following command.

```shell
sh examples/features/trpc_stream/run.sh
```

The content of the output from the client program is as follows:

``` text
name: Streaming RPC, ClientStreamSayHello1, ok: 1
name: Streaming RPC, ClientStreamSayHello2, ok: 1
name: Streaming RPC, ClientStreamSayHello3, ok: 1
name: Streaming RPC, ClientStreamSayHello4, ok: 1
name: Streaming RPC, ClientStreamSayHello5, ok: 1
name: Streaming RPC, ClientStreamSayHello6, ok: 1
name: Streaming RPC, ClientStreamSayHello7, ok: 1
name: Streaming RPC, ClientStreamSayHello8, ok: 1
final result of streaming RPC calling: 1
```

## Synchronous streaming interfaces

### Stream reader-writer

Explanation: The `R` and `W` in the interface are template parameters, representing the stream message object to be read
and the stream message object to be written, respectively.

| Class/Object       | Interface Name                        | Functionality                             | Parameters                                                                                                                                                                                                                                                                            | Return Value |
|--------------------|---------------------------------------|-------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------|
| StreamReader       | Status Read(R* msg, int timeout = -1) | Read a message from the stream            | msg: A pointer to the response message object. After reading the message, the object pointed to by the pointer will be updated.<br>timeout: Wait timeout. When no message is ready, it will block and wait. <br>Values: -1: Wait indefinitely, >=0: Wait timeout, unit: milliseconds. | Status       |
| StreamReader       | Status Finish(void)                   | Wait for the RPC to finish                | void                                                                                                                                                                                                                                                                                  | Status       |
| StreamReader       | Status GetStatus(void)                | Get the status of the stream              | void                                                                                                                                                                                                                                                                                  | Status       |
| StreamWriter       | Status Write(const W& msg)            | Write a message to the stream             | msg: A reference to the message to be written.                                                                                                                                                                                                                                        | Status       |
| StreamWriter       | Status WriteDone(void)                | Notify the other end that writing is done | void                                                                                                                                                                                                                                                                                  | Status       |
| StreamWriter       | Status Finish(void)                   | Wait for the RPC to finish                | void                                                                                                                                                                                                                                                                                  | Status       |
| StreamWriter       | Status GetStatus(void)                | Get the status of the stream              | void                                                                                                                                                                                                                                                                                  | Status       |
| StreamReaderWriter | Status Read(R* msg, int timeout = -1) | Read a message from the stream            | msg: A pointer to the response message object. After reading the message, the object pointed to by the pointer will be updated.<br>timeout: Wait timeout. When no message is ready, it will block and wait. <br>Values: -1: Wait indefinitely, >=0: Wait timeout, unit: milliseconds. | Status       |
| StreamReaderWriter | Status Write(const W& msg)            | Write a message to the stream             | msg: A reference to the message to be written.                                                                                                                                                                                                                                        | Status       |
| StreamReaderWriter | Status WriteDone(void)                | Notify the other end that writing is done | void                                                                                                                                                                                                                                                                                  | Status       |
| StreamReaderWriter | Status Finish(void)                   | Wait for the RPC to finish                | void                                                                                                                                                                                                                                                                                  | Status       |
| StreamReaderWriter | Status GetStatus(void)                | Get the status of the stream              | void                                                                                                                                                                                                                                                                                  | Status       |

### Return value of stream interface

The stream operation interface returns `Status`, which includes framework error codes, RPC method return codes, error
messages, etc. The following interfaces are commonly used in stream programming:

| Class/Object | Interface Name           | Functionality                                             | Parameters | Return Value                                            |
|--------------|--------------------------|-----------------------------------------------------------|------------|---------------------------------------------------------|
| Status       | bool Status::OK()        | Whether the stream interface call is correct              | void       | bool, true: correct, false: error                       |
| Status       | bool Status::StreamEof() | Whether the data read from the stream has encountered EOF | void       | bool, true: encountered EOF, false: not encountered EOF |

### Error codes for stream interface

Common error codes for the server:

| Error Code                                    | Meaning                                      |
|-----------------------------------------------|----------------------------------------------|
| TRPC_STREAM_SERVER_NETWORK_ERR = 201          | Network error in server stream               |
| TRPC_STREAM_SERVER_MSG_EXCEED_LIMIT_ERR = 211 | Stream message is too large                  |
| TRPC_STREAM_SERVER_ENCODE_ERR = 221           | Encoding error in server stream              |
| TRPC_STREAM_SERVER_DECODE_ERR = 222           | Encoding and decoding error in server stream |
| TRPC_STREAM_SERVER_READ_TIMEOUT_ERR = 254     | Read timeout in server stream                |

Common error codes for the client:

| Error Code                                    | Meaning                                      |
|-----------------------------------------------|----------------------------------------------|
| TRPC_STREAM_CLIENT_NETWORK_ERR = 301          | Network error in client stream               |
| TRPC_STREAM_CLIENT_MSG_EXCEED_LIMIT_ERR = 311 | Stream message is too large                  |
| TRPC_STREAM_CLIENT_ENCODE_ERR = 321           | Encoding error in client stream              |
| TRPC_STREAM_CLIENT_DECODE_ERR = 322           | Encoding and decoding error in client stream |
| TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR = 354     | Read timeout in client stream                |

## Setting up a stream RPC service

Before setting up a stream service, let's review the steps for setting up a single RPC service, which are as follows:

* Define the RPC interface.
* Generate RPC service code.
* Implement the business logic of the RPC service.
* Call the RPC service on the client.

The steps for setting up a tRPC stream RPC service are similar to those for setting up a single RPC service. Below, we will explain how to set up a stream RPC service using examples as follows:

* Client-Server scenario scenario
* Client-Router-Server scenario (abbreviation for forward service).

### Client-Server scenario

We will use the SayHello as an example to demonstrate the process of setting up a stream RPC service in the
Client-Server scenario. There are two modules:

* StreamClient: The stream RPC client, which initiates the stream RPC call.
* StreamServer: The stream RPC server, which provides the stream RPC service.

#### Defining streaming RPC methods

```protobuf
// @file: examples/features/trpc_stream/server/stream.proto
syntax = "proto3";

package trpc.test.helloworld;

service Greeter {
  // Client streaming.
  rpc ClientStreamSayHello (stream HelloRequest) returns (HelloReply) {}

  // Server streaming.
  rpc ServerStreamSayHello (HelloRequest) returns (stream HelloReply) {}

  // Bidirectional streaming.
  rpc BidiStreamSayHello (stream HelloRequest) returns (stream HelloReply) {}
}

message HelloRequest {
  string msg = 1;
}

message HelloReply {
  string msg = 1;
}
```

#### Generating stream RPC service code

The code generation tool provided by tRPC will automatically generate the stream RPC service code. Take a look at the
code snippet to get a preliminary impression of stream RPC. The following is a code snippet from the StreamServer
module.

```cpp
// @file: bazel-bin/examples/features/trpc_stream/server/stream.trpc.pb.h
// ...
class StreamGreeter: public ::trpc::RpcServiceImpl {
public:
...
  virtual ::trpc::Status ClientStreamSayHello(const ::trpc::ServerContextPtr& context, const ::trpc::stream::StreamReader<trpc::test::helloworld::HelloRequest>& reader, ::trpc::test::helloworld::HelloReply* response);
  virtual ::trpc::Status ServerStreamSayHello(const ::trpc::ServerContextPtr& context, const ::trpc::test::helloworld::HelloRequest& request, ::trpc::stream::StreamWriter<trpc::test::helloworld::HelloReply>* writer);
  virtual ::trpc::Status BidiStreamSayHello(const ::trpc::ServerContextPtr& context, const ::trpc::stream::StreamReader<trpc::test::helloworld::HelloRequest>& reader, ::trpc::stream::StreamWriter<trpc::test::helloworld::HelloReply>* writer);
};
// ...
```

#### Implementing the stream RPC service

Example: [stream_service.cc](../../examples/features/trpc_stream/server/stream_server.cc)

* **Client Streaming**

  The key points of the ClientStreamSayHello method in the StreamServer module are as follows:

  * Continuously read the request messages from the stream. If an error occurs, stop reading and return an error status to
  the client.
  * After reading the EOF from the stream, set the response message and the RPC call result status, which will be returned
  to the client.

  ```cpp
  // Client streaming RPC
  ::trpc::Status StreamGreeterServiceImpl::ClientStreamSayHello(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
      ::trpc::test::helloworld::HelloReply* reply) {
    ::trpc::Status status{};
    uint32_t request_counter{0};
    uint32_t request_bytes{0};
    for (;;) {
      ::trpc::test::helloworld::HelloRequest request{};
      // Read streaming request message from the client.
      status = reader.Read(&request, 3000);
      if (status.OK()) {
        ++request_counter;
        request_bytes += request.msg().size();
        continue;
      }
      // EOF of stream. 
      if (status.StreamEof()) {
        std::stringstream reply_msg;
        reply_msg << "server got EOF, reply to client, server got request"
                  << ", count:" << request_counter << ", received bytes:" << request_bytes;
        reply->set_msg(reply_msg.str());
        status = ::trpc::Status{0, 0, "OK"};
        break;
      }
      break;
    }
    return status;
  }
  ```

* **Server Streaming**

  The key points of the ServerStreamSayHello method in the StreamServer module are as follows:

  * Write the request message sent by the client back to the client in a stream manner.
  * Return the RPC call result to the client.

  ```cpp
  // Server streaming RPC
  ::trpc::Status StreamGreeterServiceImpl::ServerStreamSayHello(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::test::helloworld::HelloRequest& request, 
      ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer) {
    ::trpc::Status status{};
    // A simple case: try to send 10 responses.
    int request_count = 10;
    for (int i = 0; i < request_count; ++i) {
      std::stringstream reply_msg;
      ::trpc::test::helloworld::HelloReply reply{};
      reply.set_msg(reply_msg.str());
      // Write the streaming message.
      status = writer->Write(reply);
      if (status.OK()) {
        continue;
      }
      break;
    }
    return status;
  }
  ```

* **Bidirectional Streaming**

  The key points of the BidiStreamSayHello method in the StreamServer module are as follows:

  * Continuously read the request messages from the client side and immediately send them back to the client.
  * If EOF is read, an additional request message containing statistical information is sent to the client.
  * Return the RPC call result to the client.

  ```cpp
  // Bidirectional streaming RPC.
  ::trpc::Status StreamGreeterServiceImpl::BidiStreamSayHello(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
      ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer) {
    std::vector<std::string> msg_list{};
    ::trpc::Status status{};
    uint32_t request_counter{0};
    uint32_t request_bytes{0};
    for (;;) {
      ::trpc::test::helloworld::HelloRequest request{};
      // Read the streaming request message from the client.
      status = reader.Read(&request, 3000);
      if (status.OK()) {
        ++request_counter;
        request_bytes += request.msg().size();
        std::stringstream reply_msg;
        reply_msg << " reply:" << request_counter << ", received bytes:" << request_bytes;
        ::trpc::test::helloworld::HelloReply reply;
        reply.set_msg(reply_msg.str());
        // Write the streaming message to client.
        writer->Write(reply);
        continue;
      }
      // EOF of stream.
      if (status.StreamEof()) {
        std::stringstream reply_msg;
        reply_msg << "server got EOF, reply to client, server got request"
                  << ", count:" << request_counter << ", received bytes:" << request_bytes;
        ::trpc::test::helloworld::HelloReply reply;
        reply.set_msg(reply_msg.str());
        status = writer->Write(reply);
      }
      break;
    }
    return status;
  }
  ```

### Client-Router-Server scenario

We will use the SayHello as an example to demonstrate the process of setting up a stream RPC service in the
Client-Router-Server scenario. There are three modules:

* StreamClient: The stream RPC client, which initiates the stream RPC call.
* StreamServer: The stream RPC server, which provides the stream RPC service (the same as the StreamServer in the
  Client-Server scenario).
* StreamForwardServer: Receives requests from StreamClient and forwards them to StreamServer, then sends the response
  from StreamServer back to StreamClient.

#### Defining streaming RPC methods for forward service

```protobuf
// @file: examples/features/trpc_stream/forward/proxy/stream_forward.proto
syntax = "proto3";

package trpc.test.helloworld;

import "examples/features/trpc_stream/server/stream.proto";

service StreamForward {
  // Client streaming
  rpc ClientStreamSayHello (stream HelloRequest) returns (HelloReply) {}

  // Server streaming
  rpc ServerStreamSayHello (HelloRequest) returns (stream HelloReply) {}

  // Bidirectional streaming
  rpc BidiStreamSayHello (stream HelloRequest) returns (stream HelloReply) {}
}
```

#### Generating stream RPC service code for forward service

The following is a code snippet from the StreamServer module.

```cpp
// @file: bazel-bin/examples/features/trpc_stream/forward/proxy/stream_forward.trpc.pb.h
// ...
class StreamForward: public ::trpc::RpcServiceImpl {
public:
// ...
  virtual ::trpc::Status ClientStreamSayHello(const ::trpc::ServerContextPtr& context, const ::trpc::stream::StreamReader<trpc::test::helloworld::HelloRequest>& reader, ::trpc::test::helloworld::HelloReply* response);
  virtual ::trpc::Status ServerStreamSayHello(const ::trpc::ServerContextPtr& context, const ::trpc::test::helloworld::HelloRequest& request, ::trpc::stream::StreamWriter<trpc::test::helloworld::HelloReply>* writer);
  virtual ::trpc::Status BidiStreamSayHello(const ::trpc::ServerContextPtr& context, const ::trpc::stream::StreamReader<trpc::test::helloworld::HelloRequest>& reader, ::trpc::stream::StreamWriter<trpc::test::helloworld::HelloReply>* writer);
};
// ...
```

#### Implementing the stream RPC service for forward service

Example: [stream_forward_service.cc](../../examples/features/trpc_stream_forward/proxy/stream_forward_service.cc)

* **Client streaming**

  The key points of the ClientStreamSayHello method in the StreamForwardServer module are as follows:

  * Call the ClientStreamSayHello method of StreamServer to obtain the client stream reader.
  * Continuously read the request messages from the client and send them to StreamServer. If an error occurs during the
  process, stop reading/writing.
  * If EOF is read from the client stream, wait for StreamServer to return the RPC method call result and return it to the
  client.

  ```cpp
  // Client streaming RPC.
  ::trpc::Status StreamForwardImpl::ClientStreamSayHello(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
      ::trpc::test::helloworld::HelloReply* reply) {
    ::trpc::Status status{};
    const StreamGreeterServiceProxyPtr& stream_greeter_proxy = GetStreamGreeterServiceProxyPtr();
    auto client_context = ::trpc::MakeClientContext(context, stream_greeter_proxy);
    do {
      // Open a stream of StreamGreeter. 
      auto stream = stream_greeter_proxy->ClientStreamSayHello(client_context, reply);
      status = stream.GetStatus();
      if (!status.OK()) {
        break;
      }
  
      for (;;) {
        ::trpc::test::helloworld::HelloRequest request;
        // Read the streaming request message from the client, then try to send to StreamServer.
        status = reader.Read(&request);
        if (status.OK()) {
          status = stream.Write(request);
          if (status.OK()) {
            continue;
          }
        }
        // EOF of stream. 
        if (status.StreamEof()) {
          status = stream.WriteDone();
          if (status.OK()) {
            status = stream.Finish();
          }
        }
        break;
      }
    } while (0);
    TRPC_FMT_INFO("final status: {}", status.ToString());
    return status;
  }
  ```

* **Server streaming**

  The key points of the ServerStreamSayHello method in the StreamRouter module are as follows:

  * Call the ServerStreamSayHello method of the StreamServer module to obtain the stream reader.
  * Continuously read the response messages from StreamServer and send them to the client. If an error occurs during the
  process, stop reading/writing.
  * If EOF is read from the stream, wait for StreamServer to return the RPC method call result and return it to the
  client.

  ```cpp
  // Server streaming RPC.
  ::trpc::Status StreamForwardImpl::ServerStreamSayHello(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::test::helloworld::HelloRequest& request,  // NO LINT
      ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer) {
    ::trpc::Status status{};
    const StreamGreeterServiceProxyPtr& stream_greeter_proxy = GetStreamGreeterServiceProxyPtr();
    auto client_context = ::trpc::MakeClientContext(context, stream_greeter_proxy);
    do {
      // Open a stream of StreamGreeter. 
      auto stream = stream_greeter_proxy->ServerStreamSayHello(client_context, request);
      status = stream.GetStatus();
      if (!status.OK()) {
        break;
      }
  
      for (;;) {
        ::trpc::test::helloworld::HelloReply reply;
       // Read the streaming response message from StreamServer, then try to send to the client.
        status = stream.Read(&reply);
        if (status.OK()) {
          status = writer->Write(reply);
          if (status.OK()) {
            continue;
          }
        }
        // EOF of stream. 
        if (status.StreamEof()) {
          status = stream.Finish();
        }
        break;
      }
    } while (0);
    TRPC_FMT_INFO("final status: {}", status.ToString());
    return status;
  }
  ```

* **Bidirectional streaming**

  The key points of the BidiStreamSayHello method in the StreamForwardServer module are as follows:

  * Call the BidiStreamSayHello method of the StreamServer module to obtain the stream reader/writer.
  * Continuously read the request messages from the client and send them to StreamServer. If an error occurs during the
  process, stop reading/writing.
  * If EOF is read from the stream, notify StreamServer that the request writing is finished.
  * Continuously read the response messages from StreamServer and send them to the client.
  * If EOF is read from the stream, wait for the RPC call result and return it to the client.

  ```cpp
  // Bidirectional streaming RPC.
  ::trpc::Status StreamForwardImpl::BidiStreamSayHello(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
      ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer) {
    ::trpc::Status status{};
    const StreamGreeterServiceProxyPtr& stream_greeter_proxy = GetStreamGreeterServiceProxyPtr();
    auto client_context = ::trpc::MakeClientContext(context, stream_greeter_proxy);
    do {
      // Open a stream of StreamGreeter.
      auto stream = stream_greeter_proxy->BidiStreamSayHello(client_context);
      status = stream.GetStatus();
      if (!status.OK()) {
        break;
      }
  
      for (;;) {
        ::trpc::test::helloworld::HelloRequest request;
        // Read the streaming request message from the client, then try to send to StreamServer.
        status = reader.Read(&request);
        if (status.OK()) {
          status = stream.Write(request);
          if (status.OK()) {
            continue;
          }
        }
        // EOF of stream. 
        if (status.StreamEof()) {
          status = stream.WriteDone();
          if (!status.OK()) {
            break;
          }
          for (;;) {
            ::trpc::test::helloworld::HelloReply reply;
            // Read the streaming response message from StreamServer, then try to send to the client.
            status = stream.Read(&reply);
            if (status.OK()) {
              status = writer->Write(reply);
              if (status.OK()) {
                continue;
              }
            }
            if (status.StreamEof()) {
              status = stream.Finish();
            }
            break;
          }
        }
        break;
      }
    } while (0);
    TRPC_FMT_INFO("final status: {}", status.ToString());
    return status;
  }
  ```

# How to develop asynchronous streaming RPC services

Note: The asynchronous streaming interface needs to run in a `merge` thread model.

## Experience a asynchronous streaming RPC service

Example: [trpc_async_stream](../../examples/features/trpc_async_stream)

Go to the main directory of the tRPC code repository and run the following command.

```shell
sh examples/features/trpc_async_stream/run.sh
```

The content of the output from the client program is as follows:

``` text
name: Async Streaming RPC, ClientStreamSayHello1, ok: 1
name: Async Streaming RPC, ClientStreamSayHello2, ok: 1
name: Async Streaming RPC, ClientStreamSayHello3, ok: 1
name: Async Streaming RPC, ClientStreamSayHello4, ok: 1
name: Async Streaming RPC, ClientStreamSayHello5, ok: 1
name: Async Streaming RPC, ClientStreamSayHello6, ok: 1
name: Async Streaming RPC, ClientStreamSayHello7, ok: 1
name: Async Streaming RPC, ClientStreamSayHello8, ok: 1
final result of streaming RPC calling: 1
```

## Asynchronous streaming interfaces

In the `merge` thread model, the basic development process and specifications for stream RPC are the same as those in
the `fiber` thread mode. The difference is that the interfaces in the `merge` mode are all asynchronous `Future/Promise`
style.
For example, in the stream.proto file mentioned above, the generated stub code is:

```cpp
// New server stub code (compared to synchronous stream server stub code)
class AsyncStreamGreeter : public RpcServiceImpl {
 public:
  AsyncGreeter();
  // Unary RPC.
  virtual Future<HelloReply> SayHello(const ServerContextPtr& context, const HelloRequest* request);
  // Client Streaming RPC.
  virtual Future<HelloReply> ClientStreamSayHello(const ServerContextPtr& context, const stream::AsyncReaderPtr<HelloRequest>& reader);
  // Server Streaming RPC.
  virtual Future<> ServerStreamSayHello(const ServerContextPtr& context, HelloRequest&& request, const stream::AsyncWriterPtr<HelloReply>& writer);
  // Bidi Streaming RPC.
  virtual Future<> BidiStreamSayHello(const ServerContextPtr& context, const stream::AsyncReaderWriterPtr<HelloRequest, HelloReply>& rw);
};
```

### Asynchronous stream reader-writer

Asynchronous stream RPC adds asynchronous stream reader and stream writer `AsyncReader, AsyncWriter`.

Explanation: The `R` and `W` in the interface are template parameters, representing the stream message object to be read
and the stream message object to be written, respectively.

| Class/Object      | Interface Name                                                         | Function                      | Parameters                                                      | Return Value                                                                                    |
|-------------------|------------------------------------------------------------------------|-------------------------------|-----------------------------------------------------------------|-------------------------------------------------------------------------------------------------|
| AsyncReader       | Future&lt;std::optional&lt;R>> Read(std::chrono::milliseconds timeout) | Read messages from the stream | timeout: waiting timeout in milliseconds, default is no timeout | Future&lt;std::optional&lt;R>>, std::optional&lt;R> is empty to indicate EOF from the other end |
| AsyncReader       | Future<> Finish()                                                      | End the RPC call              | void                                                            | Future<>                                                                                        |
| AsyncWriter       | Future<> Write(const W& msg)                                           | Write messages to the stream  | msg: the message to be written to the stream                    | Future<>                                                                                        |
| AsyncWriter       | Future<> Finish()                                                      | End the RPC call              | void                                                            | Future<>                                                                                        |
| AsyncReaderWriter | Future&lt;std::optional&lt;R>> Read(std::chrono::milliseconds timeout) | Read messages from the stream | timeout: waiting timeout in milliseconds, default is no timeout | Future&lt;std::optional&lt;R>>, std::optional&lt;R> is empty to indicate EOF from the other end |
| AsyncReaderWriter | Future<> Write(const W& msg)                                           | Write messages to the stream  | msg: the message to be written to the stream                    | Future<>                                                                                        |
| AsyncReaderWriter | Future<> Finish()                                                      | End the RPC call              | void                                                            | Future<>                                                                                        |

As can be seen, the interfaces of the asynchronous stream reader and writer are more concise and their semantics are
clearer. We no longer need to worry about the internal state of the stream.
According to the design requirements, the asynchronous stream uses the following specifications:

* The asynchronous reader (AsyncReader) of the same stream must be called serially, that is, the next Read can only be
  called after the previous Read is ready (Ready/Failed).
* The asynchronous writer (AsyncWriter) of the same stream must be called serially, that is, the next Write can only be
  called after the previous Write is ready (Ready/Failed).
* The asynchronous reader and writer of the same stream are bidirectional operations and can be called concurrently.
* The Finish semantics of each asynchronous reader and writer are exactly the same. Only one call can be made to the
  same stream (independently counted for the client and server). Since Finish is also considered a write operation, it
  needs to be called serially with Write.
* Stream establishment: When the user obtains an asynchronous reader or writer of a stream, it means that the stream has
  been negotiated and established (tRPC will automatically open the stream).
* Stream closing:
  * For the client, calling Finish will actively close the stream, and it will be ready after both parties agree to
      close (currently only Ready, even if the stream has an error, it will not be Failed). We can choose to call
      it (recommended for the client to call it actively).
  * For the server, tRPC will automatically close the stream after the Future returned by the user service function (
      such as the user-implemented AsyncStreamGreeter service function) is ready; calling Finish will not initiate any
      operations, but will only be ready after both parties agree to close (currently only Ready, even if the stream has
      an error, it will not be Failed). We can choose to call it.
* The current stream RPC interfaces do not support the PB Arena capability.

### Future utility interfaces

```cpp
// The following utility functions are all templates, and template parameters are not shown here, more details: trpc/future/future_utility.h.

Future<> DoUntil(Stop&& stop, Func&& func);
Future<> DoUntil(Func&& func);
Future<> DoWhile(Condition&& cond, Func&& func);
Future<> Repeat(Func&& func);
Future<> DoForEach(Iterator&& begin, Iterator&& end, Func&& func);
Future<> DoForEach(Container& container, Func&& func);
Future<> DoFor(std::size_t n, Func&& func);

R DoWith(T &&value, Func &&func);
Future<std::tuple<FutsType...>> WhenAll(FutsType &&... futs);
Future<std::vector<FutureType>> WhenAll(InputIterator first, InputIterator last);
Future<size_t, FutureValueType> WhenAny(InputIterator first, InputIterator last);
Future<size_t, FutureValueType> WhenAnyWithoutException(InputIterator first, InputIterator last);
```

### Implementing the stream RPC service by asynchronous mode

Example: [stream_service.cc](../../examples/features/trpc_async_stream/server/stream_service.cc)

* **Client Streaming**

  ```cpp
  // Client streaming RPC.
  ::trpc::Future<::trpc::test::helloworld::HelloReply> AsyncStreamGreeterServiceImpl::ClientStreamSayHello(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::stream::AsyncReaderPtr<::trpc::test::helloworld::HelloRequest>& reader) {
    struct State {
      std::size_t request_counter{0};
      std::size_t request_bytes{0};
    };
    auto state = std::make_shared<State>();
  
    return ::trpc::DoUntil([state, reader]() {
             // Loop to read stream messages in order
             return reader->Read(3000ms).Then(
                 [state, reader](std::optional<::trpc::test::helloworld::HelloRequest>&& req) {
                   if (req) {
                     ++state->request_counter;
                     state->request_bytes += req.value().msg().size();
                     // Loop: continue to read the next streaming message.
                     return ::trpc::MakeReadyFuture<bool>(true);
                   } else {
                     // EOF of stream.
                     // Loop: finish reading. 
                     return ::trpc::MakeReadyFuture<bool>(false);
                   }
                 });
           })
        .Then([state]() {
          auto msg =
              ::trpc::util::FormatString("get {} requests, {} bytes", state->request_counter, state->request_bytes);
          ::trpc::test::helloworld::HelloReply reply;
          reply.set_msg(std::move(msg));
          return ::trpc::MakeReadyFuture<::trpc::test::helloworld::HelloReply>(std::move(reply));
        });
  }
  ```

* **Server streaming**

  ```cpp
  // Server streaming RPC.
  ::trpc::Future<> AsyncStreamGreeterServiceImpl::ServerStreamSayHello(
      const ::trpc::ServerContextPtr& context, ::trpc::test::helloworld::HelloRequest&& request,
      const ::trpc::stream::AsyncWriterPtr<::trpc::test::helloworld::HelloReply>& writer) {
    // Try to send 10 streaming messages to the client.
    return ::trpc::DoFor(10,
                         [writer, request = std::move(request)](std::size_t i) {
                           auto msg = ::trpc::util::FormatString("{}#{}", request.msg(), i + 1);
                           ::trpc::test::helloworld::HelloReply reply;
                           reply.set_msg(std::move(msg));
                           return writer->Write(std::move(reply));
                         })
        .Then([]() {
          // Trigger a read timeout(>3000ms) for client to show its usage
          return ::trpc::AsyncTimer(false).After(3050 /*ms*/);
        });
  }
  ```

* **Bidirectional streaming**

  ```cpp
  // Bidirectional streaming RPC.
  ::trpc::Future<> AsyncStreamGreeterServiceImpl::BidiStreamSayHello(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::stream::AsyncReaderWriterPtr<::trpc::test::helloworld::HelloRequest,
                                                 ::trpc::test::helloworld::HelloReply>& rw) {
    struct State {
      std::size_t request_counter{0};
      std::size_t request_bytes{0};
    };
    auto state = std::make_shared<State>();
  
    return ::trpc::DoUntil([state, rw]() {
      // Loop to read stream messages in order, then try to reply it to the client.
      return rw->Read(3000ms).Then([state, rw](std::optional<::trpc::test::helloworld::HelloRequest>&& req) {
        if (req) {
          ++state->request_counter;
          state->request_bytes += req.value().msg().size();
          auto msg =
              ::trpc::util::FormatString("get {} requests, {} bytes", state->request_counter, state->request_bytes);
          ::trpc::test::helloworld::HelloReply reply;
          reply.set_msg(std::move(msg));
          return rw->Write(std::move(reply)).Then([]() { return ::trpc::MakeReadyFuture<bool>(true); });
        } else {
          // EOF of stream.
          // Loop: finish reading.
          return ::trpc::MakeReadyFuture<bool>(false);
        }
      });
    });
  }
  ```

# FAQ

## What is the difference between tRPC streaming and gRPC streaming?

tRPC streaming is an RPC protocol based on the TCP protocol, while gRPC is a universal 7-layer protocol based on
HTTP/2.0.
In terms of implementation complexity, HTTP/2.0 is more complex than tRPC, and as a standard protocol, it requires more
considerations and adherence to details.
In terms of functionality, both can perform streaming transmission, and the server and client can interactively respond.
tRPC is more based on RPC considerations.

## Why is there no Finish/WriteDone interface called in the synchronous streaming interface on the server side?

The server can also call Finish/WriteDone, but it does not execute anything. How can the server stop receiving or
sending?
If the server wants to stop receiving/sending, it can simply return, which represents the end of the stream.

## Why is there no WriteDone in the asynchronous streaming interface?

In the asynchronous streaming interface, when the client calls the Finish interface of the reader/writer, it notifies
the other end that the data has been written (equivalent to WriteDone).

## Can asynchronous streaming only run in the `merge` thread model?

Yes, other thread models are not yet supported.
