[English](../en/attachment.md)

# 前言

对于一些应用场景，业务传递的数据中包含大文件数据（如模型、多媒体转码数据等），如果将这种数据作为IDL定义的结构中一个普通字段（如pb的string、bytes），打包在协议的Unary包体中进行传输，将会产生较大的序列化、反序列化、内存拷贝的开销，影响请求的耗时。

为了减少传递大数据文件带来的开销，tRPC框架支持在传递正常的RPC请求/响应时，附带传输attachment（附件）的能力。业务的数据分将为两部分：一部分是IDL中定义的请求参数和响应结果（通常为大小值较小的数据，将会进行序列化/反序列化），另一部分为业务额外设置的二进制数据附件。其中的附件，框架在发送端直接将其放入网络包中进行发送，而接收端直接则从网络包中取出这部分数据，不进行任何处理，从而减少序列化、反序列化、内存拷贝的开销。

# 使用方式

详细的使用例子可以参考：[Attachment example](../../examples/features/trpc_attachment/)

## 客户端

在客户端，用户可以通过`ClientContext`来设置请求的附件，以及获取从服务端响应返回的附件。

```cpp
class ClientContext {
...
 public:
  /// @brief Set request attachment
  void SetRequestAttachment(NoncontiguousBuffer&& attachment);
  
  /// @brief Get response attachment
  const NoncontiguousBuffer& GetResponseAttachment();
// ...
};
```

## 服务端

在服务端，用户可以通过`ServerContext`来获取请求的附件，和设置回包响应的附件。

```cpp
class ServerContext {
...
 public:  
  /// @brief Get request attachment data
  const NoncontiguousBuffer& GetRequestAttachment();

  /// @brief Set response attachment data
  void SetResponseAttachment(NoncontiguousBuffer&& attachment);
// ...
};
```
