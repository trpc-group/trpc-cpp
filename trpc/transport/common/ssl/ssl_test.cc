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

#ifdef TRPC_BUILD_INCLUDE_SSL
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cassert>
#include <memory>
#include <string>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/server/testing/http_client_testing.h"
#include "trpc/transport/common/ssl/core.h"
#include "trpc/transport/common/ssl/errno.h"
#include "trpc/transport/common/ssl/random.h"
#include "trpc/transport/common/ssl/ssl.h"

namespace trpc::testing {

namespace {
std::string MakeBytes(size_t size) {
  static std::string chars = "0123456789ABCDEF";
  std::random_device rd;
  std::default_random_engine random(rd());
  std::string bytes_buffer;
  for (size_t i = 0; i < size; ++i) {
    bytes_buffer += chars[random() % chars.size()];
  }
  return bytes_buffer;
}
}  // namespace

// For testing purposes only.
using namespace trpc::ssl;
using namespace std::chrono_literals;

class OpenSslTest : public ::testing::Test {
 protected:
  void SetUp() override { assert(InitOpenSsl()); }
  void TearDown() override { DestroyOpenSsl(); }
};

TEST_F(OpenSslTest, ParseProtocols) {
  int protocols_value = kSslSslV2 | kSslSslV3 | kSslTlsV1 | kSslTlsV11 | kSslTlsV12 | kSslTlsV13;
  std::vector<std::string> protocol_list{"SSLv2", "SSLv3", "TLSv1", "TLSv1.1", "TLSv1.2", "TLSv1.3", "xxx"};

  ASSERT_EQ(protocols_value, ParseProtocols(protocol_list));
  protocol_list.emplace_back("unknown version");

  ASSERT_EQ(protocols_value, ParseProtocols(protocol_list));

  protocols_value = kSslTlsV11 | kSslTlsV12;
  protocol_list.clear();
  ASSERT_EQ(protocols_value, ParseProtocols(protocol_list));

  protocol_list.emplace_back("");
  ASSERT_EQ(protocols_value, ParseProtocols(protocol_list));
}

class SslContextTest : public ::testing::Test {
 protected:
  void SetUp() override { InitSslCtx(); }

  void TearDown() override { DestroyOpenSsl(); }

 protected:
  int InitSslCtx() {
    // Init OpenSSL.
    assert(InitOpenSsl());

    // Init options of ssl context.
    InitSslCtxOption();

    return kOk;
  }

  int InitSslCtxOption() {
    server_ssl_options_.default_cert.cert_path = "./trpc/transport/common/ssl/cert/server_cert.pem";
    server_ssl_options_.default_cert.private_key_path = "./trpc/transport/common/ssl/cert/server_key.pem";
    server_ssl_options_.ciphers = GetDefaultCiphers();
    server_ssl_options_.dh_param_path = "./trpc/transport/common/ssl/cert/server_dhparam.pem";
    server_ssl_options_.protocols = kSslTlsV1 | kSslTlsV11 | kSslTlsV12;

    client_ssl_options_.ciphers = GetDefaultCiphers();
    client_ssl_options_.protocols = kSslTlsV1 | kSslTlsV11 | kSslTlsV12;
    client_ssl_options_.dh_param_path = "./trpc/transport/common/ssl/cert/server_dhparam.pem";
    client_ssl_options_.verify_peer_options.ca_cert_path = "./trpc/transport/common/ssl/cert/xxops-com-chain.pem";
    client_ssl_options_.verify_peer_options.verify_depth = 2;
    client_ssl_options_.sni_name = "www.xxops.com";

    return kOk;
  }

 protected:
  ServerSslOptions server_ssl_options_;
  ClientSslOptions client_ssl_options_;
};

TEST_F(SslContextTest, InitAsServerSslContext) {
  SslContextPtr ssl_ctx = MakeRefCounted<SslContext>();
  ASSERT_TRUE(ssl_ctx->Init(server_ssl_options_));

  auto ssl = ssl_ctx->NewSsl();
  ASSERT_TRUE(ssl != nullptr);
}

TEST_F(SslContextTest, InitAsClientSslContext) {
  SslContextPtr ssl_ctx = MakeRefCounted<SslContext>();
  ASSERT_TRUE(ssl_ctx->Init(client_ssl_options_));

  auto ssl = ssl_ctx->NewSsl();
  ASSERT_TRUE(ssl != nullptr);
}

// ---- ~ Delimiter ~ ----

class SslTest : public ::testing::Test {
 public:
  int InitSslCtx() {
    // Init OpenSSL
    assert(InitOpenSsl());

    // Init option of ssl context
    InitSslCtxOption();

    // Init SSL_CTX
    ssl_ctx_ = MakeRefCounted<SslContext>();
    assert(ssl_ctx_->Init(server_ssl_options_));

    return kOk;
  }

  int InitSslCtxOption() {
    std::vector<std::string> protocols{"SSLv3", "TLSv1", "TLSv1.1", "TLSv1.2"};
    server_ssl_options_.default_cert.cert_path = "./trpc/transport/common/ssl/cert/server_cert.pem";
    server_ssl_options_.default_cert.private_key_path = "./trpc/transport/common/ssl/cert/server_key.pem";
    server_ssl_options_.ciphers = GetDefaultCiphers();
    server_ssl_options_.dh_param_path = "./trpc/transport/common/ssl/cert/server_dhparam.pem";
    server_ssl_options_.protocols = ParseProtocols(protocols);
    return kOk;
  }

  int InitListenSocketFd() {
    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr);

    listen_port_ = Random::RandomInt(23001, 24000);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(listen_port_);
    addr.sin_addr.s_addr = inet_addr(listen_addr_.c_str());

    // Create socket
    listen_socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    assert(0 < listen_socket_fd_);

    int nb = 1;
    ioctl(listen_socket_fd_, FIONBIO, &nb);

    // To avoid error `Address already in use`, enable address reuse.
    int on = 1;
    setsockopt(listen_socket_fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    // Bind socket
    int rc = bind(listen_socket_fd_, reinterpret_cast<struct sockaddr*>(&addr), socklen);
    assert(rc == 0);

    // Listen on socket
    assert(0 == listen(listen_socket_fd_, 1));

    return kOk;
  }

  void Start() {
    accept_worker_ = std::make_unique<std::thread>([this](void) { this->Run(); });
  }

  void Stop() { terminated_ = true; }

  void Run() {
    int request_len = 1024000;
    char request_buf[1024000 + 1];
    char* request_buf_ptr = request_buf;

    while (!terminated_) {
      struct sockaddr_in addr;
      uint32_t len = sizeof(addr);

      int client = accept(listen_socket_fd_, reinterpret_cast<struct sockaddr*>(&addr), &len);
      if (client < 0) {
        trpc_err_t err = trpc_errno();
        if (err == kErrnoEAGAIN) {
          std::this_thread::sleep_for(200ms);
          continue;
        } else {
          perror("Unable to accept");
          break;
        }
      }

      // Set non-blocking
      int nb = 1;
      ioctl(client, FIONBIO, &nb);

      SslPtr ssl = ssl_ctx_->NewSsl();
      assert(ssl != nullptr);
      assert(ssl->SetFd(client));

      // Set state to work in server mode
      ssl->SetAcceptState();

      int rc = kError;
      // Do handshake
      do {
        rc = ssl->DoHandshake();
        std::this_thread::sleep_for(20ms);
      } while (rc == kWantRead || rc == kWantWrite);

      int n = 0;
      int buf_size = request_len;
      int read_bytes = 0;
      if (rc == kOk) {
        for (;;) {
          n = ssl->Recv(request_buf_ptr, buf_size);
          if (n > 0) {
            read_bytes += n;
            buf_size -= n;

            if (buf_size <= 0) {
              break;
            }

            continue;
          }

          // Recv again
          if (n == kWantWrite || n == kWantRead || n == kAgain) {
             if (read_bytes > message_size_) {
              break;
            }
            std::this_thread::sleep_for(200ms);
            continue;
          } else {
            break;
          }
        }

        request_buf_ptr[read_bytes] = '\0';
        std::string rsp_header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(read_bytes) + "\r\n"
            "\r\n";
        std::cout << "response header: " << rsp_header << std::endl;

        struct iovec iov {
          request_buf_ptr, static_cast<size_t>(read_bytes)
        };

        ssl->Send(rsp_header.data(), rsp_header.size());
        ssl->Writev(&iov, 1);
      } else {
        perror("DoHandshake() Failed:");
      }

      std::this_thread::sleep_for(2000ms);
      ssl->Shutdown();
      ::close(client);
    }
    ::close(listen_socket_fd_);
  }

 protected:
  void SetUp() override {
    // Init ssl_ctx
    assert(kOk == InitSslCtx());

    // Init listen socket
    assert(kOk == InitListenSocketFd());

    // To start accept worker
    Start();
  }

  void TearDown() override {
    Stop();
    if (accept_worker_) {
      accept_worker_->join();
      accept_worker_ = nullptr;
    }

    ssl_ctx_ = nullptr;
    DestroyOpenSsl();
  }

 protected:
  // Context and option of ssl
  SslContextPtr ssl_ctx_{nullptr};
  ServerSslOptions server_ssl_options_;

  // Listening socket, port, address
  int listen_socket_fd_{-1};
  int listen_port_{4433};  // Use random port may work well.
  std::string listen_addr_{"127.0.0.1"};

  // Worker thread of server
  bool terminated_{false};
  std::unique_ptr<std::thread> accept_worker_{nullptr};

  int message_size_{200000};
};

// just for debugging
TEST_F(SslTest, HTTPSPostSslServer) {
  auto http_client = std::make_shared<TestHttpClient>();
  http_client->SetInsecure(true);

  std::string url = R"(https://)" + listen_addr_ + R"(:)" + std::to_string(listen_port_) + R"(/)";
  std::string body = MakeBytes(message_size_);
  body.append("hello world!");

  trpc::http::Request req;
  req.SetMethod("POST");
  req.SetUrl(url);
  req.SetContent(body);

  trpc::http::Response rsp;
  trpc::Status status = http_client->Do(req, &rsp);
  std::cout << "status: " << status.ToString() << std::endl;
  ASSERT_TRUE(status.OK());
  std::string rsp_content = rsp.GetContent();

  std::cout << "POST, url:" << url << ", response:["
            << "response code:" << rsp.GetStatus() << ", body size:" << rsp_content.size() << "]" << std::endl;

  ASSERT_EQ(rsp_content.empty(), false);
  ASSERT_GT(rsp_content.size(), message_size_);
  ASSERT_TRUE(rsp_content.find("hello world!") != std::string::npos);

  Stop();
  std::this_thread::sleep_for(1000ms);
}

}  // namespace trpc::testing
#endif
