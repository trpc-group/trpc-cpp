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

#include "trpc/server/testing/stream_transport_testing.h"

namespace trpc::testing {

bool CreateTcpSocket(const std::string& ip, int port, bool is_ipv6, trpc::Socket* socket) {
  NetworkAddress addr(ip, port, is_ipv6 ? NetworkAddress::IpType::kIpV6 : NetworkAddress::IpType::kIpV4);
  *socket = Socket::CreateTcpSocket(addr.IsIpv6());
  if (!socket->IsValid()) {
    std::cerr << "invalid socket" << std::endl;
    return false;
  }
  socket->SetBlock(false);
  int ret = socket->Connect(addr);
  if (ret != 0) {
    std::cerr << "failed to connect to peer: " << ip << ":" << port << std::endl;
    return false;
  }
  std::cout << "connected to peer: " << ip << ":" << port << std::endl;
  return true;
}

int TestStreamTransportImpl::Send(trpc::NoncontiguousBuffer&& buff) {
  std::string s = trpc::FlattenSlow(buff);
  int nwrite = Write(s.data(), s.size());
  if (nwrite <= 0) {
    std::cerr << "socket send error: " << nwrite << ", errno: " << errno << ", errstr:" << strerror(errno) << std::endl;
  }
  return nwrite;
}

int TestStreamTransportImpl::Recv(trpc::NoncontiguousBuffer& buff) {
  int nread = 0;
  while (true) {
    size_t writable_size = recv_buffer_builder_.SizeAvailable();
    int n = Read(recv_buffer_builder_.data(), writable_size);
    if (n > 0) {
      nread += n;
      buff.Append(recv_buffer_builder_.Seal(n));
      if ((size_t)n < writable_size) {
        break;
      }
    } else if (n == 0) {
      nread = -1;
      std::cerr << "remote peer closed connection" << std::endl;
      break;
    } else {
      if (errno != EAGAIN) {
        if (errno == EINTR) {
          continue;
        }
        nread = n;
        std::cerr << "socket recv error: " << nread << ", errno: " << errno << ", errstr:" << strerror(errno)
                  << std::endl;
        break;
      }
      // Try again until read some byes.
    }
  }
  return nread;
}

int TestStreamTransportImpl::Read(void* buff, size_t len) { return socket_.Recv(buff, len); }

int TestStreamTransportImpl::Write(const void* buff, size_t len) { return socket_.Send(buff, len); }

#ifdef TRPC_BUILD_INCLUDE_SSL

int TestSslStreamTransportImpl::Handshake() {
  if (handshake_ok_) {
    return 0;
  }
  int n = ssl_->DoHandshake();
  if (trpc::ssl::kOk == n) {
    handshake_ok_ = true;
    return 0;
  } else if (trpc::ssl::kError == n) {
    return -1;
  } else if (trpc::ssl::kWantRead == n || trpc::ssl::kWantWrite == n) {
    return -2;
  }
  return -1;
}

int TestSslStreamTransportImpl::Read(void* buff, size_t len) {
  int n = ssl_->Recv(reinterpret_cast<char*>(buff), len);
  if (n > 0) {
    return n;
  } else if (n == trpc::ssl::kWantRead || n == trpc::ssl::kWantWrite) {
    errno = EAGAIN;
  }
  return n;
}

int TestSslStreamTransportImpl::Write(const void* buff, size_t len) {
  int n = ssl_->Send(reinterpret_cast<const char*>(buff), len);
  if (n > 0) {
    return n;
  } else if (n == trpc::ssl::kWantRead || n == trpc::ssl::kWantWrite) {
    errno = EAGAIN;
  }
  return n;
}

#endif

}  // namespace trpc::testing
