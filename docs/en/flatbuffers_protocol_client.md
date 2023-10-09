[中文版](../zh/flatbuffers_protocol_client.md)

[TOC]

# Client development based on the flatbuffers serialization protocol

# Overview

This chapter introduces a client example of the tRPC-Cpp framework using flatbuffers. For the principles and background of flatbuffers, please refer to the documentation [flatbuffers_protocol_service](../en/flatbuffers_protocol_service.md).

# Example

The example mainly focuses on writing client-side business logic. For steps such as defining flatbuffers IDL files and generating stub code, you can refer to the documentation [flatbuffers_protocol_service](../en/flatbuffers_protocol_service.md).

## Definition of flatbuffers IDL files and code generation for stubs

Taking `forward.fbs` as an example, please refer to the documentation [Writing an IDL file of flatbuffers](../en/flatbuffers_protocol_service.md#writing-an-idl-file-of-flatbuffers)和 [To generate stub code using the fbs code generation tool](../en/flatbuffers_protocol_service.md#to-generate-stub-code-using-the-fbs-code-generation-tool)

## Writing client business code based on stub code

- Client stub code

  ```cpp
  // ... Omitted code.
  
  namespace trpc {
  namespace test {
  namespace forward {
  // ... Omitted code.
  class FbForwardServiceProxy : public ::trpc::RpcServiceProxy {
   public:
    // Synchronous interface.
    virtual ::trpc::Status Forward(const ::trpc::ClientContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>* request, flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>* response);
    // Asynchronous interface.
    virtual ::trpc::Future<flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>> AsyncForward(const ::trpc::ClientContextPtr& context, const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>* request);
  };
  
  }  // namespace forward
  }  // namespace test
  }  // namespace trpc
  ```

  `FbForwardServiceProxy` is the client proxy class. Users can access the service based on the flatbuffers serialization protocol by calling the `Forward` or `AsyncForward` interfaces through objects of this class.

- Writing client business code(source code: [client](../../examples/features/trpc_flatbuffers/client/client.cc))

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

    // Accessing backend FlatBuffer service through the client object.
    auto status = proxy->Forward(context, request_msg, &response_msg);
    if (status.OK()) {
      const trpc::test::helloworld::FbReply* response = response_msg.GetRoot();
      std::cout << "response msg: " << response->message()->str() << std::endl;
    } else {
      std::cout << "error: " << status.ErrorMessage() << std::endl;
    }
  }
  ```

  The above source code introduces the process of synchronous invocation, mainly including request construction (lines 2-10). The construction of requests and responses in the [Building server-side code based on stub code](../en/flatbuffers_protocol_service.md#building-server-side-code-based-on-stub-code) is the same. However, the interface being constructed is changed to **trpc::test::helloworld::CreateFbRequest** (line 6). Additionally, synchronous interface invocation is used here: **proxy->Forward(context, request_msg, &response_msg)**, where the request is a constant reference of **flatbuffers::trpc::Message<::trpc::test::helloworld::FbRequest>**, and the response is a pointer form of **flatbuffers::trpc::Message<::trpc::test::helloworld::FbReply>**.
