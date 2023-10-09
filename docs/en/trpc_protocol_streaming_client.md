[中文](../zh/trpc_protocol_streaming_client.md)

[TOC]

# Accessing tRPC Streaming Protocol Services

**Topic: How to access tRPC streaming services based on tRPC-Cpp**

The streaming RPC of the tRPC protocol can be divided into three types:

* Client streaming RPC: The client sends one or more request messages, and the server sends one response message.
* Server streaming RPC: The client sends a request message, and the server sends one or more response messages.
* Bidirectional streaming RPC: The client sends one or more request messages, and the server sends one or more response
  messages.

tRPC streaming services support two thread models:

* The `fiber` thread model supports synchronous streaming RPC.
* The `merge` thread model supports asynchronous streaming RPC.

This article introduces how to access tRPC streaming services based on tRPC-Cpp. Developers can learn the following:

* How to use synchronous interfaces to access streaming RPC services.
* How to use asynchronous interfaces to access streaming RPC services.
* FAQ.

The way to develop tRPC streaming services can refer to [trpc_streaming_service.md](./trpc_protocol_streaming_service.md).

# How to use synchronous interfaces to access streaming RPC services

Example: [trpc_stream](../../examples/features/trpc_stream)

The synchronous streaming interface is introduced in trpc_streaming_service.md. Here is a brief list for easy reference.

```cpp
// Synchronous stream reader.
template <typename R>
class StreamReader {
 public:
  Status Read(R* msg, int timeout = -1) const;
  Status Finish();
  Status GetStatus();
};

// Synchronous stream writer.
template <typename W>
class StreamWriter {
 public:
  Status Write(const W& msg);
  Status WriteDone();
  Status Finish();
  Status GetStatus();
};

// Synchronous stream reader-writer.
template <typename W, typename R>
class StreamReaderWriter {
 public:
  Status Read(R* msg, int timeout = -1) const;
  Status Write(const W& msg);
  Status WriteDone();
  Status Finish();
  Status GetStatus();
};
```

Here we take [stream.proto](../../examples/features/trpc_stream/server/stream.proto) as an example to illustrate.

The tRPC stub code generation tool will generate the following code.

```cpp
class StreamGreeterServiceProxy : public ::trpc::RpcServiceProxy {
 public:
  // Client Streaming RPC
  virtual ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloRequest> ClientStreamSayHello(const ::trpc::ClientContextPtr& context, ::trpc::test::helloworld::HelloReply* response);
  // Server Streaming RPC
  virtual ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloReply> ServerStreamSayHello(const ::trpc::ClientContextPtr& context, const ::trpc::test::helloworld::HelloRequest& request);
  // Bidirectional Streaming RPC
  virtual ::trpc::stream::StreamReaderWriter<::trpc::test::helloworld::HelloRequest, ::trpc::test::helloworld::HelloReply> BidiStreamSayHello(const ::trpc::ClientContextPtr& context);
};
```

Example: [client.cc](../../examples/features/trpc_stream/client/client.cc)

Here is a brief example of how a client accesses a streaming RPC service using bidirectional streaming:

* The client sends a specified number of request messages to the server and notifies the server of the completion of the
  sending process.
* The client continuously reads response messages sent from the server until it reaches EOF.
* After successfully reading EOF, the RPC call ends, and the client waits for the final result of the RPC call.

```cpp
bool CallBidiStreamSayHello(const StreamGreeterServiceProxyPtr& proxy, int request_count) {
  auto context = ::trpc::MakeClientContext(proxy);
  // Open a stream.
  auto stream = proxy->BidiStreamSayHello(context);
  ::trpc::Status status{0, 0, "OK"};
  bool ok{true};
  do {
    // Check the stream is ok or not.
    if (!stream.GetStatus().OK()) {
      std::cout << "stream error:" << stream.GetStatus().ToString() << std::endl;
      ok = false;
      break;
    }

    // Send the $request_count messages to the server. 
    for (int i = 0; i < request_count; ++i) {
      std::stringstream request_msg;
      request_msg << "BidiStreamSayHello, " << (i + 1);
      ::trpc::test::helloworld::HelloRequest request;
      request.set_msg(request_msg.str());
      // Write the streaming message.
      status = stream.Write(request);
      if (status.OK()) {
        continue;
      }
      break;
    }

    if (!status.OK()) {
      std::cerr << "write error: " << status.ToString() << std::endl;
      ok = false;
      break;
    }

    // All streaming messages had been sent.
    status = stream.WriteDone();
    if (!status.OK()) {
      std::cerr << "write done error: " << status.ToString() << std::endl;
      ok = false;
      break;
    }

    ::trpc::test::helloworld::HelloReply reply;
    // Read the streaming messages until the EOF received.
    for (;;) {
      status = stream.Read(&reply, 2000);
      if (status.OK()) {
        std::stringstream reply_msg;
        reply_msg << "stream got reply:" << reply.msg();
        std::cout << reply_msg.str() << std::endl;
        continue;
      }
      if (status.StreamEof()) {
        std::cout << "got EOF" << std::endl;
        // Wait the final call result when EOF received.
        status = stream.Finish();
      }
      break;
    }

    if (status.OK()) {
      std::cout << "stream rpc succeed" << std::endl;
    } else {
      std::cerr << "stream rpc failed:" << status.ToString() << std::endl;
      ok = false;
    }
  } while (0);
  return ok;
}
```

# How to use asynchronous interfaces to access streaming RPC services

The asynchronous streaming interface is introduced in trpc_streaming_service.md. Here is a brief list for easy
reference.

```cpp
// Asynchronous stream reader.
template <typename R>
class AsyncReader final : public RefCounted<AsyncReader<R>> {
 public:
  Future<std::optional<R>> Read(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());  // 返回 std::optional<R> 为空表示 EOF
  Future<> Finish();
};

// Asynchronous stream writer.
template <typename W>
class AsyncWriter : public RefCounted<AsyncWriter<W>> {
 public:
  Future<> Write(const W& msg);
  Future<> Finish();
};

// Asynchronous stream reader-writer.
template <typename R, typename W>
class AsyncReaderWriter final : public RefCounted<AsyncReaderWriter<R, W>> {
 public:
  Future<std::optional<R>> Read(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());  // 返回 std::optional<R> 为空表示 EOF
  Future<> Write(const W& msg);
  Future<> Finish();
};
```

Here we take [stream.proto](../../examples/features/trpc_stream/server/stream.proto) as an example to illustrate.

The tRPC stub code generation tool will generate the following code.

```cpp
class AsyncStreamGreeterServiceProxy : public ::trpc::RpcServiceProxy {
 public:
  // Client Streaming RPC
  ::trpc::Future<std::pair<::trpc::stream::AsyncWriterPtr<::trpc::test::helloworld::HelloRequest>, ::trpc::Future<::trpc::test::helloworld::HelloReply>>> ClientStreamSayHello(const ::trpc::ClientContextPtr& context);
  // Server Streaming RPC
  ::trpc::Future<::trpc::stream::AsyncReaderPtr<::trpc::test::helloworld::HelloReply>> ServerStreamSayHello(const ::trpc::ClientContextPtr& context, ::trpc::test::helloworld::HelloRequest&& request);
  // Bidirectional Streaming RPC
  ::trpc::Future<::trpc::stream::AsyncReaderWriterPtr<::trpc::test::helloworld::HelloReply, ::trpc::test::helloworld::HelloRequest>> BidiStreamSayHello(const ::trpc::ClientContextPtr& context);
};
```

TODO: example code.

# FAQ

## 1 How to choose between synchronous and asynchronous streaming interfaces?

It depends on the thread model used. `fiber` can only use synchronous streaming interfaces, while `merge` can only use
asynchronous streaming interfaces.

## 2 Why is there no WriteDone in the asynchronous streaming interface?

In the asynchronous streaming interface, when the client calls the Finish interface of the reader/writer, it notifies
the other end that the data has been written (equivalent to WriteDone).

## 3 Can asynchronous streaming only run in the `merge` thread model?

Yes, other thread models are not yet supported.
In addition, the `merge` thread model currently only supports tRPC streaming protocol and HTTP protocol, and gRPC is not
yet supported.