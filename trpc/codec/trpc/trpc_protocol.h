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
#include <utility>

#include "trpc/codec/protocol.h"
#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

/// @brief A Header structure represents fixed 16-bytes header of `trpc` protocol.
struct TrpcFixedHeader {
  static constexpr uint32_t TRPC_PROTO_PREFIX_SPACE = 16;
  static constexpr uint32_t TRPC_PROTO_MAGIC_SPACE = 2;
  static constexpr uint32_t TRPC_PROTO_DATAFRAME_TYPE_SPACE = 1;
  static constexpr uint32_t TRPC_PROTO_STREAMFRAME_TYPE_SPACE = 1;
  static constexpr uint32_t TRPC_PROTO_DATAFRAME_SIZE_SPACE = 4;
  static constexpr uint32_t TRPC_PROTO_HEADER_SIZE_SPACE = 2;
  static constexpr uint32_t TRPC_PROTO_STREAM_ID_SPACE = 4;
  static constexpr uint32_t TRPC_PROTO_REVERSED_SPACE = 2;

  // Magic number.
  uint16_t magic_value = TrpcMagic::TRPC_MAGIC_VALUE;

  // Type of date frame type, unary call and streaming call are included.
  // 0x00: TRPC_UNARY_FRAME
  // 0x01: TRPC_STREAM_FRAME
  uint8_t data_frame_type = 0;

  // Type of streaming frame.
  // 0x00: TRPC_UNARY, the default value of unary data frame, it does not make sense where data frame is streaming.
  // 0x01: TRPC_STREAM_FRAME_INIT, initial frame of streaming data frame.
  // 0x02: TRPC_STREAM_FRAME_DATA，data frame of streaming data frame.
  // 0x03: TRPC_STREAM_FRAME_FEEDBACK, feedback frame of streaming data frame.
  // 0x04: TRPC_STREAM_FRAME_CLOSE，closed frame of streaming data frame.
  uint8_t stream_frame_type = 0;

  // Total length of wire protocol packet.
  // It contains: fixed header + header of request or response + serialized bytes of messages  + attachment.
  uint32_t data_frame_size = 0;

  // Size of request or size of response. It is zero where data frame is streaming.
  uint16_t pb_header_size = 0;

  // 32-bits Stream ID.
  uint32_t stream_id = 0;

  // Reserved bytes.
  char reversed[2] = {0};

  /// @brief Decodes binary bytes into fixed header structure.
  ///
  /// @param buff is the buffer of binary bytes.
  /// @param skip indicates whether skip first bytes long of fixed header.
  /// @return Returns true on success, false otherwise.
  bool Decode(NoncontiguousBuffer& buff, bool skip = true);

  /// @brief Encodes fixed header structure into bytes.
  bool Encode(char* buff) const;

  /// @brief Returns bytes long of fixed header which is encoded into binary bytes.
  constexpr size_t ByteSizeLong() const noexcept { return TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE; }
};

/// @brief Trpc request protocol message.
class TrpcRequestProtocol : public Protocol {
 public:
  ~TrpcRequestProtocol() override = default;

  /// @brief Decodes or encodes trpc request protocol message (zero copy).
  bool ZeroCopyDecode(NoncontiguousBuffer& buff) override;
  bool ZeroCopyEncode(NoncontiguousBuffer& buff) override;

  /// @brief Sets or Gets body payload of request protocol message.
  /// @note For "GetBody" function, the value of body will be moved, cannot be called repeatedly.
  void SetNonContiguousProtocolBody(NoncontiguousBuffer&& buff) override { req_body = std::move(buff); }
  NoncontiguousBuffer GetNonContiguousProtocolBody() override { return std::move(req_body); }

  /// @brief Sets or Gets attachment payload of request protocol message.
  /// @note For "GetAttachment" function, the value of body will be moved, cannot be called repeatedly.
  void SetProtocolAttachment(NoncontiguousBuffer&& buff) override { req_attachment = std::move(buff); }
  NoncontiguousBuffer GetProtocolAttachment() override { return std::move(req_attachment); }

  /// @brief Get/Set the unique id of request protocol.
  bool GetRequestId(uint32_t& req_id) const override {
    req_id = req_header.request_id();
    return true;
  }
  bool SetRequestId(uint32_t req_id) override {
    req_header.set_request_id(req_id);
    return true;
  }

  /// @brief Get/Set the timeout of request/response protocol.
  virtual void SetTimeout(uint32_t timeout) { req_header.set_timeout(timeout); }
  virtual uint32_t GetTimeout() const { return req_header.timeout(); }

  /// @brief Set/Get name of caller.
  void SetCallerName(std::string caller_name) override { req_header.set_caller(std::move(caller_name)); }
  const std::string& GetCallerName() const override { return req_header.caller(); }

  /// @brief Set/Get name of callee.
  void SetCalleeName(std::string callee_name) override { req_header.set_callee(std::move(callee_name)); }
  const std::string& GetCalleeName() const override { return req_header.callee(); }

  /// @brief  Set/Get function name of RPC.
  void SetFuncName(std::string func_name) override { req_header.set_func(std::move(func_name)); }
  const std::string& GetFuncName() const override { return req_header.func(); }

  /// @brief Set key-value pair (tans-info map).
  void SetKVInfo(std::string key, std::string value) override;

  /// @brief Get key-value information.
  const TransInfoMap& GetKVInfos() const override;

  /// @brief Returns mutable key-value pairs (trans-info map).
  TransInfoMap* GetMutableKVInfos() override;

  /// @brief Get size of message
  uint32_t GetMessageSize() const override;

 public:
  // Fixed 16-bytes header of `trpc` protocol.
  TrpcFixedHeader fixed_header;

  // Header of request.
  RequestProtocol req_header;

  // Body of request, it will be moved.
  NoncontiguousBuffer req_body;

  // Content of attachment.
  NoncontiguousBuffer req_attachment;
};

/// @brief Trpc response protocol message.
class TrpcResponseProtocol : public Protocol {
 public:
  ~TrpcResponseProtocol() override = default;

  /// @brief Decodes or encodes a response protocol message (zero copy).
  bool ZeroCopyDecode(NoncontiguousBuffer& buff) override;
  bool ZeroCopyEncode(NoncontiguousBuffer& buff) override;

  /// @brief Sets or Gets body payload of response protocol message.
  /// @note For "GetBody" function, the value of body will be moved, cannot be called repeatedly.
  void SetNonContiguousProtocolBody(NoncontiguousBuffer&& buff) override { rsp_body = std::move(buff); }
  NoncontiguousBuffer GetNonContiguousProtocolBody() override { return std::move(rsp_body); }

  /// @brief Sets or Gets attachment payload of response protocol message.
  /// @note For "GetAttachment" function, the value of body will be moved, cannot be called repeatedly.
  void SetProtocolAttachment(NoncontiguousBuffer&& buff) override { rsp_attachment = std::move(buff); }
  NoncontiguousBuffer GetProtocolAttachment() override { return std::move(rsp_attachment); }

  /// @brief Get/Set the unique id of response protocol.
  bool GetRequestId(uint32_t& req_id) const override {
    req_id = rsp_header.request_id();
    return true;
  }
  bool SetRequestId(uint32_t req_id) override {
    rsp_header.set_request_id(req_id);
    return true;
  }

  /// @brief Set key-value pair (trans-info map).
  void SetKVInfo(std::string key, std::string value) override;

  /// @brief Get key-value information.
  const TransInfoMap& GetKVInfos() const override;

  /// @brief Returns mutable key-value pairs (trans-info map).
  google::protobuf::Map<std::string, std::string>* GetMutableKVInfos() override;

  /// @brief Get size of message
  uint32_t GetMessageSize() const override;

 public:
  // Fixed 16-bytes header of `trpc` protocol.
  TrpcFixedHeader fixed_header;

  // Header of response.
  ResponseProtocol rsp_header;

  // Body of response, it will be moved.
  NoncontiguousBuffer rsp_body;

  // Content of attachment.
  NoncontiguousBuffer rsp_attachment;
};

using TrpcRequestProtocolPtr = std::shared_ptr<TrpcRequestProtocol>;
using TrpcResponseProtocolPtr = std::shared_ptr<TrpcResponseProtocol>;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// @brief Metadata of trpc protocol message.
struct TrpcProtocolMessageMetadata {
  // Type of data frame. e.g. Unary RPC, Streaming RPC.
  uint8_t data_frame_type{0};

  // Indicates it has streaming RPC.
  bool enable_stream{false};

  // Type of streaming-frame, e.g. INIT_FRAME, DATA_FRAME, CLOSE_FRAME, FEEDBACK_FRAME
  uint8_t stream_frame_type{0};

  // ID of stream.
  uint32_t stream_id{0};
};

/// @brief Streaming init-frame protocol message.
class TrpcStreamInitFrameProtocol : public Protocol {
 public:
  TrpcStreamInitFrameProtocol() {
    fixed_header.data_frame_type = TrpcDataFrameType::TRPC_STREAM_FRAME;
    fixed_header.stream_frame_type = TrpcStreamFrameType::TRPC_STREAM_FRAME_INIT;
    fixed_header.data_frame_size = 0;
    fixed_header.pb_header_size = 0;
    fixed_header.stream_id = 0;
  }

  ~TrpcStreamInitFrameProtocol() override = default;

  /// @brief Decodes or encodes a streaming init-frame protocol message (zero copy).
  bool ZeroCopyDecode(NoncontiguousBuffer& buffer) override;
  bool ZeroCopyEncode(NoncontiguousBuffer& buffer) override;

  /// @brief Reports whether it is a valid init-frame.
  bool Check() const noexcept;

  /// @brief Returns bytes long of init-frame which is encoded into binary bytes.
  size_t ByteSizeLong() const noexcept override {
    return fixed_header.ByteSizeLong() + stream_init_metadata.ByteSizeLong();
  }

 public:
  // Fixed 16-bytes header of `trpc` protocol.
  TrpcFixedHeader fixed_header;

  // Metadata of init-frame.
  TrpcStreamInitMeta stream_init_metadata;
};
using TrpcStreamInitFrameProtocolPtr = std::shared_ptr<TrpcStreamInitFrameProtocol>;

/// @brief Streaming data-frame protocol message.
class TrpcStreamDataFrameProtocol : public Protocol {
 public:
  TrpcStreamDataFrameProtocol() {
    fixed_header.data_frame_type = TrpcDataFrameType::TRPC_STREAM_FRAME;
    fixed_header.stream_frame_type = TrpcStreamFrameType::TRPC_STREAM_FRAME_DATA;
    fixed_header.data_frame_size = 0;
    fixed_header.pb_header_size = 0;
    fixed_header.stream_id = 0;
  }

  ~TrpcStreamDataFrameProtocol() override = default;

  /// @brief Decodes or encodes a streaming data-frame protocol message (zero copy).
  bool ZeroCopyDecode(NoncontiguousBuffer& buffer) override;
  bool ZeroCopyEncode(NoncontiguousBuffer& buffer) override;

  /// @brief Reports whether it is a valid data-frame.
  bool Check() const noexcept;

  /// @brief Returns bytes long of data-frame which is encoded into binary bytes.
  size_t ByteSizeLong() const noexcept override { return fixed_header.ByteSizeLong() + body.ByteSize(); }

 public:
  // Fixed 16-bytes header of `trpc` protocol.
  TrpcFixedHeader fixed_header;

  // Payload of data-frame. Only serialized message of protobuf is ok here.
  NoncontiguousBuffer body;
};
using TrpcStreamDataFrameProtocolPtr = std::shared_ptr<TrpcStreamDataFrameProtocol>;

/// @brief Streaming feedback-frame protocol message.
class TrpcStreamFeedbackFrameProtocol : public Protocol {
 public:
  TrpcStreamFeedbackFrameProtocol() {
    fixed_header.data_frame_type = TrpcDataFrameType::TRPC_STREAM_FRAME;
    fixed_header.stream_frame_type = TrpcStreamFrameType::TRPC_STREAM_FRAME_FEEDBACK;
    fixed_header.data_frame_size = 0;
    fixed_header.pb_header_size = 0;
    fixed_header.stream_id = 0;
  }

  ~TrpcStreamFeedbackFrameProtocol() override = default;

  /// @brief Decodes or encodes a streaming feedback-frame protocol message (zero copy).
  bool ZeroCopyDecode(NoncontiguousBuffer& buffer) override;
  bool ZeroCopyEncode(NoncontiguousBuffer& buffer) override;

  /// @brief Reports whether it is a valid feedback-frame.
  bool Check() const noexcept;

  /// @brief Returns bytes long of feedback-frame which is encoded into binary bytes.
  size_t ByteSizeLong() const noexcept override {
    return fixed_header.ByteSizeLong() + stream_feedback_metadata.ByteSizeLong();
  }

 public:
  // Fixed 16-bytes header of `trpc` protocol.
  TrpcFixedHeader fixed_header;

  // Metadata of feedback-frame.
  TrpcStreamFeedBackMeta stream_feedback_metadata;
};
using TrpcStreamFeedbackFrameProtocolPtr = std::shared_ptr<TrpcStreamFeedbackFrameProtocol>;

/// @brief Streaming close-frame protocol message.
class TrpcStreamCloseFrameProtocol : public Protocol {
 public:
  TrpcStreamCloseFrameProtocol() {
    fixed_header.data_frame_type = TrpcDataFrameType::TRPC_STREAM_FRAME;
    fixed_header.stream_frame_type = TrpcStreamFrameType::TRPC_STREAM_FRAME_CLOSE;
    fixed_header.data_frame_size = 0;
    fixed_header.pb_header_size = 0;
    fixed_header.stream_id = 0;
  }

  ~TrpcStreamCloseFrameProtocol() override = default;

  /// @brief Decodes or encodes a streaming close-frame protocol message (zero copy).
  bool ZeroCopyDecode(NoncontiguousBuffer& buffer) override;
  bool ZeroCopyEncode(NoncontiguousBuffer& buffer) override;

  /// @brief Reports whether it is a valid close-frame.
  bool Check() const noexcept;

  /// @brief Returns bytes long of close-frame which is encoded into binary bytes.
  size_t ByteSizeLong() const noexcept override {
    return fixed_header.ByteSizeLong() + stream_close_metadata.ByteSizeLong();
  }

 public:
  // Fixed 16-bytes header of `trpc` protocol.
  TrpcFixedHeader fixed_header;

  // Metadata of close-frame.
  TrpcStreamCloseMeta stream_close_metadata;
};
using TrpcStreamCloseFrameProtocolPtr = std::shared_ptr<TrpcStreamCloseFrameProtocol>;

}  // namespace trpc
