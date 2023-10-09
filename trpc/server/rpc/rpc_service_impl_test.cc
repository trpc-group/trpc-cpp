//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/server/rpc/rpc_service_impl.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/codec/trpc/testing/trpc_protocol_testing.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/proto/testing/helloworld_generated.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/rpc/rpc_method_handler.h"
#include "trpc/server/rpc/stream_rpc_method_handler.h"
#include "trpc/server/testing/server_context_testing.h"
#include "trpc/server/testing/service_adapter_testing.h"
#include "trpc/stream/stream.h"
#include "trpc/stream/testing/mock_stream_handler.h"
#include "trpc/stream/testing/mock_stream_provider.h"
#include "trpc/util/flatbuffers/trpc_fbs.h"

namespace trpc::testing {

using FbReply = flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>;

class RpcServiceImplTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    codec::Init();
    serialization::Init();
  }

  static void TearDownTestCase() {
    codec::Destroy();
    serialization::Destroy();
  }

 protected:
};

class Greeter {
 public:
  trpc::Status SayHello(trpc::ServerContextPtr context, const trpc::test::helloworld::HelloRequest* request,
                        trpc::test::helloworld::HelloReply* reply) {
    reply->set_msg(request->msg());
    return trpc::Status(0, "");
  }

  trpc::Status FbsSayHello(trpc::ServerContextPtr context,
                           const flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>* request,
                           flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>* reply) {
    flatbuffers::trpc::MessageBuilder mb;
    auto name_offset = mb.CreateString(request->GetRoot()->message()->str());
    auto rsp_offset = trpc::test::helloworld::CreateFbReply(mb, name_offset);
    mb.Finish(rsp_offset);
    *reply = mb.ReleaseMessage<trpc::test::helloworld::FbReply>();
    return trpc::Status(0, "");
  }

  trpc::Status StringSayHello(trpc::ServerContextPtr context, const std::string* request, std::string* reply) {
    *reply = *request;
    return trpc::Status(0, "");
  }

  trpc::Status NoncontiguousBufferSayHello(trpc::ServerContextPtr context, const trpc::NoncontiguousBuffer* request,
                                           trpc::NoncontiguousBuffer* reply) {
    *reply = *request;
    return trpc::Status(0, "");
  }

  trpc::Status RapidJsonSayHello(trpc::ServerContextPtr context, const rapidjson::Document* request,
                                 rapidjson::Document* reply) {
    reply->CopyFrom(*request, const_cast<rapidjson::Document*>(request)->GetAllocator());
    return trpc::Status(0, "");
  }

  trpc::Status ServerStreamSayHello(const trpc::ServerContextPtr& context,
                                    const trpc::test::helloworld::HelloRequest& request,
                                    trpc::stream::StreamWriter<trpc::test::helloworld::HelloReply>* writer) {
    for (int i = 10; i > 0; i--) {
      trpc::test::helloworld::HelloReply reply;
      reply.set_msg(request.msg());
      writer->Write(reply);
    }
    return trpc::Status(0, "OK, ServerStreamSayHello");
  }

  trpc::Status ClientStreamSayHello(const trpc::ServerContextPtr& context,
                                    const trpc::stream::StreamReader<trpc::test::helloworld::HelloRequest>& reader,
                                    trpc::test::helloworld::HelloReply* reply) {
    trpc::Status status{};
    int count{0};
    for (;;) {
      trpc::test::helloworld::HelloRequest request{};
      status = reader.Read(&request);
      if (status.OK()) {
        count++;
        continue;
      }
      break;
    }
    reply->set_msg(std::to_string(count));
    return trpc::Status(0, "OK, ClientStreamSayHello");
  }

  trpc::Status BidiStreamSayHello(const trpc::ServerContextPtr& context,
                                  const trpc::stream::StreamReader<trpc::test::helloworld::HelloRequest>& reader,
                                  trpc::stream::StreamWriter<trpc::test::helloworld::HelloReply>* writer) {
    trpc::Status status{};
    for (;;) {
      trpc::test::helloworld::HelloRequest request{};
      status = reader.Read(&request);
      if (status.OK()) {
        trpc::test::helloworld::HelloReply reply;
        reply.set_msg(request.msg());
        writer->Write(reply);
        continue;
      }
      break;
    }
    return trpc::Status(0, "OK, BidiStreamSayHello");
  }
};

static const char* Greeter_method_names[] = {
    "/trpc.test.helloworld.Greeter/SayHello",                     // 0
    "/trpc.test.helloworld.Greeter/FbsSayHello",                  // 1
    "/trpc.test.helloworld.Greeter/StringSayHello",               // 2
    "/trpc.test.helloworld.Greeter/NoncontiguousBufferSayHello",  // 3
    "/trpc.test.helloworld.Greeter/RapidJsonSayHello",            // 4
    "/trpc.test.helloworld.Greeter/ServerStreamSayHello",         // 5
    "/trpc.test.helloworld.Greeter/ClientStreamSayHello",         // 6
    "/trpc.test.helloworld.Greeter/BidiStreamSayHello",           // 7
};

TEST_F(RpcServiceImplTest, PbMessage) {
  DummyTrpcProtocol req_data;
  req_data.func = Greeter_method_names[0];

  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("Normal");

  NoncontiguousBuffer req_bin_data;

  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  std::cout << "pack ok" << std::endl;

  std::shared_ptr<RpcServiceImpl> test_rpc_server_impl = std::make_shared<RpcServiceImpl>();

  ServerContextPtr context = MakeTestServerContext("trpc", test_rpc_server_impl.get(), std::move(req_bin_data));

  Greeter greeter;

  test_rpc_server_impl->AddRpcServiceMethod(new trpc::RpcServiceMethod(
      Greeter_method_names[0], trpc::MethodType::UNARY,
      new trpc::RpcMethodHandler<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(std::bind(
          &Greeter::SayHello, &greeter, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))));

  test_rpc_server_impl->Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  std::cout << "dispatch ok" << std::endl;

  ASSERT_TRUE(context->GetStatus().OK());

  trpc::test::helloworld::HelloReply hello_rsp;

  NoncontiguousBuffer rsp_bin_data = context->GetResponseMsg()->GetNonContiguousProtocolBody();

  std::cout << "pack begin" << std::endl;

  ASSERT_TRUE(UnPackTrpcResponseBody(rsp_bin_data, req_data, &hello_rsp));

  ASSERT_TRUE(hello_rsp.msg() == hello_req.msg());
}

TEST_F(RpcServiceImplTest, NotFoundFunc) {
  DummyTrpcProtocol req_data;
  req_data.func = "SayHello";

  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("Normal");

  NoncontiguousBuffer req_bin_data;

  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  std::shared_ptr<RpcServiceImpl> test_rpc_server_impl = std::make_shared<RpcServiceImpl>();

  ServerContextPtr context = MakeTestServerContext("trpc", test_rpc_server_impl.get(), std::move(req_bin_data));

  Greeter greeter;

  test_rpc_server_impl->AddRpcServiceMethod(new trpc::RpcServiceMethod(
      Greeter_method_names[0], trpc::MethodType::UNARY,
      new trpc::RpcMethodHandler<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(std::bind(
          &Greeter::SayHello, &greeter, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))));

  test_rpc_server_impl->Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  ASSERT_TRUE(!context->GetStatus().OK());
  ASSERT_TRUE(context->GetStatus().ErrorMessage() == "SayHello not found");
}

TEST_F(RpcServiceImplTest, FbsMessage) {
  flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest> hello_req;
  flatbuffers::trpc::MessageBuilder req_mb;
  auto req_name_offset = req_mb.CreateString("hello world");
  auto request_offset = trpc::test::helloworld::CreateFbRequest(req_mb, req_name_offset);
  req_mb.Finish(request_offset);
  hello_req = req_mb.ReleaseMessage<trpc::test::helloworld::FbRequest>();

  DummyTrpcProtocol req_data;
  req_data.func = Greeter_method_names[1];
  req_data.data_type = serialization::kFlatBuffers;
  req_data.content_type = TrpcContentEncodeType::TRPC_FLATBUFFER_ENCODE;
  NoncontiguousBuffer req_bin_data;

  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  std::shared_ptr<RpcServiceImpl> test_rpc_server_impl = std::make_shared<RpcServiceImpl>();

  ServerContextPtr context = MakeTestServerContext("trpc", test_rpc_server_impl.get(), std::move(req_bin_data));

  Greeter greeter;

  test_rpc_server_impl->AddRpcServiceMethod(new trpc::RpcServiceMethod(
      Greeter_method_names[1], trpc::MethodType::UNARY,
      new trpc::RpcMethodHandler<flatbuffers::trpc::Message<trpc::test::helloworld::FbRequest>,
                                 flatbuffers::trpc::Message<trpc::test::helloworld::FbReply>>(std::bind(
          &Greeter::FbsSayHello, &greeter, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))));

  test_rpc_server_impl->Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  std::cout << "dispatch err msg:" << context->GetStatus().ErrorMessage() << std::endl;

  ASSERT_TRUE(context->GetStatus().OK());

  FbReply hello_reply;
  NoncontiguousBuffer rsp_bin_data = context->GetResponseMsg()->GetNonContiguousProtocolBody();

  ASSERT_TRUE(UnPackTrpcResponseBody(rsp_bin_data, req_data, &hello_reply));
  ASSERT_TRUE(hello_reply.GetRoot()->message()->str() == "hello world");
}

TEST_F(RpcServiceImplTest, StringMessage) {
  std::string hello_req("hello world");

  DummyTrpcProtocol req_data;
  req_data.func = Greeter_method_names[2];
  req_data.data_type = serialization::kStringNoop;
  req_data.content_type = TrpcContentEncodeType::TRPC_NOOP_ENCODE;
  NoncontiguousBuffer req_bin_data;

  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  std::cout << "pack ok" << std::endl;

  std::shared_ptr<RpcServiceImpl> test_rpc_server_impl = std::make_shared<RpcServiceImpl>();

  ServerContextPtr context = MakeTestServerContext("trpc", test_rpc_server_impl.get(), std::move(req_bin_data));

  Greeter greeter;

  test_rpc_server_impl->AddRpcServiceMethod(new trpc::RpcServiceMethod(
      Greeter_method_names[2], trpc::MethodType::UNARY,
      new trpc::RpcMethodHandler<std::string, std::string>(std::bind(
          &Greeter::StringSayHello, &greeter, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))));

  test_rpc_server_impl->Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  std::cout << "dispatch ok" << std::endl;

  ASSERT_TRUE(context->GetStatus().OK());

  std::string hello_rsp;

  NoncontiguousBuffer rsp_bin_data = context->GetResponseMsg()->GetNonContiguousProtocolBody();

  std::cout << "pack begin" << std::endl;

  ASSERT_TRUE(UnPackTrpcResponseBody(rsp_bin_data, req_data, &hello_rsp));

  ASSERT_TRUE(hello_rsp == hello_req);
}

TEST_F(RpcServiceImplTest, NoncontiguousBufferMessage) {
  NoncontiguousBuffer hello_req = CreateBufferSlow("hello world");

  DummyTrpcProtocol req_data;
  req_data.func = Greeter_method_names[3];
  req_data.data_type = serialization::kNonContiguousBufferNoop;
  req_data.content_type = TrpcContentEncodeType::TRPC_NOOP_ENCODE;
  NoncontiguousBuffer req_bin_data;

  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  std::cout << "pack ok" << std::endl;

  std::shared_ptr<RpcServiceImpl> test_rpc_server_impl = std::make_shared<RpcServiceImpl>();

  ServerContextPtr context = MakeTestServerContext("trpc", test_rpc_server_impl.get(), std::move(req_bin_data));

  Greeter greeter;

  test_rpc_server_impl->AddRpcServiceMethod(
      new trpc::RpcServiceMethod(Greeter_method_names[3], trpc::MethodType::UNARY,
                                 new trpc::RpcMethodHandler<trpc::NoncontiguousBuffer, trpc::NoncontiguousBuffer>(
                                     std::bind(&Greeter::NoncontiguousBufferSayHello, &greeter, std::placeholders::_1,
                                               std::placeholders::_2, std::placeholders::_3))));

  test_rpc_server_impl->Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  std::cout << "dispatch ok" << std::endl;

  ASSERT_TRUE(context->GetStatus().OK());

  NoncontiguousBuffer hello_rsp;

  NoncontiguousBuffer rsp_bin_data = context->GetResponseMsg()->GetNonContiguousProtocolBody();

  std::cout << "pack begin" << std::endl;

  ASSERT_TRUE(UnPackTrpcResponseBody(rsp_bin_data, req_data, &hello_rsp));

  std::string rsp = trpc::FlattenSlow(hello_rsp);

  ASSERT_TRUE(rsp == "hello world");
}

TEST_F(RpcServiceImplTest, RapidJsonMessage) {
  std::string json_str = "{\"age\":\"18\",\"height\":180}";

  rapidjson::Document hello_req;
  hello_req.Parse(json_str.c_str());

  ASSERT_TRUE(!hello_req.HasParseError());

  DummyTrpcProtocol req_data;
  req_data.func = Greeter_method_names[4];
  req_data.data_type = serialization::kRapidJson;
  req_data.content_type = TrpcContentEncodeType::TRPC_JSON_ENCODE;
  NoncontiguousBuffer req_bin_data;

  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  std::cout << "pack ok" << std::endl;

  std::shared_ptr<RpcServiceImpl> test_rpc_server_impl = std::make_shared<RpcServiceImpl>();

  ServerContextPtr context = MakeTestServerContext("trpc", test_rpc_server_impl.get(), std::move(req_bin_data));

  Greeter greeter;

  test_rpc_server_impl->AddRpcServiceMethod(
      new trpc::RpcServiceMethod(Greeter_method_names[4], trpc::MethodType::UNARY,
                                 new trpc::RpcMethodHandler<rapidjson::Document, rapidjson::Document>(
                                     std::bind(&Greeter::RapidJsonSayHello, &greeter, std::placeholders::_1,
                                               std::placeholders::_2, std::placeholders::_3))));

  test_rpc_server_impl->Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  std::cout << "dispatch ok" << std::endl;

  ASSERT_TRUE(context->GetStatus().OK());

  rapidjson::Document hello_rsp;

  NoncontiguousBuffer rsp_bin_data = context->GetResponseMsg()->GetNonContiguousProtocolBody();

  std::cout << "pack begin" << std::endl;

  ASSERT_TRUE(UnPackTrpcResponseBody(rsp_bin_data, req_data, &hello_rsp));

  rapidjson::Value::ConstMemberIterator it = hello_rsp.FindMember("age");
  ASSERT_TRUE(it != hello_rsp.MemberEnd());
  ASSERT_TRUE(it->value.GetString() == std::string("18"));
}

namespace {
NoncontiguousBuffer HelloRequestToBuffer(const trpc::test::helloworld::HelloRequest& hello_request) {
  std::string msg_buffer{""};
  hello_request.SerializeToString(&msg_buffer);
  return CreateBufferSlow(msg_buffer);
}

std::deque<NoncontiguousBuffer> CreateHelloRequests(const trpc::test::helloworld::HelloRequest& hello_request,
                                                    int count) {
  std::deque<NoncontiguousBuffer> msgs;
  for (int i = 0; i < count; ++i) {
    msgs.emplace_back(HelloRequestToBuffer(hello_request));
  }
  return msgs;
}

void MockStream(MockStreamReaderWriterProviderPtr& stream, std::deque<NoncontiguousBuffer>& req_msgs,
                std::deque<NoncontiguousBuffer>& rsp_msgs) {
  EXPECT_CALL(*stream, Read(::testing::_, ::testing::_))
      .WillRepeatedly([&req_msgs](NoncontiguousBuffer* msg, int) mutable {
        if (req_msgs.empty()) {
          return Status{static_cast<int>(stream::StreamStatus::kStreamEof), 0, "stream eof"};
        }
        *msg = std::move(req_msgs.front());
        req_msgs.pop_front();
        return kSuccStatus;
      });

  EXPECT_CALL(*stream, Start()).WillRepeatedly(::testing::Return(kSuccStatus));
  EXPECT_CALL(*stream, Close(::testing::_)).Times(1);

  EXPECT_CALL(*stream, Write(::testing::_)).WillRepeatedly([&rsp_msgs](NoncontiguousBuffer&& msg) {
    rsp_msgs.emplace_back(std::move(msg));
    return kSuccStatus;
  });
  auto stream_options = std::make_shared<stream::StreamOptions>();
  EXPECT_CALL(*stream, GetMutableStreamOptions()).WillRepeatedly([stream_options]() { return stream_options.get(); });
}
}  // namespace

TEST_F(RpcServiceImplTest, ServerStreamSayHello) {
  RunAsFiber([] {
    Greeter greeter;
    std::shared_ptr<RpcServiceImpl> test_rpc_server_impl = std::make_shared<RpcServiceImpl>();
    test_rpc_server_impl->AddRpcServiceMethod(new trpc::RpcServiceMethod(
        Greeter_method_names[5], trpc::MethodType::SERVER_STREAMING,
        new trpc::StreamRpcMethodHandler<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(
            std::bind(&Greeter::ServerStreamSayHello, &greeter, std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3))));
    trpc::ServiceAdapterOption service_adapter_option = CreateServiceAdapterOption();
    ServiceAdapter* service_adapter = new ServiceAdapter(std::move(service_adapter_option));
    FillServiceAdapter(service_adapter, "trpc.test.helloworld.Greeter", test_rpc_server_impl);

    DummyTrpcProtocol req_data;
    req_data.func = Greeter_method_names[5];
    trpc::test::helloworld::HelloRequest hello_req;
    hello_req.set_msg("ServerStreamSayHello");
    NoncontiguousBuffer req_bin_data;
    ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

    std::deque<NoncontiguousBuffer> req_msgs = CreateHelloRequests(hello_req, 1);
    std::deque<NoncontiguousBuffer> rsp_msgs;
    auto mock_stream_provider = CreateMockStreamReaderWriterProvider();
    MockStream(mock_stream_provider, req_msgs, rsp_msgs);

    ServerContextPtr context = MakeTestServerContext("trpc", test_rpc_server_impl.get(), std::move(req_bin_data));
    context->SetStreamReaderWriterProvider(mock_stream_provider);

    test_rpc_server_impl->DispatchStream(context);
    // FIXME: Sleep is not a good idea.
    FiberSleepFor(std::chrono::seconds(2));
    ASSERT_TRUE(context->GetStatus().OK());
    ASSERT_EQ(context->GetStatus().ErrorMessage(), "OK, ServerStreamSayHello");
    ASSERT_EQ(rsp_msgs.size(), 10);
    for (auto it = rsp_msgs.begin(); it != rsp_msgs.end(); ++it) {
      trpc::test::helloworld::HelloReply hello_rsp;
      NoncontiguousBuffer rsp_bin_data = std::move(*it);
      ASSERT_TRUE(UnPackTrpcResponseBody(rsp_bin_data, req_data, &hello_rsp));
      ASSERT_EQ(hello_rsp.msg(), hello_req.msg());
    }
  });
}

TEST_F(RpcServiceImplTest, ClientStreamSayHello) {
  RunAsFiber([] {
    Greeter greeter;
    std::shared_ptr<RpcServiceImpl> test_rpc_server_impl = std::make_shared<RpcServiceImpl>();
    test_rpc_server_impl->AddRpcServiceMethod(new trpc::RpcServiceMethod(
        Greeter_method_names[6], trpc::MethodType::CLIENT_STREAMING,
        new trpc::StreamRpcMethodHandler<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(
            std::bind(&Greeter::ClientStreamSayHello, &greeter, std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3))));
    trpc::ServiceAdapterOption service_adapter_option = CreateServiceAdapterOption();
    ServiceAdapter* service_adapter = new ServiceAdapter(std::move(service_adapter_option));
    FillServiceAdapter(service_adapter, "trpc.test.helloworld.Greeter", test_rpc_server_impl);

    DummyTrpcProtocol req_data;
    req_data.func = Greeter_method_names[6];
    trpc::test::helloworld::HelloRequest hello_req;
    hello_req.set_msg("ClientStreamSayHello");
    NoncontiguousBuffer req_bin_data;
    ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

    std::deque<NoncontiguousBuffer> req_msgs = CreateHelloRequests(hello_req, 10);
    std::deque<NoncontiguousBuffer> rsp_msgs;
    auto mock_stream_provider = CreateMockStreamReaderWriterProvider();
    MockStream(mock_stream_provider, req_msgs, rsp_msgs);

    ServerContextPtr context = MakeTestServerContext("trpc", test_rpc_server_impl.get(), std::move(req_bin_data));
    context->SetStreamReaderWriterProvider(mock_stream_provider);

    test_rpc_server_impl->DispatchStream(context);

    // FIXME: Sleep is not a good idea.
    FiberSleepFor(std::chrono::seconds(2));
    ASSERT_TRUE(context->GetStatus().OK());
    ASSERT_EQ(context->GetStatus().ErrorMessage(), "OK, ClientStreamSayHello");
    ASSERT_EQ(rsp_msgs.size(), 1);
    for (auto it = rsp_msgs.begin(); it != rsp_msgs.end(); ++it) {
      trpc::test::helloworld::HelloReply hello_rsp;
      NoncontiguousBuffer rsp_bin_data = std::move(*it);
      ASSERT_TRUE(UnPackTrpcResponseBody(rsp_bin_data, req_data, &hello_rsp));
      ASSERT_EQ(hello_rsp.msg(), "10");
    }
  });
}

TEST_F(RpcServiceImplTest, BidiStreamSayHello) {
  RunAsFiber([this] {
    Greeter greeter;
    std::shared_ptr<RpcServiceImpl> test_rpc_server_impl = std::make_shared<RpcServiceImpl>();
    test_rpc_server_impl->AddRpcServiceMethod(new trpc::RpcServiceMethod(
        Greeter_method_names[7], trpc::MethodType::BIDI_STREAMING,
        new trpc::StreamRpcMethodHandler<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(
            std::bind(&Greeter::BidiStreamSayHello, &greeter, std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3))));
    trpc::ServiceAdapterOption service_adapter_option = CreateServiceAdapterOption();
    ServiceAdapter* service_adapter = new ServiceAdapter(std::move(service_adapter_option));
    FillServiceAdapter(service_adapter, "trpc.test.helloworld.Greeter", test_rpc_server_impl);

    DummyTrpcProtocol req_data;
    req_data.func = Greeter_method_names[7];
    trpc::test::helloworld::HelloRequest hello_req;
    hello_req.set_msg("BidiStreamSayHello");
    NoncontiguousBuffer req_bin_data;
    ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

    std::deque<NoncontiguousBuffer> req_msgs = CreateHelloRequests(hello_req, 10);
    std::deque<NoncontiguousBuffer> rsp_msgs;
    auto mock_stream_provider = CreateMockStreamReaderWriterProvider();
    MockStream(mock_stream_provider, req_msgs, rsp_msgs);
    ServerContextPtr context = MakeTestServerContext("trpc", test_rpc_server_impl.get(), std::move(req_bin_data));
    context->SetStreamReaderWriterProvider(mock_stream_provider);

    test_rpc_server_impl->DispatchStream(context);
    // FIXME: Sleep is not a good idea.
    FiberSleepFor(std::chrono::seconds(2));
    ASSERT_TRUE(context->GetStatus().OK());
    ASSERT_EQ(context->GetStatus().ErrorMessage(), "OK, BidiStreamSayHello");
    ASSERT_EQ(rsp_msgs.size(), 10);
    for (auto it = rsp_msgs.begin(); it != rsp_msgs.end(); ++it) {
      trpc::test::helloworld::HelloReply hello_rsp;
      NoncontiguousBuffer rsp_bin_data = std::move(*it);
      ASSERT_TRUE(UnPackTrpcResponseBody(rsp_bin_data, req_data, &hello_rsp));
      ASSERT_EQ(hello_rsp.msg(), hello_req.msg());
    }
  });
}

}  // namespace trpc::testing
