[English](../en/flatbuffers_protocol_client.md)

# 前言

本章介绍tRPC-Cpp框架调用flatbuffers的客户端例子，flatbuffers 的原理和背景可以参考：[flatbuffers_protocol_service](flatbuffers_protocol_service.md)

# 示例

示例主要介绍客户端业务逻辑编写, 对于fatbuffer的IDL文件定义、桩代码生成等步骤，可以参考[flatbuffers_protocol_service](flatbuffers_protocol_service.md)，这里不会继续赘述。

## flatbuffers的IDL文件定义和桩代码生成

以`forward.fbs`为例子，参考：[编写flatbuffers对应的IDL文件](flatbuffers_protocol_service.md#编写flatbuffers对应的IDL文件) 和 [利用fbs桩代码工具生成桩代码](flatbuffers_protocol_service.md#利用fbs桩代码工具生成桩代码)

## 基于桩代码编写客户端业务代码

- 客户端桩代码

  ```cpp
  // ... 省略部分代码
  
  namespace trpc {
  namespace test {
  namespace forward {
  // ... 省略部分代码
  class FbForwardServiceProxy : public ::trpc::RpcServiceProxy {
   public:
    // 同步接口
    virtual ::trpc::Status Forward(const ::trpc::ClientContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>* request, flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>* response);
    // 异步接口
    virtual ::trpc::Future<flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>> AsyncForward(const ::trpc::ClientContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>* request);
  };
  
  }  // namespace forward
  }  // namespace test
  }  // namespace trpc
  ```

  `FbForwardServiceProxy`是客户端代理类，用户通过该类对象调用`Forward`或者`AsyncForward`接口即可访问基于flatbuffer序列化协议的服务

- 编写客户端业务代码(完整源码参考：[client](../../examples/features/trpc_flatbuffers/client/client.cc))

  ```cpp
  void DoForward(std::shared_ptr<::trpc::test::forward::FbForwardServiceProxy>& proxy) {
    std::string msg("hello world");

    ::flatbuffers::trpc::MessageBuilder mb;
    auto name_offset = mb.CreateString(msg);
    auto request_offset = ::trpc::test::helloworld::CreateFbRequest(mb, name_offset);
    mb.Finish(request_offset);
    auto request_msg = mb.ReleaseMessage<::trpc::test::helloworld::FbRequest>();

    const ::trpc::test::helloworld::FbRequest* request = request_msg.GetRoot();

    std::cout << "request msg: " << request->message()->str() << std::endl;

    ::flatbuffers::trpc::Message<::trpc::test::helloworld::FbReply> response_msg;

    auto context = trpc::MakeClientContext(proxy);
    context->SetTimeout(1000);

    // 通过客户端对象访问后端 flatbuffer 服务
    auto status = proxy->Forward(context, request_msg, &response_msg);
    if (status.OK()) {
      const trpc::test::helloworld::FbReply* response = response_msg.GetRoot();
      std::cout << "response msg: " << response->message()->str() << std::endl;
    } else {
      std::cout << "error: " << status.ErrorMessage() << std::endl;
    }
  }
  ```

  上述源码介绍了同步调用过程，主要包含请求构建(line2～line10行)，与[基于桩代码构建服务端代码](flatbuffers_protocol_service.md#4-基于桩代码构建服务端代码)中请求和回复构建是一样；但是构建接口换成**trpc::test::helloworld::CreateFbRequest** (第6行)方法。另外；这里采用了同步接口调用：**proxy->Forward(context, request_msg, &response_msg)**，请求是 **flatbuffers::trpc::Message<::trpc::test::helloworld::FbRequest>** 的常引用；回复是 **flatbuffers::trpc::Message<::trpc::test::helloworld::FbReply>**的指针形式
