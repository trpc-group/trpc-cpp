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

#include "trpc/client/http/testing/http_client_call_testing.h"

#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "trpc/client/make_client_context.h"
#include "trpc/proto/testing/helloworld.pb.h"

namespace trpc::testing {

namespace {
constexpr std::string_view kGreetings{"hello world!"};
}  // namespace

namespace {
//// HTTP Unary RPC call.

// PB message.
bool UnaryRpcCallWithPb(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  ctx->SetFuncName("/trpc.test.helloworld.Greeter/SayHello");
  trpc::test::helloworld::HelloRequest hello_req;
  trpc::test::helloworld::HelloRequest hello_rsp;

  hello_req.set_msg("hello world!");
  auto status = proxy->UnaryInvoke<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloRequest>(
      ctx, hello_req, &hello_rsp);
  if (!status.OK()) {
    std::cerr << "status: " << status.ToString() << std::endl;
    return false;
  }
  std::cout << "response content: " << hello_rsp.msg() << std::endl;
  if (hello_rsp.msg() != kGreetings) return false;
  return true;
}

// JSON message.(FIXME: Content-Type: application/json)
// bool UnaryRpcCallWithJson(const HttpServiceProxyPtr& proxy) {
//  auto ctx = MakeClientContext(proxy);
//  ctx->SetFuncName("/trpc.test.helloworld.Greeter/SayHello");
//  rapidjson::Document req_json;
//  rapidjson::Document rsp_json;
//
//  auto req_str = R"({"msg": "hello world!"})";
//  req_json.Parse(req_str);
//  auto status = proxy->UnaryInvoke<rapidjson::Document, rapidjson::Document>(ctx, req_json, &rsp_json);
//  if (!status.OK()) {
//    std::cerr << "status: " << status.ToString() << std::endl;
//    return false;
//  }
//  rapidjson::StringBuffer buffer;
//  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
//  rsp_json.Accept(writer);
//  auto rsp_str = buffer.GetString();
//  std::cout << "response content: " << rsp_str << std::endl;
//  if (rsp_str != req_str) return false;
//  return true;
//}

// JSON message.
bool UnaryRpcCallWithPostingJson(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  rapidjson::Document req_json;
  req_json.Parse(R"({"msg": "hello world!"})");
  rapidjson::Document rsp_json;
  auto status = proxy->Post(ctx, "http://example.com/trpc.test.helloworld.Greeter/SayHello", req_json, &rsp_json);
  if (!status.OK()) {
    std::cerr << "status: " << status.ToString() << std::endl;
    return false;
  }
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  rsp_json.Accept(writer);
  auto rsp_str = buffer.GetString();
  std::cout << "response content: " << rsp_str << std::endl;
  if (req_json["msg"] != rsp_json["msg"]) {
    return false;
  }
  return true;
}

bool UnaryRpcCallNotFound(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  ctx->SetFuncName("/trpc.test.helloworld.Greeter/SayHello-Not-Found");
  trpc::test::helloworld::HelloRequest hello_req;
  trpc::test::helloworld::HelloRequest hello_rsp;

  hello_req.set_msg("hello world!");
  auto status = proxy->UnaryInvoke<trpc::test::helloworld::HelloRequest, trpc::test::helloworld::HelloRequest>(
      ctx, hello_req, &hello_rsp);
  if (!status.OK() && status.GetFrameworkRetCode() == trpc::TrpcRetCode::TRPC_SERVER_NOFUNC_ERR) {
    std::cerr << "status: " << status.ToString() << std::endl;
    return true;
  }
  return false;
}

bool TestHttpUnaryRpcCall(const HttpServiceProxyPtr& proxy) {
  bool ok{true};
  ok &= UnaryRpcCallWithPb(proxy);
  ok &= UnaryRpcCallWithPostingJson(proxy);
  ok &= UnaryRpcCallNotFound(proxy);
  return ok;
}
}  // namespace

namespace {
//// HTTP GET.

bool GetString(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  std::string rsp_str;
  auto status = proxy->GetString(ctx, "http://example.com/foo", &rsp_str);
  if (!status.OK()) {
    std::cerr << "status: " << status.ToString() << std::endl;
    return false;
  }
  std::cout << "response content: " << rsp_str << std::endl;
  if (rsp_str != kGreetings) return false;
  return true;
}

bool GetJson(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  rapidjson::Document rsp_json;
  auto status = proxy->Get(ctx, "http://example.com/hello", &rsp_json);
  if (!status.OK()) {
    std::cerr << "status: " << status.ToString() << std::endl;
    return false;
  }
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  rsp_json.Accept(writer);
  auto rsp_str = buffer.GetString();
  std::cout << "response content: " << rsp_str << std::endl;
  if (rsp_json["msg"].GetString() == kGreetings && rsp_json["age"].GetInt() == 18) {
    return true;
  }
  return false;
}

bool GetHttpResponse(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  http::HttpResponse http_rsp;
  auto status = proxy->Get2(ctx, "http://example.com/foo", &http_rsp);
  if (!status.OK()) {
    std::cerr << "status: " << status.ToString() << std::endl;
    return false;
  }
  std::cout << "response content: " << http_rsp.GetContent() << std::endl;
  if (http_rsp.GetContent() != kGreetings) return false;
  return true;
}

bool GetHttpResponseNotFound(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  http::HttpResponse http_rsp;
  auto status = proxy->Get2(ctx, "http://example.com/foo-not-found", &http_rsp);
  if (!status.OK() && status.GetFuncRetCode() == 404) {
    std::cerr << "status: " << status.ToString() << std::endl;
    return true;
  }
  return false;
}

bool TestHttpGet(const HttpServiceProxyPtr& proxy) {
  bool ok{true};
  ok &= GetString(proxy);
  ok &= GetJson(proxy);
  ok &= GetHttpResponse(proxy);
  ok &= GetHttpResponseNotFound(proxy);
  return ok;
}
}  // namespace

namespace {
//// HTTP HEAD.

bool HeadHttpResponse(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  http::HttpResponse http_rsp;
  auto status = proxy->Head(ctx, "http://example.com/foo", &http_rsp);
  if (!status.OK()) {
    std::cerr << "status: " << status.ToString() << std::endl;
    return false;
  }
  std::cout << "response content: " << http_rsp.GetContent() << std::endl;
  if (!http_rsp.GetContent().empty()) return false;
  if (http_rsp.GetHeader("Content-Length") != std::to_string(kGreetings.size())) return false;
  return true;
}

bool TestHttpHead(const HttpServiceProxyPtr& proxy) {
  bool ok{true};
  ok &= HeadHttpResponse(proxy);
  return ok;
}
}  // namespace

namespace {
//// HTTP POST.

bool PostString(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  std::string req_str{kGreetings};
  std::string rsp_str;
  auto status = proxy->Post(ctx, "http://example.com/foo", std::move(req_str), &rsp_str);
  if (!status.OK()) {
    std::cerr << "status: " << status.ToString() << std::endl;
    return false;
  }
  std::cout << "response content: " << rsp_str << std::endl;
  if (rsp_str != kGreetings) return false;
  return true;
}

bool PostJson(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  rapidjson::Document req_json;
  req_json.Parse(R"({"msg": "hello world!"})");
  rapidjson::Document rsp_json;
  auto status = proxy->Post(ctx, "http://example.com/foo", req_json, &rsp_json);
  if (!status.OK()) {
    std::cerr << "status: " << status.ToString() << std::endl;
    return false;
  }
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  rsp_json.Accept(writer);
  auto rsp_str = buffer.GetString();
  std::cout << "response content: " << rsp_str << std::endl;
  if (req_json["msg"] != rsp_json["msg"]) {
    return false;
  }
  return true;
}

bool PostStringAndWaitHttpResponse(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  std::string req_str{kGreetings};
  http::HttpResponse http_rsp;
  auto status = proxy->Post2(ctx, "http://example.com/foo", std::move(req_str), &http_rsp);
  if (!status.OK()) {
    std::cerr << "status: " << status.ToString() << std::endl;
    return false;
  }
  std::cout << "response content: " << http_rsp.GetContent() << std::endl;
  if (http_rsp.GetContent() != kGreetings) return false;
  return true;
}

bool TestHttpPost(const HttpServiceProxyPtr& proxy) {
  bool ok{true};
  ok &= PostString(proxy);
  ok &= PostJson(proxy);
  ok &= PostStringAndWaitHttpResponse(proxy);
  return ok;
}
}  // namespace

namespace {
//// HTTP unary invoke.

bool HttpUnaryInvoke(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  http::HttpRequest http_req;
  http_req.SetMethod("POST");
  http_req.SetUrl("/foo");

  std::string req_content = R"({"msg": "hello world!"})";
  http_req.SetHeader("Content-Type", "application/json");
  http_req.SetHeader("Content-Length", std::to_string(req_content.size()));
  http_req.SetContent(req_content);

  http::HttpResponse http_rsp;
  auto status = proxy->HttpUnaryInvoke<trpc::http::HttpRequest, trpc::http::HttpResponse>(ctx, http_req, &http_rsp);
  if (!status.OK()) {
    std::cerr << "status: " << status.ToString() << std::endl;
    return false;
  }
  std::cout << "response content: " << http_rsp.GetContent() << std::endl;
  if (http_rsp.GetContent() != req_content) return false;
  return true;
}

bool HttpUnaryInvokeTimeout(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  ctx->SetTimeout(500);
  http::HttpRequest http_req;
  http_req.SetMethod("POST");
  // Request URI.
  http_req.SetUrl("/hello_delay");

  std::string req_content = R"({"msg": "hello world!"})";
  http_req.SetHeader("Content-Type", "application/json");
  http_req.SetHeader("Content-Length", std::to_string(req_content.size()));
  http_req.SetContent(req_content);

  http::HttpResponse http_rsp;
  auto status = proxy->HttpUnaryInvoke<trpc::http::HttpRequest, trpc::http::HttpResponse>(ctx, http_req, &http_rsp);
  if (!status.OK() && status.GetFrameworkRetCode() == trpc::TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR) {
    std::cerr << "status timeout: " << status.ToString() << std::endl;
    return true;
  }
  return false;
}

bool TestHttpUnaryInvoke(const HttpServiceProxyPtr& proxy) {
  bool ok{true};
  ok &= HttpUnaryInvoke(proxy);
  ok &= HttpUnaryInvokeTimeout(proxy);
  return ok;
}
}  // namespace

namespace {
//// HTTP streaming call.

bool TestHttpStreamingDownload(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  ctx->SetTimeout(5000);
  trpc::stream::HttpClientStreamReaderWriter stream = proxy->Get(ctx, "http://example.com/stream/download");
  if (!stream.GetStatus().OK()) {
    std::cerr << "failed to create http client stream" << std::endl;
    return false;
  }

  int http_status{0};
  trpc::http::HttpHeader http_header;
  trpc::Status status = stream.ReadHeaders(http_status, http_header);
  if (!status.OK()) {
    std::cerr << "failed to http status: " << status.ToString() << std::endl;
    return false;
  } else if (http_status != trpc::http::ResponseStatus::kOk) {
    std::cerr << "bad http status: " << http_status << std::endl;
    return false;
  }

  const std::size_t kMaxPieceSize{1024 * 1024};
  std::size_t nread{0};
  for (;;) {
    trpc::NoncontiguousBuffer buffer;
    trpc::Status status = stream.Read(buffer, kMaxPieceSize);
    if (!status.OK() && !status.StreamEof()) {
      std::cerr << "failed to read http response content: " << status.ToString() << std::endl;
      return false;
      break;
    }
    nread += buffer.ByteSize();
    if (status.StreamEof()) {
      std::cout << "finish reading http response content, bytes of read content:" << nread << std::endl;
      break;
    }
  }
  // Checks response content bytes.
  const std::size_t kResponseContentSize{2 * 1024 * 1024};
  if (nread < kResponseContentSize) return false;

  return true;
}

bool TestHttpStreamingUploadWithContentLength(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  ctx->SetTimeout(5000);

  std::string hello{"http, hello world!"};
  std::size_t kRequestContentSize{hello.size() * 1024 * 1024 / 8};
  ctx->SetHttpHeader(trpc::http::kHeaderContentLength, std::to_string(kRequestContentSize));

  trpc::stream::HttpClientStreamReaderWriter stream = proxy->Post(ctx, "http://example.com/stream/upload");
  if (!stream.GetStatus().OK()) {
    std::cerr << "failed to create http client stream" << std::endl;
    return false;
  }

  int http_status{0};
  trpc::http::HttpHeader http_header;
  trpc::Status status = stream.ReadHeaders(http_status, http_header);
  if (!status.OK()) {
    std::cerr << "failed to http status: " << status.ToString() << std::endl;
    return false;
  } else if (http_status != trpc::http::ResponseStatus::kOk) {
    std::cerr << "bad http status: " << http_status << std::endl;
    return false;
  }

  std::size_t nwrite{0};
  for (;;) {
    trpc::NoncontiguousBuffer buffer = trpc::CreateBufferSlow(hello);
    nwrite += buffer.ByteSize();
    trpc::Status status = stream.Write(std::move(buffer));
    if (!status.OK()) {
      std::cerr << "failed to read http response content: " << status.ToString() << std::endl;
      return false;
    }
    // EOF.
    if (nwrite >= kRequestContentSize) {
      break;
    }
  }
  // Sends EOF.
  status = stream.WriteDone();
  if (!status.OK()) {
    std::cerr << "failed to write http request EOF: " << status.ToString() << std::endl;
    return false;
  }
  std::cout << "finish writing http request content, bytes of written content:" << nwrite << std::endl;
  if (nwrite < kRequestContentSize) return false;
  return true;
}

bool TestHttpStreamingUploadWithChunked(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  ctx->SetTimeout(5000);

  std::string hello{"http, hello world!"};
  std::size_t kRequestContentSize{hello.size() * 1024 * 1024 / 8};
  ctx->SetHttpHeader(trpc::http::kHeaderTransferEncoding, trpc::http::kTransferEncodingChunked);

  trpc::stream::HttpClientStreamReaderWriter stream = proxy->Post(ctx, "http://example.com/stream/upload");
  if (!stream.GetStatus().OK()) {
    std::cerr << "failed to create http client stream" << std::endl;
    return false;
  }

  int http_status{0};
  trpc::http::HttpHeader http_header;
  trpc::Status status = stream.ReadHeaders(http_status, http_header);
  if (!status.OK()) {
    std::cerr << "failed to http status: " << status.ToString() << std::endl;
    return false;
  } else if (http_status != trpc::http::ResponseStatus::kOk) {
    std::cerr << "bad http status: " << http_status << std::endl;
    return false;
  }

  std::size_t nwrite{0};
  for (;;) {
    trpc::NoncontiguousBuffer buffer = trpc::CreateBufferSlow(hello);
    nwrite += buffer.ByteSize();
    trpc::Status status = stream.Write(std::move(buffer));
    if (!status.OK()) {
      std::cerr << "failed to read http response content: " << status.ToString() << std::endl;
      return false;
    }
    // EOF.
    if (nwrite >= kRequestContentSize) {
      break;
    }
  }
  // Sends EOF.
  status = stream.WriteDone();
  if (!status.OK()) {
    std::cerr << "failed to write http request EOF: " << status.ToString() << std::endl;
    return false;
  }
  std::cout << "finish writing http request content, bytes of written content:" << nwrite << std::endl;
  if (nwrite < kRequestContentSize) return false;

  return true;
}

bool TestHttpStreamingUploadResetByPeer(const HttpServiceProxyPtr& proxy) {
  auto ctx = MakeClientContext(proxy);
  ctx->SetTimeout(5000);

  std::string hello{"http, hello world!"};
  std::size_t kRequestContentSize{hello.size() * 1024 * 1024 / 8};
  ctx->SetHttpHeader(trpc::http::kHeaderTransferEncoding, trpc::http::kTransferEncodingChunked);

  trpc::stream::HttpClientStreamReaderWriter stream = proxy->Put(ctx, "http://example.com/stream/upload-reset-by-peer");
  if (!stream.GetStatus().OK()) {
    std::cerr << "failed to create http client stream" << std::endl;
    return false;
  }

  int http_status{0};
  trpc::http::HttpHeader http_header;
  trpc::Status status = stream.ReadHeaders(http_status, http_header);
  if (!status.OK()) {
    std::cerr << "failed to http status: " << status.ToString() << std::endl;
    return false;
  } else if (http_status != trpc::http::ResponseStatus::kOk) {
    std::cerr << "bad http status: " << http_status << std::endl;
    return false;
  }

  std::size_t nwrite{0};
  for (;;) {
    trpc::NoncontiguousBuffer buffer = trpc::CreateBufferSlow(hello);
    nwrite += buffer.ByteSize();
    trpc::Status status = stream.Write(std::move(buffer));
    if (!status.OK()) {
      std::cerr << "failed to read http response content: " << status.ToString() << std::endl;
      return false;
    }
    // EOF.
    if (nwrite >= kRequestContentSize) {
      break;
    }
  }
  // Sends EOF.
  status = stream.WriteDone();
  if (!status.OK()) {
    std::cerr << "failed to write http request EOF: " << status.ToString() << std::endl;
    return false;
  }
  std::cout << "finish writing http request content, bytes of written content:" << nwrite << std::endl;
  if (nwrite < kRequestContentSize) return false;

  return true;
}

}  // namespace

bool TestHttpClientUnaryRpcCall(const HttpServiceProxyPtr& proxy) { return TestHttpUnaryRpcCall(proxy); }

bool TestHttpClientUnaryCall(const HttpServiceProxyPtr& proxy) {
  bool ok{true};
  ok &= TestHttpGet(proxy);
  ok &= TestHttpHead(proxy);
  ok &= TestHttpPost(proxy);
  ok &= TestHttpUnaryInvoke(proxy);
  return ok;
}

bool TestHttpClientStreamingCall(const HttpServiceProxyPtr& proxy) {
  bool ok{true};
  ok &= TestHttpStreamingDownload(proxy);
  ok &= TestHttpStreamingUploadWithContentLength(proxy);
  ok &= TestHttpStreamingUploadWithChunked(proxy);
  ok &= !TestHttpStreamingUploadResetByPeer(proxy);
  return ok;
}

}  // namespace trpc::testing
