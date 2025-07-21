//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/server/async_rpc/async_rpc_service_impl.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "google/protobuf/io/gzip_stream.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/codec/trpc/testing/trpc_protocol_testing.h"
#include "trpc/compressor/trpc_compressor.h"
#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/serialization/serialization_type.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/async_rpc/async_rpc_method_handler.h"
#include "trpc/server/async_rpc/async_stream_rpc_method_handler.h"
#include "trpc/server/testing/server_context_testing.h"
#include "trpc/stream/stream_async.h"
#include "trpc/stream/testing/mock_stream_provider.h"
#include "trpc/transport/server/testing/server_transport_testing.h"

namespace trpc::testing {

class AsyncRpcServiceImplTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    codec::Init();
    serialization::Init();
  }

  static void TearDownTestCase() {
    codec::Destroy();
    serialization::Destroy();
  }
};

using namespace ::trpc::test::helloworld;

class AsyncGreeter {
 public:
  Future<HelloReply> SayHello(const ServerContextPtr&, const HelloRequest* req) {
    HelloReply rsp;
    rsp.set_msg(req->msg());
    return MakeReadyFuture<HelloReply>(std::move(rsp));
  }

  Future<HelloReply> Failed(const ServerContextPtr&, const HelloRequest* req) {
    return MakeExceptionFuture<HelloReply>(UnaryRpcError(kUnknownErrorStatus));
  }

  Future<> ServerStream(const ServerContextPtr& ctx, HelloRequest&& req,
                        const stream::AsyncWriterPtr<HelloReply>& writer) {
    EXPECT_EQ(req.msg(), "ServerStream");
    return MakeReadyFuture<>();
  }

  Future<HelloReply> ClientStream(const ServerContextPtr& ctx, const stream::AsyncReaderPtr<HelloRequest>& reader) {
    return reader->Read().Then([](std::optional<HelloRequest>&& req) {
      EXPECT_TRUE(req);
      EXPECT_EQ(req.value().msg(), "ClientStream");
      HelloReply rsp;
      rsp.set_msg(std::move(req.value().msg()));
      return MakeReadyFuture<HelloReply>(std::move(rsp));
    });
  }

  Future<> BidiStream(const ServerContextPtr& ctx, const stream::AsyncReaderWriterPtr<HelloRequest, HelloReply>& rw) {
    return MakeReadyFuture<>();
  }
};

static const char* Greeter_method_names[] = {
    "/trpc.test.helloworld.Greeter/SayHello",      // 0
    "/trpc.test.helloworld.Greeter/Failed",        // 1
    "/trpc.test.helloworld.Greeter/ServerStream",  // 2
    "/trpc.test.helloworld.Greeter/ClientStream",  // 3
    "/trpc.test.helloworld.Greeter/BidiStream",    // 4
};

TEST_F(AsyncRpcServiceImplTest, Normal) {
  DummyTrpcProtocol req_data;
  req_data.func = Greeter_method_names[0];

  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("Normal");

  NoncontiguousBuffer req_bin_data;

  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  std::cout << "pack ok" << std::endl;

  std::shared_ptr<AsyncRpcServiceImpl> test_async_rpc_server_impl = std::make_shared<AsyncRpcServiceImpl>();
  std::shared_ptr<TestServerTransport> test_server_transport = std::make_shared<TestServerTransport>();

  test_async_rpc_server_impl->SetServerTransport(test_server_transport.get());

  ServerContextPtr context = MakeTestServerContext("trpc", test_async_rpc_server_impl.get(), std::move(req_bin_data));

  AsyncGreeter greeter;

  std::cout << "add method" << std::endl;
  test_async_rpc_server_impl->AddRpcServiceMethod(new trpc::RpcServiceMethod(
      Greeter_method_names[0], trpc::MethodType::UNARY,
      new trpc::AsyncRpcMethodHandler<HelloRequest, HelloReply>(
          std::bind(&AsyncGreeter::SayHello, &greeter, std::placeholders::_1, std::placeholders::_2))));

  std::cout << "dispatch begin" << std::endl;
  test_async_rpc_server_impl->Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  std::cout << "dispatch ok" << std::endl;

  ASSERT_TRUE(context->GetStatus().OK());

  STransportRspMsg* rsp_msg = test_server_transport->GetRspMsg();
  NoncontiguousBuffer rsp_bin_data = std::move(rsp_msg->buffer);

  trpc::test::helloworld::HelloReply hello_rsp;

  ASSERT_TRUE(UnPackTrpcResponse(rsp_bin_data, req_data, &hello_rsp));

  std::cout << "unpack end" << std::endl;

  ASSERT_TRUE(hello_rsp.msg() == hello_req.msg());
}

TEST_F(AsyncRpcServiceImplTest, NotFoundFunc) {
  DummyTrpcProtocol req_data;
  req_data.func = "SayHello";

  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("Normal");

  NoncontiguousBuffer req_bin_data;

  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  std::cout << "pack ok" << std::endl;

  std::shared_ptr<AsyncRpcServiceImpl> test_async_rpc_server_impl = std::make_shared<AsyncRpcServiceImpl>();
  std::shared_ptr<TestServerTransport> test_server_transport = std::make_shared<TestServerTransport>();

  test_async_rpc_server_impl->SetServerTransport(test_server_transport.get());

  ServerContextPtr context = MakeTestServerContext("trpc", test_async_rpc_server_impl.get(), std::move(req_bin_data));

  AsyncGreeter greeter;

  std::cout << "add method" << std::endl;
  test_async_rpc_server_impl->AddRpcServiceMethod(new trpc::RpcServiceMethod(
      Greeter_method_names[0], trpc::MethodType::UNARY,
      new trpc::AsyncRpcMethodHandler<HelloRequest, HelloReply>(
          std::bind(&AsyncGreeter::SayHello, &greeter, std::placeholders::_1, std::placeholders::_2))));

  std::cout << "dispatch begin" << std::endl;
  test_async_rpc_server_impl->Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  std::cout << "dispatch ok" << std::endl;

  ASSERT_TRUE(!context->GetStatus().OK());
  ASSERT_TRUE(context->GetStatus().ErrorMessage() == "SayHello not found");
}

TEST_F(AsyncRpcServiceImplTest, DecompressFailed) {
  DummyTrpcProtocol req_data;
  req_data.func = Greeter_method_names[0];

  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("Normal");

  NoncontiguousBuffer req_bin_data;

  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  std::cout << "pack ok" << std::endl;

  std::shared_ptr<AsyncRpcServiceImpl> test_async_rpc_server_impl = std::make_shared<AsyncRpcServiceImpl>();
  std::shared_ptr<TestServerTransport> test_server_transport = std::make_shared<TestServerTransport>();

  test_async_rpc_server_impl->SetServerTransport(test_server_transport.get());

  ServerContextPtr context = MakeTestServerContext("trpc", test_async_rpc_server_impl.get(), std::move(req_bin_data));

  AsyncGreeter greeter;

  test_async_rpc_server_impl->AddRpcServiceMethod(new trpc::RpcServiceMethod(
      Greeter_method_names[0], trpc::MethodType::UNARY,
      new trpc::AsyncRpcMethodHandler<HelloRequest, HelloReply>(
          std::bind(&AsyncGreeter::SayHello, &greeter, std::placeholders::_1, std::placeholders::_2))));

  context->SetReqCompressType(trpc::compressor::kGzip);

  test_async_rpc_server_impl->Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  std::cout << "dispatch ok" << std::endl;

  ASSERT_TRUE(!context->GetStatus().OK());
  ASSERT_TRUE(context->GetStatus().GetFrameworkRetCode() == TrpcRetCode::TRPC_SERVER_DECODE_ERR);
}

TEST_F(AsyncRpcServiceImplTest, FuncExecuteException) {
  DummyTrpcProtocol req_data;
  req_data.func = Greeter_method_names[1];

  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("Normal");

  NoncontiguousBuffer req_bin_data;

  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  std::shared_ptr<AsyncRpcServiceImpl> test_async_rpc_server_impl = std::make_shared<AsyncRpcServiceImpl>();
  std::shared_ptr<TestServerTransport> test_server_transport = std::make_shared<TestServerTransport>();

  test_async_rpc_server_impl->SetServerTransport(test_server_transport.get());

  ServerContextPtr context = MakeTestServerContext("trpc", test_async_rpc_server_impl.get(), std::move(req_bin_data));

  AsyncGreeter greeter;

  test_async_rpc_server_impl->AddRpcServiceMethod(
      new trpc::RpcServiceMethod(Greeter_method_names[1], trpc::MethodType::UNARY,
                                 new trpc::AsyncRpcMethodHandler<HelloRequest, HelloReply>(std::bind(
                                     &AsyncGreeter::Failed, &greeter, std::placeholders::_1, std::placeholders::_2))));

  test_async_rpc_server_impl->Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  ASSERT_TRUE(!context->GetStatus().OK());
  ASSERT_TRUE(context->GetStatus().GetFrameworkRetCode() == -99999);
}

namespace {
stream::MessageContentCodecOptions StreamCodecOptions() {
  stream::MessageContentCodecOptions content_codec{
      .serialization = serialization::SerializationFactory::GetInstance()->Get(serialization::kPbType),
      // protobuf.
      .content_type = serialization::kPbMessage,
  };
  return content_codec;
}
}  // namespace

MATCHER_P(ExpectedStatus, s, "Status") { return arg.ToString() == s.ToString(); }

TEST_F(AsyncRpcServiceImplTest, ServerStream) {
  std::shared_ptr<AsyncRpcServiceImpl> test_async_rpc_server_impl = std::make_shared<AsyncRpcServiceImpl>();
  AsyncGreeter greeter;
  test_async_rpc_server_impl->AddRpcServiceMethod(
      new trpc::RpcServiceMethod(Greeter_method_names[2], trpc::MethodType::SERVER_STREAMING,
                                 new trpc::AsyncStreamRpcMethodHandler<HelloRequest, HelloReply>(
                                     std::bind(&AsyncGreeter::ServerStream, &greeter, std::placeholders::_1,
                                               std::placeholders::_2, std::placeholders::_3))));

  DummyTrpcProtocol req_data;
  req_data.func = Greeter_method_names[2];
  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("ServerStream");
  NoncontiguousBuffer req_bin_data;
  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  auto provider = CreateMockStreamReaderWriterProvider();
  auto content_codec = StreamCodecOptions();

  NoncontiguousBuffer buff;
  ASSERT_TRUE(content_codec.serialization->Serialize(content_codec.content_type, &hello_req, &buff));

  EXPECT_CALL(*provider, AsyncStart()).WillOnce(::testing::Return(::testing::ByMove(MakeReadyFuture<>())));
  EXPECT_CALL(*provider, AsyncRead(::testing::_))
      .WillOnce(::testing::Return(
          ::testing::ByMove(MakeReadyFuture<std::optional<NoncontiguousBuffer>>(std::make_optional(buff)))));
  EXPECT_CALL(*provider, Close(ExpectedStatus(kSuccStatus))).Times(1);

  ServerContextPtr context = MakeTestServerContext("trpc", test_async_rpc_server_impl.get(), std::move(req_bin_data));
  context->SetStreamReaderWriterProvider(provider);

  test_async_rpc_server_impl->DispatchStream(context);
}

TEST_F(AsyncRpcServiceImplTest, ServerStreamFailure) {
  std::shared_ptr<AsyncRpcServiceImpl> test_async_rpc_server_impl = std::make_shared<AsyncRpcServiceImpl>();
  AsyncGreeter greeter;
  test_async_rpc_server_impl->AddRpcServiceMethod(
      new trpc::RpcServiceMethod(Greeter_method_names[2], trpc::MethodType::SERVER_STREAMING,
                                 new trpc::AsyncStreamRpcMethodHandler<HelloRequest, HelloReply>(
                                     std::bind(&AsyncGreeter::ServerStream, &greeter, std::placeholders::_1,
                                               std::placeholders::_2, std::placeholders::_3))));
  DummyTrpcProtocol req_data;
  req_data.func = Greeter_method_names[2];
  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("ServerStream");
  NoncontiguousBuffer req_bin_data;
  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  auto provider = CreateMockStreamReaderWriterProvider();
  auto content_codec = StreamCodecOptions();

  NoncontiguousBuffer buff;
  ASSERT_TRUE(content_codec.serialization->Serialize(content_codec.content_type, &hello_req, &buff));

  EXPECT_CALL(*provider, AsyncStart()).WillOnce(::testing::Return(::testing::ByMove(MakeReadyFuture<>())));
  EXPECT_CALL(*provider, AsyncRead(::testing::_))
      .WillOnce(::testing::Return(::testing::ByMove(
          MakeExceptionFuture<std::optional<NoncontiguousBuffer>>(stream::StreamError(kUnknownErrorStatus)))));
  EXPECT_CALL(*provider, Reset(ExpectedStatus(kUnknownErrorStatus))).Times(1);

  ServerContextPtr context = MakeTestServerContext("trpc", test_async_rpc_server_impl.get(), std::move(req_bin_data));
  context->SetStreamReaderWriterProvider(provider);

  test_async_rpc_server_impl->DispatchStream(context);
}

MATCHER_P(ExpectedRsp, expected, "Serialized Buffer") { return FlattenSlow(arg) == FlattenSlow(expected); }

TEST_F(AsyncRpcServiceImplTest, ClientStream) {
  std::shared_ptr<AsyncRpcServiceImpl> test_async_rpc_server_impl = std::make_shared<AsyncRpcServiceImpl>();
  AsyncGreeter greeter;
  test_async_rpc_server_impl->AddRpcServiceMethod(new trpc::RpcServiceMethod(
      Greeter_method_names[3], trpc::MethodType::SERVER_STREAMING,
      new trpc::AsyncStreamRpcMethodHandler<HelloRequest, HelloReply>(
          std::bind(&AsyncGreeter::ClientStream, &greeter, std::placeholders::_1, std::placeholders::_2))));

  DummyTrpcProtocol req_data;
  req_data.func = Greeter_method_names[3];
  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("ClientStream");
  NoncontiguousBuffer req_bin_data;
  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  auto provider = CreateMockStreamReaderWriterProvider();
  auto content_codec = StreamCodecOptions();

  NoncontiguousBuffer buff;
  ASSERT_TRUE(content_codec.serialization->Serialize(content_codec.content_type, &hello_req, &buff));

  EXPECT_CALL(*provider, AsyncStart()).WillOnce(::testing::Return(::testing::ByMove(MakeReadyFuture<>())));
  EXPECT_CALL(*provider, AsyncRead(::testing::_))
      .WillOnce(::testing::Return(
          ::testing::ByMove(MakeReadyFuture<std::optional<NoncontiguousBuffer>>(std::make_optional(buff)))));

  HelloReply rsp;
  rsp.set_msg("ClientStream");
  NoncontiguousBuffer expected_buff;
  ASSERT_TRUE(content_codec.serialization->Serialize(content_codec.content_type, &rsp, &expected_buff));

  EXPECT_CALL(*provider, AsyncWrite(ExpectedRsp(expected_buff)))
      .WillOnce(::testing::Return(::testing::ByMove(MakeReadyFuture<>())));

  EXPECT_CALL(*provider, Close(ExpectedStatus(kSuccStatus))).Times(1);

  ServerContextPtr context = MakeTestServerContext("trpc", test_async_rpc_server_impl.get(), std::move(req_bin_data));
  context->SetStreamReaderWriterProvider(provider);

  test_async_rpc_server_impl->DispatchStream(context);
}

TEST_F(AsyncRpcServiceImplTest, BidiStream) {
  std::shared_ptr<AsyncRpcServiceImpl> test_async_rpc_server_impl = std::make_shared<AsyncRpcServiceImpl>();
  AsyncGreeter greeter;
  test_async_rpc_server_impl->AddRpcServiceMethod(new trpc::RpcServiceMethod(
      Greeter_method_names[4], trpc::MethodType::SERVER_STREAMING,
      new trpc::AsyncStreamRpcMethodHandler<HelloRequest, HelloReply>(
          std::bind(&AsyncGreeter::BidiStream, &greeter, std::placeholders::_1, std::placeholders::_2))));

  DummyTrpcProtocol req_data;
  req_data.func = Greeter_method_names[4];
  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("BidiStream");
  NoncontiguousBuffer req_bin_data;
  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  auto provider = CreateMockStreamReaderWriterProvider();
  EXPECT_CALL(*provider, AsyncStart()).WillOnce(::testing::Return(::testing::ByMove(MakeReadyFuture<>())));
  EXPECT_CALL(*provider, Close(ExpectedStatus(kSuccStatus))).Times(1);

  ServerContextPtr context = MakeTestServerContext("trpc", test_async_rpc_server_impl.get(), std::move(req_bin_data));
  context->SetStreamReaderWriterProvider(provider);

  test_async_rpc_server_impl->DispatchStream(context);
}

}  // namespace trpc::testing
