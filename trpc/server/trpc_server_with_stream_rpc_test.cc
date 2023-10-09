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

#include "trpc/server/trpc_server.h"

#include "gtest/gtest.h"

#include "trpc/codec/trpc/testing/trpc_protocol_testing.h"
#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/server/testing/fiber_server_testing.h"
#include "trpc/server/testing/greeter_service_testing.h"
#include "trpc/server/testing/http_client_testing.h"
#include "trpc/server/testing/http_service_testing.h"
#include "trpc/server/testing/trpc_client_testing.h"
#ifdef TRPC_BUILD_INCLUDE_SSL
#include "trpc/transport/common/ssl/ssl.h"
#endif

namespace trpc::testing {

namespace {
struct Input {
  std::string ip;
  int port;
  bool is_ipv6{false};

  std::string func;
  std::string caller;
  std::string callee;
};

RefPtr<TestTrpcClientStreamProvider> CreateTestTrpcStreamReaderWriterProvider(const Input& in) {
  trpc::Socket socket;
  if (!CreateTcpSocket(in.ip, in.port, in.is_ipv6, &socket)) {
    std::cerr << "failed to create tcp socket" << std::endl;
    return nullptr;
  }
  TestTrpcClientStreamProvider::Options options{};
  options.stream_id = NextStreamId();
  options.func = in.func;
  options.caller = in.caller;
  options.callee = in.callee;
  options.transport = std::make_shared<TestStreamTransportImpl>(std::move(socket));

  RefPtr<TestTrpcClientStreamProvider> provider = trpc::MakeRefCounted<TestTrpcClientStreamProvider>(options);
  // Handshake, send init and recv init.
  trpc::Status status = provider->Start();
  if (!status.OK()) {
    std::cerr << status.ToString() << std::endl;
    return nullptr;
  }

  return provider;
}

bool TestClientStreamFuncNotFound() {
  Input in{};
  in.ip = "127.0.0.1";
  in.port = 13451;
  in.func = "/trpc.test.helloworld.Greeter/ClientStreamSayHello-Not-Found";
  in.caller = "greeter.client";
  in.callee = "trpc.test.helloworld.Greeter";

  auto provider = CreateTestTrpcStreamReaderWriterProvider(in);
  if (provider == nullptr) return false;
  return true;
}

bool TestClientStreamSayHello() {
  Input in{};
  in.ip = "127.0.0.1";
  in.port = 13451;
  in.func = "/trpc.test.helloworld.Greeter/ClientStreamSayHello";
  in.caller = "greeter.client";
  in.callee = "trpc.test.helloworld.Greeter";

  auto provider = CreateTestTrpcStreamReaderWriterProvider(in);
  if (provider == nullptr) return false;

  auto codec = trpc::stream::MessageContentCodecOptions{
      .serialization = serialization::SerializationFactory::GetInstance()->Get(serialization::kPbType),
      .content_type = serialization::kPbType,
  };
  auto stream =
      trpc::stream::Create<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(provider, codec);
  // Writes request.
  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("ClientStreamSayHello");
  for (int i = 0; i < 10; i++) {
    auto status = stream.Write(hello_req);
    if (!status.OK()) {
      std::cerr << "write error: " << status.ToString() << std::endl;
      return false;
    }
  }

  // Writes done.
  auto status = stream.WriteDone();
  if (!status.OK()) {
    std::cerr << "write done error: " << status.ToString() << std::endl;
    return false;
  }

  // Reads response.
  trpc::test::helloworld::HelloReply hello_rsp;
  status = stream.Read(&hello_rsp);
  if (!status.OK()) {
    std::cerr << "read error: " << status.ToString() << std::endl;
    return false;
  }
  std::cout << "content of reply:" << hello_rsp.msg() << std::endl;
  // Expect reply content: "10"
  if (hello_rsp.msg() != "10") {
    std::cerr << "check response error" << std::endl;
    return false;
  }

  // Expect: EOF
  status = stream.Read(&hello_rsp);
  if (!status.StreamEof()) {
    std::cerr << "wait eof error: " << status.ToString() << std::endl;
    return false;
  }
  return true;
}

bool TestServerStreamSayHello() {
  Input in{};
  in.ip = "127.0.0.1";
  in.port = 13451;
  in.func = "/trpc.test.helloworld.Greeter/ServerStreamSayHello";
  in.caller = "greeter.client";
  in.callee = "trpc.test.helloworld.Greeter";

  auto provider = CreateTestTrpcStreamReaderWriterProvider(in);
  if (provider == nullptr) return false;

  auto codec = trpc::stream::MessageContentCodecOptions{
      .serialization = serialization::SerializationFactory::GetInstance()->Get(serialization::kPbType),
      .content_type = serialization::kPbType,
  };
  auto stream =
      trpc::stream::Create<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(provider, codec);
  // Writes request.
  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("ServerStreamSayHello");
  auto status = stream.Write(hello_req);
  if (!status.OK()) {
    std::cerr << "write error: " << status.ToString() << std::endl;
    return false;
  }

  // Writes done.
  status = stream.WriteDone();
  if (!status.OK()) {
    std::cerr << "write done error: " << status.ToString() << std::endl;
    return false;
  }

  // Reads response.
  for (int i = 0; i < 10; i++) {
    trpc::test::helloworld::HelloReply hello_rsp;
    status = stream.Read(&hello_rsp);
    if (!status.OK()) {
      std::cerr << "read error: " << status.ToString() << std::endl;
      return false;
    }
    std::cout << "content of reply:" << hello_rsp.msg() << std::endl;
    // Expect reply content: request.msg();
    if (hello_rsp.msg() != hello_req.msg()) {
      std::cerr << "check response error" << std::endl;
      return false;
    }
  }

  // Expect: EOF
  trpc::test::helloworld::HelloReply hello_rsp;
  status = stream.Read(&hello_rsp);
  if (!status.StreamEof()) {
    std::cerr << "wait eof error: " << status.ToString() << std::endl;
    return false;
  }
  return true;
}

bool TestBidiStreamSayHello() {
  Input in{};
  in.ip = "127.0.0.1";
  in.port = 13451;
  in.func = "/trpc.test.helloworld.Greeter/BidiStreamSayHello";
  in.caller = "greeter.client";
  in.callee = "trpc.test.helloworld.Greeter";

  auto provider = CreateTestTrpcStreamReaderWriterProvider(in);
  if (provider == nullptr) return false;

  auto codec = trpc::stream::MessageContentCodecOptions{
      .serialization = serialization::SerializationFactory::GetInstance()->Get(serialization::kPbType),
      .content_type = serialization::kPbType,
  };
  auto stream =
      trpc::stream::Create<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloReply>(provider, codec);

  // Writes request.
  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("BidiStreamSayHello");
  for (int i = 0; i < 10; i++) {
    auto status = stream.Write(hello_req);
    if (!status.OK()) {
      std::cerr << "write error: " << status.ToString() << std::endl;
      return false;
    }
  }

  // Writes done.
  auto status = stream.WriteDone();
  if (!status.OK()) {
    std::cerr << "write done error: " << status.ToString() << std::endl;
    return false;
  }

  // Reads response.
  for (int i = 0; i < 10; i++) {
    trpc::test::helloworld::HelloReply hello_rsp;
    status = stream.Read(&hello_rsp);
    if (!status.OK()) {
      std::cerr << "read error: " << status.ToString() << std::endl;
      return false;
    }
    std::cout << "content of reply:" << hello_rsp.msg() << std::endl;
    // Expect reply content: request.msg();
    if (hello_rsp.msg() != hello_req.msg()) {
      std::cerr << "check response error" << std::endl;
      return false;
    }
  }

  // Expect: EOF
  trpc::test::helloworld::HelloReply hello_rsp;
  status = stream.Read(&hello_rsp);
  if (!status.StreamEof()) {
    std::cerr << "wait eof error: " << status.ToString() << std::endl;
    return false;
  }

  return true;
}
}  // namespace

namespace {
bool TestUnarySayHello() {
  NetworkAddress addr("127.0.0.1", 13453);
  Socket socket = Socket::CreateTcpSocket(addr.IsIpv6());
  int ret = socket.Connect(addr);
  if (ret != 0) return false;

  DummyTrpcProtocol req_data;
  req_data.func = "/trpc.test.helloworld.Greeter/SayHello";
  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("hello world");

  NoncontiguousBuffer req_bin_data;
  if (!PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data)) return false;
  std::cout << "pack ok" << std::endl;

  char* message = new char[64 * 1024];
  FlattenToSlow(req_bin_data, message, req_bin_data.ByteSize());
  int send_size = socket.Send(message, req_bin_data.ByteSize());
  if (send_size != static_cast<int>(req_bin_data.ByteSize())) return false;

  char recvbuf[64 * 1024] = {0};
  int recv_size = socket.Recv(recvbuf, sizeof(recvbuf));
  if (recv_size <= 0) return false;
  NoncontiguousBuffer rsp_bin_data = CreateBufferSlow(recvbuf, recv_size);

  trpc::test::helloworld::HelloReply hello_rsp;
  if (!UnPackTrpcResponse(rsp_bin_data, req_data, &hello_rsp)) return false;

  if (hello_rsp.msg() != hello_req.msg()) return false;
  return true;
}
}  // namespace

namespace {
bool TestUnarySayHelloOverHttp() {
  std::string url = "http://127.0.0.1:13454/trpc.test.helloworld.Greeter/SayHello";

  auto http_client = std::make_shared<TestHttpClient>();

  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("hello world");
  auto serialization = serialization::SerializationFactory::GetInstance()->Get(serialization::kPbType);
  NoncontiguousBuffer req_body_buffer;
  if (!serialization->Serialize(serialization::kPbMessage, &hello_req, &req_body_buffer)) return false;
  std::string req_body = trpc::FlattenSlow(req_body_buffer);
  std::cout << "request body size: " << req_body.size() << std::endl;

  trpc::http::Request req;
  req.SetMethod("POST");
  req.SetUrl(url);
  req.SetHeader("Content-Type", "application/pb");
  req.SetContent(req_body);

  trpc::http::Response rsp;
  trpc::Status status = http_client->Do(req, &rsp);
  std::cout << "status: " << status.ToString() << std::endl;
  if (!status.OK()) return false;
  std::cout << "POST, url:" << url << ", response:["
            << "response code:" << rsp.GetStatus() << ", body size:" << rsp.GetContent().size() << "]" << std::endl;

  NoncontiguousBuffer rsp_body = trpc::CreateBufferSlow(rsp.GetContent());
  trpc::test::helloworld::HelloRequest hello_rsp;
  if (!serialization->Deserialize(&rsp_body, serialization::kPbMessage, &hello_rsp)) return false;
  if (hello_req.msg() != hello_rsp.msg()) return false;
  return true;
}
}  // namespace

class TrpcServerTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    std::string config_path = "trpc/server/testing/server_fiber.yaml";
#ifdef TRPC_BUILD_INCLUDE_SSL
    ASSERT_TRUE(ssl::InitOpenSsl());
    config_path = "trpc/server/testing/server_fiber_with_ssl.yaml";
#endif
    ASSERT_TRUE(Server::SetUp(config_path));
  }

  static void TearDownTestCase() {
    Server::TearDown();
#ifdef TRPC_BUILD_INCLUDE_SSL
    ssl::DestroyOpenSsl();
#endif
  }

  class Server : public TestFiberServer {
    bool Init() override {
      // Server.
      ServicePtr stream_greeter = std::make_shared<trpc::test::helloworld::TestStreamGreeter>();
      if (!RegisterService("trpc.test.helloworld.StreamGreeter", stream_greeter)) {
        std::cerr << "failed to register service" << std::endl;
        return false;
      }

      ServicePtr greeter = std::make_shared<trpc::test::helloworld::GreeterServiceTest>();
      if (!RegisterService("trpc.test.helloworld.Greeter", greeter)) {
        std::cerr << "failed to register service" << std::endl;
        return false;
      }

      ServicePtr greeter_over_http = std::make_shared<trpc::test::helloworld::GreeterServiceTest>();
      if (!RegisterService("trpc.test.helloworld.GreeterOverHTTP", greeter_over_http)) {
        std::cerr << "failed to register service" << std::endl;
        return false;
      }

      auto http_service = std::make_shared<trpc::HttpService>();
      http_service->SetRoutes(SetHttpRoutes);
      trpc::ServicePtr default_http_service{http_service};
      if (!RegisterService("http.test.helloworld", default_http_service)) {
        std::cerr << "failed to register service" << std::endl;
        return false;
      }

      std::vector<testing_args_t> testings{
          {"unary call", []() { return TestUnarySayHello(); }, false},
          {"client stream func not found", []() { return !TestClientStreamFuncNotFound(); }, false},
          {"client stream", []() { return TestClientStreamSayHello(); }, false},
          {"server stream", []() { return TestServerStreamSayHello(); }, false},
          {"bidi stream", []() { return TestBidiStreamSayHello(); }, false},
          {"rpc over http", []() { return TestUnarySayHelloOverHttp(); }, false},
          {"http unary call(http service)", []() { return TestHttpUnaryCall("127.0.0.1", 13452); }, false},
      };

      AddTestings(std::move(testings));
      return true;
    }

    void Destroy() override {}
  };
};

TEST_F(TrpcServerTest, ServerStartAndServe) {
  auto server = std::make_shared<TrpcServerTest::Server>();
  bool ok = server->Run();
  ASSERT_TRUE(ok);
}

}  // namespace trpc::testing
