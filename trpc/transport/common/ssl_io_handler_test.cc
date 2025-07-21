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

#ifdef TRPC_BUILD_INCLUDE_SSL
#include "trpc/transport/common/ssl_io_handler.h"

#include <arpa/inet.h>
#include <assert.h>
#include <gtest/gtest.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <memory>
#include <string>
#include <thread>

#include "trpc/server/testing/http_client_testing.h"
#include "trpc/transport/common/ssl/core.h"
#include "trpc/transport/common/ssl/errno.h"
#include "trpc/transport/common/ssl/random.h"
#include "trpc/transport/common/ssl/ssl.h"

namespace trpc::testing {

// For testing purposes only.
using namespace std::chrono_literals;

class SslIoHandlerTest : public ::testing::Test {
 public:
  bool InitSslCtx() {
    // Init OpenSSL
    assert(ssl::InitOpenSsl());

    // Init option of ssl context
    InitSslCtxOption();

    // Init SSL_CTX
    ssl_ctx_ = MakeRefCounted<ssl::SslContext>();
    assert(ssl_ctx_->Init(server_ssl_options_));

    return true;
  }

  bool InitSslCtxOption() {
    server_ssl_options_.default_cert.cert_path = "./trpc/transport/common/ssl/cert/server_cert.pem";
    server_ssl_options_.default_cert.private_key_path = "./trpc/transport/common/ssl/cert/server_key.pem";
    server_ssl_options_.ciphers = ssl::GetDefaultCiphers();
    server_ssl_options_.dh_param_path = "./trpc/transport/common/ssl/cert/server_dhparam.pem";
    server_ssl_options_.protocols = ssl::kSslTlsV1 | ssl::kSslTlsV11 | ssl::kSslTlsV12;

    return true;
  }

  bool InitListenSocketFd() {
    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr);

    listen_port_ = ssl::Random::RandomInt(24001, 25000);

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

    return true;
  }

  void Start() {
    accept_worker_ = std::make_unique<std::thread>([this]() { this->Run(); });
  }

  void Stop() { terminated_ = true; }

  void Run() {
    int request_len = 1024;
    char request_buf[1024 + 1];
    std::string reply_message = "Hello World !";

    while (!terminated_) {
      struct sockaddr_in addr;
      uint32_t len = sizeof(addr);

      int client = accept(listen_socket_fd_, reinterpret_cast<struct sockaddr*>(&addr), &len);

      if (client < 0) {
        ssl::trpc_err_t err = trpc_errno();
        if (err == ssl::kErrnoEAGAIN) {
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

      ssl::SslPtr ssl = ssl_ctx_->NewSsl();
      assert(ssl != nullptr);
      assert(ssl->SetFd(client));

      // Set state to work in server mode
      ssl->SetAcceptState();

      std::unique_ptr<ssl::SslIoHandler> ssl_io_handler = std::make_unique<ssl::SslIoHandler>(nullptr, std::move(ssl));

      // Do handshake
      ssl::SslIoHandler::HandshakeStatus status;
      do {
        status = ssl_io_handler->Handshake(true);
      } while (status == ssl::SslIoHandler::HandshakeStatus::kNeedWrite ||
               status == ssl::SslIoHandler::HandshakeStatus::kNeedRead);

      std::cout << "ssl handshake:" << static_cast<int>(status) << std::endl;
      if (status == ssl::SslIoHandler::HandshakeStatus::kSucc) {
        for (;;) {
          int n = ssl_io_handler->Read(request_buf, request_len);

          if (n > 0) {
            request_buf[n] = '\0';
          }

          if (n == ssl::kWantRead || n == ssl::kWantWrite || n == ssl::kAgain) {
            continue;
          } else {
            break;
          }
        }

        std::string rsp_header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(reply_message.size()) + "\r\n"
            "\r\n";
        std::cout << "response header: " << rsp_header << std::endl;
        struct iovec iov[] {
          {rsp_header.data(), rsp_header.size()},
          {static_cast<char*>(reply_message.data()), reply_message.size()},
        };

        ssl_io_handler->Writev(iov, 2);
      } else {
        perror("DoHandshake() Failed:");
      }
      std::this_thread::sleep_for(2000ms);
      close(client);
    }
    close(listen_socket_fd_);
  }

 protected:
  void SetUp() override {
    // Init ssl_ctx
    assert(InitSslCtx());

    // Init listen socket
    assert(InitListenSocketFd());

    // To start accept worker
    Start();
  }

  void TearDown() override {
    Stop();
    if (accept_worker_) {
      accept_worker_->join();
      accept_worker_ = nullptr;
    }

    ssl::DestroyOpenSsl();
    ssl_ctx_ = nullptr;
  }

 protected:
  // Context and option of ssl
  ssl::SslContextPtr ssl_ctx_{nullptr};
  ssl::ServerSslOptions server_ssl_options_;

  // Listening socket, port, address
  int listen_socket_fd_{-1};
  int listen_port_{4433};  // Use random port may work well.
  std::string listen_addr_{"127.0.0.1"};

  // Worker thread of server
  bool terminated_{false};
  std::unique_ptr<std::thread> accept_worker_{nullptr};
};

TEST_F(SslIoHandlerTest, HTTPSPost) {
  auto http_client = std::make_shared<TestHttpClient>();
  http_client->SetInsecure(true);

  std::string url = R"(https://)" + listen_addr_ + R"(:)" + std::to_string(listen_port_) + R"(/)";
  std::string body = R"({"greetings":"Hello World !"})";

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

  ASSERT_TRUE(!rsp_content.empty());
  ASSERT_EQ(rsp_content, "Hello World !");
  Stop();
  std::this_thread::sleep_for(2000ms);
}

}  // namespace trpc::testing
#endif
