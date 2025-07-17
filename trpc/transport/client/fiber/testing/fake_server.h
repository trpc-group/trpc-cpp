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
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "trpc/codec/protocol.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/runtime/iomodel/reactor/common/connection_handler.h"
#include "trpc/runtime/iomodel/reactor/common/io_message.h"
#include "trpc/runtime/iomodel/reactor/common/network_address.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_tcp_connection.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_udp_transceiver.h"
#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/runtime/threadmodel/thread_model_manager.h"
#include "trpc/transport/server/fiber/fiber_server_transport_impl.h"
#include "trpc/util/net_util.h"

#define COUT std::cout << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "|"

namespace trpc::testing {

class TestProtocol : public Protocol {
 public:
  bool ZeroCopyDecode(NoncontiguousBuffer& buff) override {
    if (buff.ByteSize() < 8) {
      TRPC_FMT_ERROR("buff.ByteSize:{} less than 8", buff.ByteSize());
      return false;
    }

    const char* ptr{nullptr};
    char header_buffer[8] = {0};

    if (buff.FirstContiguous().size() >= 8) {
      ptr = buff.FirstContiguous().data();
    } else {
      FlattenToSlow(buff, static_cast<void*>(header_buffer), 8);
      ptr = header_buffer;
    }

    uint32_t req_size = 0;
    uint32_t pos = 0;
    memcpy(&req_size, ptr + pos, sizeof(req_size));
    req_size = ntohl(req_size);
    pos += 4;

    memcpy(&req_id_, ptr + pos, sizeof(req_id_));
    req_id_ = ntohl(req_id_);
    pos += 4;

    buff.Skip(8);

    body_ = FlattenSlow(buff);

    return true;
  }

  bool ZeroCopyEncode(NoncontiguousBuffer& buff) override {
    NoncontiguousBufferBuilder builder;
    auto* unaligned_header = builder.Reserve(8);

    uint32_t req_size = 8 + body_.size();
    uint32_t req_size_temp = htonl(req_size);
    memcpy(unaligned_header, &req_size_temp, 4);
    unaligned_header += 4;

    uint32_t req_id_temp = htonl(req_id_);
    memcpy(unaligned_header, &req_id_temp, 4);
    unaligned_header += 4;

    builder.Append(body_);

    buff = builder.DestructiveGet();

    return true;
  }

  bool GetRequestId(uint32_t& req_id) const override {
    req_id = req_id_;
    COUT << req_id << std::endl;
    return true;
  }

  uint32_t req_id_;
  std::string body_;
};

using TestProtocolPtr = std::shared_ptr<TestProtocol>;

}  // namespace trpc::testing

class FakeServer {
 public:
  FakeServer() {
    addr_ = trpc::NetworkAddress(trpc::util::GenRandomAvailablePort(), false, trpc::NetworkAddress::IpType::kIpV4);
  }

  void StartServer() {
    SetUpServer("tcp,udp");
  }

  void StartTcpServer() {
    SetUpServer("tcp");
  }

  void StartUdpServer() {
    SetUpServer("udp");
  }

  trpc::NetworkAddress GetServerAddr() { return addr_; }

  trpc::NodeAddr GetServerNodeAddr() {
    trpc::NodeAddr addr;
    addr.addr_type = addr_.IsIpv4() ? trpc::NodeAddr::AddrType::kIpV4 : trpc::NodeAddr::AddrType::kIpV6;
    addr.ip = addr_.Ip();
    addr.port = addr_.Port();
    return addr;
  }

  void StopServer() { TearDownServer(); }

  void SetUpServer(const std::string& network) {
    server_transport_ = std::make_unique<trpc::FiberServerTransportImpl>();

    bind_info_.is_ipv6 = false;
    bind_info_.socket_type = "net";
    bind_info_.ip = addr_.Ip();
    bind_info_.port = addr_.Port();
    bind_info_.network = network;
    bind_info_.protocol = "trpc";
    bind_info_.accept_function = nullptr;
    bind_info_.checker_function = [](const trpc::ConnectionPtr& conn, trpc::NoncontiguousBuffer& in,
                                     std::deque<std::any>& out) {
      if (in.ByteSize() < 4) {
        return trpc::PacketChecker::PACKET_LESS;
      }

      const char* ptr{nullptr};
      char header_buffer[4] = {0};
      if (in.FirstContiguous().size() >= 4) {
        ptr = in.FirstContiguous().data();
      } else {
        FlattenToSlow(in, static_cast<void*>(header_buffer), 4);
        ptr = header_buffer;
      }

      uint32_t req_size = 0;
      memcpy(&req_size, ptr, sizeof(req_size));
      req_size = ntohl(req_size);

      out.emplace_back(in.Cut(req_size));

      return trpc::PacketChecker::PACKET_FULL;
    };

    bind_info_.msg_handle_function = [](const trpc::ConnectionPtr& conn, std::deque<std::any>& msg) -> bool {
      for (auto& it : msg) {
        trpc::testing::TestProtocol in;
        trpc::NoncontiguousBuffer& buf = std::any_cast<trpc::NoncontiguousBuffer&>(it);
        in.ZeroCopyDecode(buf);

        std::string& str = in.body_;
        COUT << "buffer data:" << str << std::endl;
        if (str.find("timeout") != std::string::npos) {
          trpc::FiberSleepFor(std::chrono::nanoseconds(500000));
        } else if (str.find("close") != std::string::npos) {
          COUT << "stop conn";
          // static_cast<trpc::FiberConnection*>(conn.Get())->Stop();
          return false;
        } else if (str.find("oneway") != std::string::npos) {
          continue;
        } else {
          trpc::testing::TestProtocol out;
          out.req_id_ = in.req_id_;
          out.body_ = "hello";

          trpc::NoncontiguousBuffer buff;
          out.ZeroCopyEncode(buff);

          trpc::IoMessage msg;
          msg.seq_id = 0;
          msg.buffer = std::move(buff);
          msg.port = conn->GetPeerPort();
          msg.ip = conn->GetPeerIp();
          if (str.find("backup request sleep") != std::string::npos) {
            // Simulate the backup request scenario:
            // the server sleeps first after receiving the package to trigger the backup request
            trpc::FiberSleepFor(std::chrono::nanoseconds(20000000));
          }
          conn->Send(std::move(msg));
        }
      }

      return true;
    };
    bind_info_.run_server_filters_function = [](trpc::FilterPoint, trpc::STransportRspMsg*) {
      return trpc::FilterStatus::CONTINUE;
    };

    server_transport_->Bind(bind_info_);
    server_transport_->Listen();
  }

  void TearDownServer() {
    std::cout << "TearDownServer stop" << std::endl;
    server_transport_->Stop();
    std::cout << "TearDownServer destroy" << std::endl;
    server_transport_->Destroy();
    std::cout << "TearDownServer destroy end" << std::endl;
    server_transport_.reset(nullptr);

    std::cout << "TearDownServer end" << std::endl;
  }

 protected:
  trpc::NetworkAddress addr_;
  trpc::BindInfo bind_info_;
  std::unique_ptr<trpc::FiberServerTransportImpl> server_transport_;
};

trpc::TransInfo MakeTransInfo(bool tcp = true, bool conn_complex = true,
                              bool pipeline = false, bool short_conn = false) {
  trpc::TransInfo trans_info;
  if (tcp) {
    if (!short_conn) {
      trans_info.conn_type = trpc::ConnectionType::kTcpLong;
    } else {
      trans_info.conn_type = trpc::ConnectionType::kTcpShort;
    }
  } else {
    trans_info.conn_type = trpc::ConnectionType::kUdp;
  }

  if (conn_complex) {
    trans_info.is_complex_conn = true;
  } else {
    trans_info.is_complex_conn = false;
  }

  if (pipeline) {
    trans_info.support_pipeline = true;
  } else {
    trans_info.support_pipeline = false;
  }

  trans_info.checker_function = [](const trpc::ConnectionPtr&, trpc::NoncontiguousBuffer& in,
                                   std::deque<std::any>& out) {
    COUT << "MakeTransInfo data:" << FlattenSlow(in) << ",total_buff_size:" << in.ByteSize()
         << std::endl;

    if (in.ByteSize() < 4) {
      TRPC_FMT_ERROR("buff.ByteSize:{} less than 4", in.ByteSize());
      return trpc::PacketChecker::PACKET_LESS;
    }

    const char* ptr{nullptr};
    char header_buffer[4] = {0};

    if (in.FirstContiguous().size() >= 4) {
      ptr = in.FirstContiguous().data();
    } else {
      FlattenToSlow(in, static_cast<void*>(header_buffer), 4);
      ptr = header_buffer;
    }

    uint32_t req_size = 0;
    memcpy(&req_size, ptr, sizeof(req_size));
    req_size = ntohl(req_size);

    out.emplace_back(in.Cut(req_size));

    return trpc::PacketChecker::PACKET_FULL;
  };

  trans_info.rsp_decode_function = [](std::any&& in, trpc::ProtocolPtr& out) {
    out = std::make_shared<trpc::testing::TestProtocol>();
    auto&& buf = std::any_cast<trpc::NoncontiguousBuffer&&>(std::move(in));
    out->ZeroCopyDecode(buf);
    return true;
  };

  return trans_info;
}
