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

#include <memory>
#include <string>

#include "trpc/codec/protocol.h"

namespace examples::thirdparty_protocol {

class DemoRequestProtocol : public ::trpc::Protocol {
 public:
  ~DemoRequestProtocol() override = default;

  bool ZeroCopyDecode(::trpc::NoncontiguousBuffer& buff) override;
  bool ZeroCopyEncode(::trpc::NoncontiguousBuffer& buff) override;

  bool GetRequestId(uint32_t& req_id) const override {
    req_id = packet_id;
    return true;
  }
  bool SetRequestId(uint32_t req_id) override {
    packet_id = req_id;
    return true;
  }

 public:
  // header(packet size[4 byte] + packet id[4 byte]) + body
  uint32_t packet_size{0};
  uint32_t packet_id{0};
  std::string req_data;
};

class DemoResponseProtocol : public ::trpc::Protocol {
 public:
  ~DemoResponseProtocol() override = default;

  bool ZeroCopyDecode(::trpc::NoncontiguousBuffer& buff) override;
  bool ZeroCopyEncode(::trpc::NoncontiguousBuffer& buff) override;

  bool GetRequestId(uint32_t& req_id) const override {
    req_id = packet_id;
    return true;
  }
  bool SetRequestId(uint32_t req_id) override {
    packet_id = req_id;
    return true;
  }

 public:
  // header(packet size[4 byte] + packet id[4 byte]) + body
  uint32_t packet_size{0};
  uint32_t packet_id{0};
  std::string rsp_data;
};

using DemoRequestProtocolPtr = std::shared_ptr<DemoRequestProtocol>;
using DemoResponseProtocolPtr = std::shared_ptr<DemoResponseProtocol>;

}  // namespace examples::thirdparty_protocol
