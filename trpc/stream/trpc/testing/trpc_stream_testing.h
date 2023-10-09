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

#include <string>

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/codec/trpc/trpc_protocol.h"

namespace trpc::testing {

TrpcStreamInitFrameProtocol CreateTrpcStreamInitFrameProtocol(uint32_t stream_id, TrpcStreamInitMeta metadata) {
  TrpcStreamInitFrameProtocol init_frame{};
  init_frame.fixed_header.stream_id = stream_id;
  init_frame.stream_init_metadata = std::move(metadata);
  init_frame.fixed_header.data_frame_size = init_frame.ByteSizeLong();
  return init_frame;
}

TrpcStreamDataFrameProtocol CreateTrpcStreamDataFrameProtocol(uint32_t stream_id, NoncontiguousBuffer data) {
  TrpcStreamDataFrameProtocol data_frame{};
  data_frame.fixed_header.stream_id = stream_id;
  data_frame.body = std::move(data);
  data_frame.fixed_header.data_frame_size = data_frame.ByteSizeLong();
  return data_frame;
}

TrpcStreamFeedbackFrameProtocol CreateTrpcStreamFeedbackFrameProtocol(uint32_t stream_id,
                                                                      TrpcStreamFeedBackMeta metadata) {
  TrpcStreamFeedbackFrameProtocol feedback_frame;
  feedback_frame.fixed_header.stream_id = stream_id;
  feedback_frame.stream_feedback_metadata = std::move(metadata);
  feedback_frame.fixed_header.data_frame_size = feedback_frame.ByteSizeLong();
  return feedback_frame;
}

TrpcStreamCloseFrameProtocol CreateTrpcStreamCloseFrameProtocol(uint32_t stream_id, TrpcStreamCloseMeta metadata) {
  TrpcStreamCloseFrameProtocol close_frame;
  close_frame.fixed_header.stream_id = stream_id;
  close_frame.stream_close_metadata = metadata;
  close_frame.fixed_header.data_frame_size = close_frame.ByteSizeLong();
  return close_frame;
}

}  // namespace trpc::testing
