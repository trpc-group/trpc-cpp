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

#include "trpc/codec/grpc/grpc_protocol.h"
#include "trpc/codec/grpc/http2/request.h"
#include "trpc/stream/stream_handler.h"

namespace trpc::stream {

/// @brief The base class for checking stream frames from the network in grpc stream.
class GrpcStreamFrame : public RefCounted<GrpcStreamFrame> {
 public:
  explicit GrpcStreamFrame(uint32_t stream_id) : stream_id_(stream_id) {}
  virtual ~GrpcStreamFrame() = default;
  virtual StreamMessageCategory GetFrameType() { return StreamMessageCategory::kStreamUnknown; }
  uint32_t GetStreamId() { return stream_id_; }

 protected:
  uint32_t stream_id_{0};
};
using GrpcStreamFramePtr = RefPtr<GrpcStreamFrame>;

/// @brief INIT frame checked from the network for grpc stream.
class GrpcStreamInitFrame : public GrpcStreamFrame {
 public:
  GrpcStreamInitFrame(uint32_t stream_id, const http2::RequestPtr& req) : GrpcStreamFrame(stream_id), req_(req) {}
  StreamMessageCategory GetFrameType() override { return StreamMessageCategory::kStreamInit; }
  http2::RequestPtr GetRequest() { return req_; }

 private:
  http2::RequestPtr req_{nullptr};
};

/// @brief DATA frame checked from the network for grpc stream.
class GrpcStreamDataFrame : public GrpcStreamFrame {
 public:
  GrpcStreamDataFrame(uint32_t stream_id, NoncontiguousBuffer&& data)
      : GrpcStreamFrame(stream_id), data_(std::move(data)) {}

  StreamMessageCategory GetFrameType() override { return StreamMessageCategory::kStreamData; }

  NoncontiguousBuffer GetData() { return std::move(data_); }

 private:
  NoncontiguousBuffer data_;
  bool decode_status_{true};
};

/// @brief CLOSE frame checked from the network for grpc stream.
class GrpcStreamCloseFrame : public GrpcStreamFrame {
 public:
  explicit GrpcStreamCloseFrame(uint32_t stream_id) : GrpcStreamFrame(stream_id) {}
  StreamMessageCategory GetFrameType() override { return StreamMessageCategory::kStreamClose; }
};

/// @brief RESET frame checked from the network for grpc stream.
class GrpcStreamResetFrame : public GrpcStreamFrame {
 public:
  GrpcStreamResetFrame(uint32_t stream_id, uint32_t error_code) : GrpcStreamFrame(stream_id), error_code_(error_code) {}
  StreamMessageCategory GetFrameType() override { return StreamMessageCategory::kStreamReset; }

  uint32_t GetErrorCode() { return error_code_; }

 private:
  uint32_t error_code_{0};
};

// @brief A wrapper of grpc streaming message.
struct GrpcRequestPacket {
  http2::RequestPtr req{nullptr};
  GrpcStreamFramePtr frame{nullptr};
};

}  // namespace trpc::stream
