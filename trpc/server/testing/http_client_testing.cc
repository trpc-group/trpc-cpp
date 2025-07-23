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

#include "trpc/server/testing/http_client_testing.h"

#include "trpc/util/http/http_parser.h"

namespace trpc::testing {

TestHttpClient::TestHttpClient() {
#ifdef TRPC_BUILD_INCLUDE_SSL
  ssl_options_.ciphers = trpc::ssl::GetDefaultCiphers();

  std::vector<std::string> protocols{"TLSv1.1", "TLSv1.2", "TLSv1.3"};
  ssl_options_.protocols = trpc::ssl::ParseProtocols(protocols);
#endif
}

trpc::Status TestHttpClient::Do(const trpc::http::Request& req, trpc::http::Response* rsp) {
  trpc::http::UrlParser url_parser(req.GetUrl());

  std::string scheme = url_parser.Scheme();
  int port = 80;
  if (!url_parser.Port().empty()) {
    port = url_parser.IntegerPort();
  } else {
    if (scheme == "https") {
      port = 443;
    }
  }

  StreamTransportPtr transport{nullptr};

#ifdef TRPC_BUILD_INCLUDE_SSL
  if (scheme == "https") {
    // Init SSL context
    ssl::SslContextPtr ssl_ctx = MakeRefCounted<ssl::SslContext>();
    if (!ssl_ctx->Init(ssl_options_)) return trpc::Status{-1, -1, "failed to init ssl context"};
    transport = CreateTransport(ssl_ctx, url_parser.Hostname(), port);
    if (transport == nullptr) return trpc::Status{-1, -1, "failed to create ssl transport"};
  }
#endif

  if (transport == nullptr) {
    transport = CreateTransport(url_parser.Hostname(), port);
  }

  if (transport == nullptr) return trpc::Status{-1, -1, "failed to create tcp socket"};

  auto http_req = req;
  http_req.SetUrl(url_parser.RequestUri());
  http_req.SetVersion("1.1");
  http_req.SetHeaderIfNotPresent("Content-Length", std::to_string(http_req.GetContent().size()));

  trpc::NoncontiguousBufferBuilder builder;
  if (!http_req.SerializeToString(builder)) return trpc::Status{-1, -1, "encode error"};
  NoncontiguousBuffer send_buff = builder.DestructiveGet();
  if (transport->Send(std::move(send_buff)) <= 0) return trpc::Status{-1, -1, "network send error"};
  if (!Recv(transport, *rsp)) return trpc::Status{-1, -1, "network recv error"};
  return trpc::kSuccStatus;
}

TestHttpClient::StreamTransportPtr TestHttpClient::CreateTransport(const std::string& ip, int port) {
  trpc::Socket socket;
  if (!CreateTcpSocket(ip, port, false, &socket)) return nullptr;
  return std::make_shared<TestStreamTransportImpl>(std::move(socket));
}

#ifdef TRPC_BUILD_INCLUDE_SSL
TestHttpClient::StreamTransportPtr TestHttpClient::CreateTransport(const trpc::ssl::SslContextPtr& ssl_ctx,
                                                                   const std::string& ip, int port) {
  trpc::Socket socket;
  if (!CreateTcpSocket(ip, port, false, &socket)) return nullptr;
  trpc::ssl::SslPtr ssl = ssl_ctx->NewSsl();
  if (ssl == nullptr) return nullptr;
  // Mount fd of connection to ssl
  if (socket.GetFd() >= 0) {
    if (!ssl->SetFd(socket.GetFd())) return nullptr;
  }
  // Set SNI server_name
  if (!ssl_options_.sni_name.empty()) {
    if (!ssl->SetTlsExtensionServerName(ssl_options_.sni_name)) return nullptr;
  }
  // Set ssl to work in client mode
  ssl->SetConnectState();
  auto transport = std::make_shared<TestSslStreamTransportImpl>(std::move(socket), std::move(ssl));
  for (;;) {
    using namespace std::chrono_literals;
    int ok = transport->Handshake();
    if (ok == 0) {
      break;
    } else if (ok == -2) {
      std::this_thread::sleep_for(20ms);
      continue;
    }
    std::cerr << "ssl handshake error" << std::endl;
    return nullptr;
  }
  return transport;
}
#endif

bool TestHttpClient::Recv(const StreamTransportPtr& transport, trpc::http::Response& rsp) {
  if (recv_msgs_.empty()) {
    for (;;) {
      using namespace std::chrono_literals;
      std::cerr << "recv http response" << std::endl;
      std::this_thread::sleep_for(20ms);

      if (transport->Recv(recv_buff_) <= 0) {
        std::cerr << "transport recv error" << std::endl;
        return false;
      }
      std::string buff = trpc::FlattenSlow(recv_buff_);
      int check_ret = trpc::http::Parse(buff, &recv_msgs_);
      if (check_ret == -1) {
        std::cerr << "check http response error" << std::endl;
        return false;
      } else if (check_ret == 0) {
        continue;
      } else if (check_ret > 0) {
        recv_buff_.Skip(check_ret);
      }

      if (recv_msgs_.size() >= 1) break;
    }
  }
  rsp = std::move(recv_msgs_.front());
  recv_msgs_.pop_front();
  return true;
};

}  // namespace trpc::testing
