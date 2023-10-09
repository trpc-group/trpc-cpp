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

#include "trpc/server/testing/http_service_testing.h"

#include <chrono>
#include <thread>

#include "trpc/server/testing/http_client_testing.h"
#include "trpc/util/http/function_handlers.h"
#include "trpc/util/http/http_handler_groups.h"

namespace trpc::testing {

trpc::Status SayHello(trpc::ServerContextPtr ctx, trpc::http::RequestPtr req, trpc::http::Response* rsp) {
  std::cout << "method: " << req->GetMethod() << ", request uri: " << req->GetUrl()
            << ", content of request: " << req->GetContent() << std::endl;
  (void)ctx;
  if (req->GetContent().empty()) {
    rsp->SetContent(R"({"msg": "hello world!", "age": 18})");
  } else {
    rsp->SetContent(req->GetContent());
  }
  return trpc::kSuccStatus;
}

trpc::Status SayHelloWithDelay(trpc::ServerContextPtr ctx, trpc::http::RequestPtr req, trpc::http::Response* rsp) {
  std::cout << "method: " << req->GetMethod() << ", request uri: " << req->GetUrl()
            << ", content of request: " << req->GetContent() << std::endl;
  (void)ctx;
  if (req->GetContent().empty()) {
    rsp->SetContent(R"({"msg": "hello world!", "age": 18})");
  } else {
    rsp->SetContent(req->GetContent());
  }
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(1000ms);
  return trpc::kSuccStatus;
}

void SetHttpRoutes(trpc::http::HttpRoutes& r) {
  auto greeter_handler = std::make_shared<trpc::http::FuncHandler>(SayHello, "json");
  // Matches exactly.
  r.Add(trpc::http::MethodType::GET, trpc::http::Path("/hello"), greeter_handler);
  r.Add(trpc::http::MethodType::POST, trpc::http::Path("/hello"), greeter_handler);
  // Matches prefix.
  r.Add(trpc::http::MethodType::POST, trpc::http::Path("/hi").Remainder("path"), greeter_handler);

  auto delay_greeter_handler = std::make_shared<trpc::http::FuncHandler>(SayHelloWithDelay, "json");
  r.Add(trpc::http::MethodType::POST, trpc::http::Path("/hello_delay"), delay_greeter_handler);

  auto foo_handler = std::make_shared<TestHttpFooHandler>();
  r.Add(trpc::http::MethodType::GET, trpc::http::Path("/foo"), foo_handler);
  r.Add(trpc::http::MethodType::HEAD, trpc::http::Path("/foo"), foo_handler);
  r.Add(trpc::http::MethodType::POST, trpc::http::Path("/foo"), foo_handler);

  auto bar_handler = std::make_shared<TestHttpBarHandler>();
  r.Add(trpc::http::MethodType::GET, trpc::http::Path("/bar").Remainder("path"), bar_handler);
  r.Add(trpc::http::MethodType::HEAD, trpc::http::Path("/bar").Remainder("path"), bar_handler);
  r.Add(trpc::http::MethodType::POST, trpc::http::Path("/bar").Remainder("path"), bar_handler);

  auto stream_handler = std::make_shared<TestHttpStreamHandler>();
  r.Add(trpc::http::MethodType::GET, trpc::http::Path("/stream/download"), stream_handler);
  r.Add(trpc::http::MethodType::POST, trpc::http::Path("/stream/upload"), stream_handler);
  r.Add(trpc::http::MethodType::PUT, trpc::http::Path("/stream/upload-reset-by-peer"), stream_handler);

  //  // clang-format off
  //  trpc::http::HttpHandlerGroups(r).Path("/", TRPC_HTTP_ROUTE_HANDLER({
  //    Path("/foo", TRPC_ROUTE_HANDLER({
  //      auto foo_handler = std::make_shared<TestHttpFooHandler>();
  //      Get(foo_handler);
  //      Head(foo_handler);
  //      Post(foo_handler);
  //    }));
  //    Path("/bar", TRPC_ROUTE_HANDLER({
  //      auto bar_handler = std::make_shared<TestHttpBarHandler>();
  //      Get(bar_handler);
  //      Head(bar_handler);
  //      Post(bar_handler);
  //    }));
  //  }));
  // clang-format on
}

trpc::Status TestHttpFooHandler::Get(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                                     trpc::http::Response* rsp) {
  std::cout << "method: " << req->GetMethod() << ", request uri: " << req->GetUrl()
            << ", content of request: " << req->GetContent() << std::endl;
  rsp->SetContent(greetings_);
  return trpc::kSuccStatus;
}

trpc::Status TestHttpFooHandler::Head(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                                      trpc::http::Response* rsp) {
  std::cout << "method: " << req->GetMethod() << ", request uri: " << req->GetUrl()
            << ", content of request: " << req->GetContent() << std::endl;
  rsp->SetHeader("Content-Length", std::to_string(greetings_.size()));
  return trpc::kSuccStatus;
}
trpc::Status TestHttpFooHandler::Post(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                                      trpc::http::Response* rsp) {
  std::cout << "method: " << req->GetMethod() << ", request uri: " << req->GetUrl()
            << ", content of request: " << req->GetContent() << std::endl;
  rsp->SetContent(req->GetContent());
  return trpc::kSuccStatus;
}

trpc::Status TestHttpBarHandler::Handle(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                                        trpc::http::Response* rsp) {
  std::cout << "method: " << req->GetMethod() << ", request uri: " << req->GetUrl()
            << ", content of request: " << req->GetContent() << std::endl;
  if (req->GetMethodType() == trpc::http::MethodType::HEAD) {
    rsp->SetHeader("Content-Length", std::to_string(greetings_.size()));
  } else if (req->GetMethodType() == trpc::http::MethodType::GET) {
    rsp->SetContent(greetings_);
  } else if (req->GetMethodType() == trpc::http::MethodType::POST) {
    rsp->SetContent(req->GetContent());
  } else {
    rsp->SetStatus(trpc::http::ResponseStatus::kNotImplemented);
  }
  return trpc::kSuccStatus;
}

trpc::Status TestHttpStreamHandler::Get(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                                        trpc::http::Response* rsp) {
  std::cout << "method: " << req->GetMethod() << ", request uri: " << req->GetUrl() << std::endl;
  // Sends response body in chunked.
  rsp->SetHeader(trpc::http::kHeaderTransferEncoding, trpc::http::kTransferEncodingChunked);

  auto& writer = rsp->GetStream();
  // Sends response header.
  trpc::Status status = writer.WriteHeader();
  if (!status.OK()) {
    std::cerr << "failed to write http response header: " << status.ToString() << std::endl;
    return trpc::kStreamRstStatus;
  }

  // Sends response content.
  std::string hello{"http, hello world!"};
  std::size_t nwrite{0};
  const std::size_t kMBs{2 * 1024 * 1024};
  for (;;) {
    NoncontiguousBuffer buffer = trpc::CreateBufferSlow(hello);
    nwrite += buffer.ByteSize();
    status = writer.Write(std::move(buffer));
    if (!status.OK()) {
      std::cerr << "failed to write http response content: " << status.ToString() << std::endl;
      return trpc::kStreamRstStatus;
    }
    // EOF.
    if (nwrite >= kMBs) {
      break;
    }
  }

  // Sends EOF.
  status = writer.WriteDone();
  if (!status.OK()) {
    std::cerr << "failed to write http response EOF: " << status.ToString() << std::endl;
    return trpc::kStreamRstStatus;
  }
  std::cout << "finish writing http response, bytes of written content:" << nwrite << std::endl;
  return kSuccStatus;
}

trpc::Status TestHttpStreamHandler::Post(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                                         trpc::http::Response* rsp) {
  std::cout << "method: " << req->GetMethod() << ", request uri: " << req->GetUrl() << std::endl;
  auto& writer = rsp->GetStream();
  // Sends response header.
  trpc::Status status = writer.WriteHeader();
  if (!status.OK()) {
    std::cerr << "failed to write http response header: " << status.ToString() << std::endl;
    return trpc::kStreamRstStatus;
  }

  auto& reader = req->GetStream();
  const std::size_t kMaxPieceSize{1024 * 1024};
  std::size_t nread{0};
  for (;;) {
    trpc::NoncontiguousBuffer buffer;
    status = reader.Read(buffer, kMaxPieceSize);
    if (!status.OK() && !status.StreamEof()) {
      std::cerr << "failed to read http request content: " << status.ToString() << std::endl;
      return trpc::kStreamRstStatus;
    }
    nread += buffer.ByteSize();
    if (status.StreamEof()) {
      std::cout << "finish reading http request content, bytes of read content:" << nread << std::endl;
      break;
    }
  }
  return kSuccStatus;
}

trpc::Status TestHttpStreamHandler::Put(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                                        trpc::http::Response* rsp) {
  std::cout << "method: " << req->GetMethod() << ", request uri: " << req->GetUrl() << std::endl;
  auto& writer = rsp->GetStream();
  // Sends response header.
  trpc::Status status = writer.WriteHeader();
  if (!status.OK()) {
    std::cerr << "failed to write http response header: " << status.ToString() << std::endl;
    return trpc::kStreamRstStatus;
  }
  // Error, reset the stream.
  return trpc::kStreamRstStatus;
}

namespace {
bool TestSayHello(const std::string& ip, int port) {
  auto http_client = std::make_shared<TestHttpClient>();
  http_client->SetInsecure(true);

  std::string url = R"(http://)" + ip + R"(:)" + std::to_string(port) + R"(/hello)";
#ifdef TRPC_BUILD_INCLUDE_SSL
  url = R"(https://)" + ip + R"(:)" + std::to_string(port) + R"(/hello)";
#endif

  std::string body = R"({"greetings":"Hello World !"})";
  trpc::http::Request req;
  req.SetMethod("POST");
  req.SetUrl(url);
  req.SetContent(body);

  trpc::http::Response rsp;
  trpc::Status status = http_client->Do(req, &rsp);
  std::cout << "status: " << status.ToString() << std::endl;
  if (!status.OK()) return false;
  std::cout << "POST, url:" << url << ", response:["
            << "response code:" << rsp.GetStatus() << ", body size:" << rsp.GetContent().size() << "]" << std::endl;
  bool ok = (rsp.GetContent() == req.GetContent());
  return ok;
}
}  // namespace

namespace {
bool TestFoo(const std::string& ip, int port) {
  auto http_client = std::make_shared<TestHttpClient>();
  http_client->SetInsecure(true);

  std::string url = R"(http://)" + ip + R"(:)" + std::to_string(port) + R"(/foo)";
#ifdef TRPC_BUILD_INCLUDE_SSL
  url = R"(https://)" + ip + R"(:)" + std::to_string(port) + R"(/foo)";
#endif

  std::string body = R"({"greetings":"Hello World !"})";
  trpc::http::Request req;
  req.SetMethod("POST");
  req.SetUrl(url);
  req.SetContent(body);

  trpc::http::Response rsp;
  trpc::Status status = http_client->Do(req, &rsp);
  std::cout << "status: " << status.ToString() << std::endl;
  if (!status.OK()) return false;
  std::cout << "POST, url:" << url << ", response:["
            << "response code:" << rsp.GetStatus() << ", body size:" << rsp.GetContent().size() << "]" << std::endl;
  bool ok = (rsp.GetContent() == req.GetContent());

  // GET
  req.SetMethod("GET");
  req.SetUrl(url);
  req.SetContent("");
  status = http_client->Do(req, &rsp);
  if (!status.OK()) return false;
  ok &= (rsp.GetContent() == "hello world!");

  return ok;
}
}  // namespace

namespace {
bool TestBar(const std::string& ip, int port) {
  auto http_client = std::make_shared<TestHttpClient>();
  http_client->SetInsecure(true);

  std::string url = R"(http://)" + ip + R"(:)" + std::to_string(port) + R"(/bar)";
#ifdef TRPC_BUILD_INCLUDE_SSL
  url = R"(https://)" + ip + R"(:)" + std::to_string(port) + R"(/bar)";
#endif

  std::string body = R"({"greetings":"Hello World !"})";
  trpc::http::Request req;
  req.SetMethod("POST");
  req.SetUrl(url);
  req.SetContent(body);

  trpc::http::Response rsp;
  trpc::Status status = http_client->Do(req, &rsp);
  std::cout << "status: " << status.ToString() << std::endl;
  if (!status.OK()) return false;
  std::cout << "POST, url:" << url << ", response:["
            << "response code:" << rsp.GetStatus() << ", body size:" << rsp.GetContent().size() << "]" << std::endl;
  bool ok = (rsp.GetContent() == req.GetContent());

  // GET
  url = R"(http://)" + ip + R"(:)" + std::to_string(port) + R"(/bar/xx)";
#ifdef TRPC_BUILD_INCLUDE_SSL
  url = R"(https://)" + ip + R"(:)" + std::to_string(port) + R"(/bar/xx)";
#endif

  req.SetMethod("GET");
  req.SetUrl(url);
  req.SetContent("");
  status = http_client->Do(req, &rsp);
  if (!status.OK()) return false;
  std::cout << "GET, url:" << url << ", response:["
            << "response code:" << rsp.GetStatus() << ", body size:" << rsp.GetContent().size() << "]" << std::endl;
  ok &= (rsp.GetContent() == "hello world!");

  return ok;
}
}  // namespace

bool TestHttpUnaryCall(const std::string& ip, int port) {
  bool ok{true};

  ok &= TestSayHello(ip, port);
  ok &= TestFoo(ip, port);
  ok &= TestBar(ip, port);

  return ok;
}

}  // namespace trpc::testing
