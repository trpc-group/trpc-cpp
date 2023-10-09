[中文](../zh/attachment.md)

[TOC]

# Overview

For certain use cases, the data transmitted in the business includes large file data, such as models or multimedia transcoding data. If this type of data is defined as a regular field in the structure defined by the IDL (e.g., using pb's string or bytes), packing it in the unary body of the protocol  for transmission will incur significant overhead in terms of serialization, deserialization, and memory copying, thereby affecting the request latency.

To reduce the overhead caused by transmitting large data files, the tRPC framework supports the ability to include attachments when transmitting regular RPC requests or responses. The business data is divided into two parts: one part consists of the request parameters and response results defined in the IDL (typically smaller-sized data that undergoes serialization and deserialization), and the other part consists of binary data attachments set by the business. The framework directly includes these attachments in the network packet for transmission at the sender, while the receiver directly retrieves this data from the network packet without any processing, thereby reducing the overhead of serialization, deserialization, and memory copying.

# Usage

For detailed usage examples, please refer to the [Attachment example](../../examples/features/trpc_attachment/).

## Client

On the client side, users can set request attachments and retrieve response attachments from the server using the `ClientContext`.
```cpp
class ClientContext {
...
 public:
  /// @brief Set request attachment
  void SetRequestAttachment(NoncontiguousBuffer&& attachment);
  
  /// @brief Get response attachment
  const NoncontiguousBuffer& GetResponseAttachment();
...
}
```

## Server

On the server side, users can retrieve request attachments and set response attachments using the `ServerContext`.
```cpp
class ServerContext {
...
 public:  
  /// @brief Get request attachment data
  const NoncontiguousBuffer& GetRequestAttachment();

  /// @brief Set response attachment data
  void SetResponseAttachment(NoncontiguousBuffer&& attachment);
...
}
```
