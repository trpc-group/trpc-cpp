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

#include "trpc/codec/trpc/trpc_proto_checker.h"

#include <arpa/inet.h>

#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc {

int CheckTrpcProtocolMessage(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
  while (true) {
    uint32_t total_buff_size = in.ByteSize();
    // Checks buffer contains a full fixed header.
    if (total_buff_size < TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE) {
      TRPC_FMT_TRACE("trpc protocol checker less, buff size:{} is less than 16", total_buff_size);
      break;
    }

    // A fixed 16-bytes header.
    TrpcFixedHeader header;
    // only return success.
    header.Decode(in, false);

    // Checks magic number.
    if (TRPC_UNLIKELY(header.magic_value != TrpcMagic::TRPC_MAGIC_VALUE)) {
      TRPC_FMT_ERROR("Check error, magic_value:{} TRPC_MAGIC_VALUE:{}", header.magic_value,
                     static_cast<int32_t>(TrpcMagic::TRPC_MAGIC_VALUE));
      return PacketChecker::PACKET_ERR;
    }

    // Checks size the length of frame.
    if (TRPC_UNLIKELY(header.data_frame_size < TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE + header.pb_header_size)) {
      TRPC_FMT_ERROR("Check error, packet size:{}, pb_header_size:{}", header.data_frame_size, header.pb_header_size);
      return PacketChecker::PACKET_ERR;
    }
    if (TRPC_UNLIKELY(conn.Get() != nullptr && header.data_frame_size > conn->GetMaxPacketSize())) {
      TRPC_FMT_ERROR("Check error, packet size:{}, MaxPacketSize:{}", header.data_frame_size, conn->GetMaxPacketSize());
      return PacketChecker::PACKET_ERR;
    }

    // Checks it contains full data frame.
    if (total_buff_size < header.data_frame_size) {
      TRPC_FMT_TRACE("Check less, total_buff_size:{} packet_size:{}", total_buff_size, header.data_frame_size);
      break;
    }
    out.emplace_back(in.Cut(header.data_frame_size));
  }
  return !out.empty() ? PacketChecker::PACKET_FULL : PacketChecker::PACKET_LESS;
}

bool PickTrpcProtocolMessageMetadata(const std::any& message, std::any& data) {
  auto& buffer = const_cast<NoncontiguousBuffer&>(std::any_cast<const NoncontiguousBuffer&>(message));
  auto& metadata = std::any_cast<TrpcProtocolMessageMetadata&>(data);
  TrpcFixedHeader fixed_header{};

  // Try to decode the fixed header.
  if (!fixed_header.Decode(buffer, false)) {
    return false;
  }

  metadata.data_frame_type = fixed_header.data_frame_type;
  metadata.stream_id = fixed_header.stream_id;
  metadata.stream_frame_type = fixed_header.stream_frame_type;
  // Streaming frame: TrpcDataFrameType::TRPC_STREAM_FRAME = 0x01
  metadata.enable_stream = fixed_header.data_frame_type == 0x01 ? true : false;

  return true;
}

}  // namespace trpc
