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

#pragma once

#include "trpc/runtime/iomodel/reactor/common/socket.h"
#ifdef TRPC_BUILD_INCLUDE_SSL
#include "trpc/transport/common/ssl/ssl.h"
#endif
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc::testing {

bool CreateTcpSocket(const std::string& ip, int port, bool is_ipv6, trpc::Socket* socket);

class TestStreamTransport {
 public:
  virtual int Send(trpc::NoncontiguousBuffer&& buff) = 0;
  virtual int Recv(trpc::NoncontiguousBuffer& buff) = 0;
};

class TestStreamTransportImpl : public TestStreamTransport {
 public:
  TestStreamTransportImpl(trpc::Socket&& socket) : socket_(std::move(socket)) {}

  int Send(trpc::NoncontiguousBuffer&& buff) override;
  int Recv(trpc::NoncontiguousBuffer& buff) override;

 protected:
  virtual int Read(void* buff, size_t len);
  virtual int Write(const void* buff, size_t len);

 protected:
  trpc::Socket socket_;
  trpc::BufferBuilder recv_buffer_builder_;
};

#ifdef TRPC_BUILD_INCLUDE_SSL

class TestSslStreamTransportImpl : public TestStreamTransportImpl {
 public:
  TestSslStreamTransportImpl(trpc::Socket&& socket, trpc::ssl::SslPtr&& ssl)
      : TestStreamTransportImpl(std::move(socket)), ssl_(std::move(ssl)) {}

  int Handshake();
  int Read(void* buff, size_t len) override;
  int Write(const void* buff, size_t len) override;

 private:
  trpc::ssl::SslPtr ssl_{nullptr};
  bool handshake_ok_{false};
};

#endif

}  // namespace trpc::testing
