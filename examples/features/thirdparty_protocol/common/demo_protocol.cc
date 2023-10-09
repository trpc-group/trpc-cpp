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

#include "examples/features/thirdparty_protocol/common/demo_protocol.h"

#include <arpa/inet.h>

namespace examples::thirdparty_protocol {

bool DemoRequestProtocol::ZeroCopyDecode(::trpc::NoncontiguousBuffer& buff) {
  const char* ptr{nullptr};
  char header_buffer[8] = {0};

  if (buff.FirstContiguous().size() >= 8) {
      ptr = buff.FirstContiguous().data();
  } else {
    ::trpc::FlattenToSlow(buff, static_cast<void*>(header_buffer), 8);
    ptr = header_buffer;
  }

  memcpy(&packet_size, ptr, sizeof(packet_size));
  packet_size = ntohs(packet_size);

  memcpy(&packet_id, ptr + 4, sizeof(packet_id));
  packet_id = ntohs(packet_id);

  buff.Skip(8);

  req_data = ::trpc::FlattenSlow(buff);

  return true;
}

bool DemoRequestProtocol::ZeroCopyEncode(::trpc::NoncontiguousBuffer& buff) {
  packet_size = 8 + req_data.size();

  ::trpc::NoncontiguousBufferBuilder builder;
  auto* unaligned_header = builder.Reserve(8);

  uint32_t tmp_packet_size = htons(packet_size);
  memcpy(unaligned_header, &tmp_packet_size, 4);
  unaligned_header += 4;

  uint32_t tmp_packet_id = htons(packet_id);
  memcpy(unaligned_header, &tmp_packet_id, 4);

  builder.Append(std::move(req_data));

  buff = builder.DestructiveGet();

  return true;
}

bool DemoResponseProtocol::ZeroCopyDecode(::trpc::NoncontiguousBuffer& buff) {
  const char* ptr{nullptr};
  char header_buffer[8] = {0};

  if (buff.FirstContiguous().size() >= 8) {
      ptr = buff.FirstContiguous().data();
  } else {
    ::trpc::FlattenToSlow(buff, static_cast<void*>(header_buffer), 8);
    ptr = header_buffer;
  }

  memcpy(&packet_size, ptr, sizeof(packet_size));
  packet_size = ntohs(packet_size);

  memcpy(&packet_id, ptr + 4, sizeof(packet_id));
  packet_id = ntohs(packet_id);

  buff.Skip(8);

  rsp_data = ::trpc::FlattenSlow(buff);

  return true;
}

bool DemoResponseProtocol::ZeroCopyEncode(::trpc::NoncontiguousBuffer& buff) {
  packet_size = 8 + rsp_data.size();

  ::trpc::NoncontiguousBufferBuilder builder;
  auto* unaligned_header = builder.Reserve(8);

  uint32_t tmp_packet_size = htons(packet_size);
  memcpy(unaligned_header, &tmp_packet_size, 4);
  unaligned_header += 4;

  uint32_t tmp_packet_id = htons(packet_id);
  memcpy(unaligned_header, &tmp_packet_id, 4);

  builder.Append(std::move(rsp_data));

  buff = builder.DestructiveGet();

  return true;
}

}  // namespace examples::thirdparty_protocol
