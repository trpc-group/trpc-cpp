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

#pragma once

#include <deque>

#include "trpc/common/status.h"
#include "trpc/runtime/iomodel/reactor/common/socket.h"
#include "trpc/server/testing/stream_transport_testing.h"
#ifdef TRPC_BUILD_INCLUDE_SSL
#include "trpc/transport/common/ssl/ssl.h"
#endif
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"

namespace trpc::testing {

class TestHttpClient {
 public:
  TestHttpClient();

  trpc::Status Do(const trpc::http::Request& req, trpc::http::Response* rsp);

  void SetInsecure(bool insecure) {
#ifdef TRPC_BUILD_INCLUDE_SSL
    ssl_options_.insecure = insecure;
#endif
  }
  void SetSniName(std::string sni_name) {
#ifdef TRPC_BUILD_INCLUDE_SSL
    ssl_options_.sni_name = std::move(sni_name);
#endif
  }

 private:
  using StreamTransportPtr = std::shared_ptr<TestStreamTransport>;

  StreamTransportPtr CreateTransport(const std::string& ip, int port);
  bool Recv(const StreamTransportPtr& transport, trpc::http::Response& rsp);

#ifdef TRPC_BUILD_INCLUDE_SSL
  StreamTransportPtr CreateTransport(const trpc::ssl::SslContextPtr& ssl_ctx, const std::string& ip, int port);
#endif

 private:
  trpc::NoncontiguousBuffer recv_buff_;
  std::deque<trpc::http::Response> recv_msgs_;
  StreamTransportPtr transport{nullptr};

#ifdef TRPC_BUILD_INCLUDE_SSL
  trpc::ssl::ClientSslOptions ssl_options_;
#endif
};

}  // namespace trpc::testing
