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

#include "trpc/codec/trpc/trpc_protocol.h"

#include <arpa/inet.h>

#include "trpc/util/buffer/zero_copy_stream.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc {

bool TrpcFixedHeader::Decode(NoncontiguousBuffer& buff, bool skip) {
  if (TRPC_UNLIKELY(buff.ByteSize() < TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE)) {
    TRPC_FMT_ERROR("buff.ByteSize:{} less than {}", buff.ByteSize(), TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE);
    return false;
  }

  const char* ptr{nullptr};
  char header_buffer[TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE] = {0};

  // Use first chunk of buffer if it is a full fixed header.
  if (buff.FirstContiguous().size() >= TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE) {
    ptr = buff.FirstContiguous().data();
  } else {
    FlattenToSlow(buff, static_cast<void*>(header_buffer), TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE);
    ptr = header_buffer;
  }

  uint32_t pos = 0;
  memcpy(&magic_value, ptr + pos, sizeof(magic_value));
  magic_value = ntohs(magic_value);
  pos += TrpcFixedHeader::TRPC_PROTO_MAGIC_SPACE;

  memcpy(&data_frame_type, ptr + pos, sizeof(data_frame_type));
  pos += TrpcFixedHeader::TRPC_PROTO_DATAFRAME_TYPE_SPACE;

  memcpy(&stream_frame_type, ptr + pos, sizeof(stream_frame_type));
  pos += TrpcFixedHeader::TRPC_PROTO_STREAMFRAME_TYPE_SPACE;

  memcpy(&data_frame_size, ptr + pos, sizeof(data_frame_size));
  data_frame_size = ntohl(data_frame_size);
  pos += TrpcFixedHeader::TRPC_PROTO_DATAFRAME_SIZE_SPACE;

  memcpy(&pb_header_size, ptr + pos, sizeof(pb_header_size));
  pb_header_size = ntohs(pb_header_size);
  pos += TrpcFixedHeader::TRPC_PROTO_HEADER_SIZE_SPACE;

  memcpy(&stream_id, ptr + pos, sizeof(stream_id));
  stream_id = ntohl(stream_id);
  pos += TrpcFixedHeader::TRPC_PROTO_STREAM_ID_SPACE;

  memcpy(&reversed, ptr + pos, TrpcFixedHeader::TRPC_PROTO_REVERSED_SPACE);
  pos += TrpcFixedHeader::TRPC_PROTO_REVERSED_SPACE;

  if (skip) {
    buff.Skip(TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE);
  }

  return true;
}

bool TrpcFixedHeader::Encode(char* buff) const {
  if (TRPC_UNLIKELY(!buff)) {
    TRPC_LOG_ERROR("Encode buff null error.");
    return false;
  }
  uint16_t magic_value_temp = htons(magic_value);
  memcpy(buff, &magic_value_temp, TrpcFixedHeader::TRPC_PROTO_MAGIC_SPACE);
  buff += TrpcFixedHeader::TRPC_PROTO_MAGIC_SPACE;

  memcpy(buff, &data_frame_type, TrpcFixedHeader::TRPC_PROTO_DATAFRAME_TYPE_SPACE);
  buff += TrpcFixedHeader::TRPC_PROTO_DATAFRAME_TYPE_SPACE;

  memcpy(buff, &stream_frame_type, TrpcFixedHeader::TRPC_PROTO_STREAMFRAME_TYPE_SPACE);
  buff += TrpcFixedHeader::TRPC_PROTO_STREAMFRAME_TYPE_SPACE;

  uint32_t data_frame_size_temp = htonl(data_frame_size);
  memcpy(buff, &data_frame_size_temp, TrpcFixedHeader::TRPC_PROTO_DATAFRAME_SIZE_SPACE);
  buff += TrpcFixedHeader::TRPC_PROTO_DATAFRAME_SIZE_SPACE;

  uint16_t pb_header_size_temp = htons(pb_header_size);
  memcpy(buff, &pb_header_size_temp, TrpcFixedHeader::TRPC_PROTO_HEADER_SIZE_SPACE);
  buff += TrpcFixedHeader::TRPC_PROTO_HEADER_SIZE_SPACE;

  uint32_t stream_id_temp = htonl(stream_id);
  memcpy(buff, &stream_id_temp, TrpcFixedHeader::TRPC_PROTO_STREAM_ID_SPACE);
  buff += TrpcFixedHeader::TRPC_PROTO_STREAM_ID_SPACE;

  memcpy(buff, &reversed, TrpcFixedHeader::TRPC_PROTO_REVERSED_SPACE);

  return true;
}

bool TrpcRequestProtocol::ZeroCopyDecode(NoncontiguousBuffer& buff) {
  if (TRPC_UNLIKELY(!fixed_header.Decode(buff))) {
    TRPC_LOG_ERROR("Decode fixed_header error.");
    return false;
  }

  auto meta = buff.Cut(fixed_header.pb_header_size);

  auto data_frame_size = TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE + fixed_header.pb_header_size + buff.ByteSize();
  if (TRPC_UNLIKELY(fixed_header.data_frame_size != data_frame_size)) {
    TRPC_LOG_ERROR("fixed_header.data_frame_size != data_frame_size)");
    return false;
  }

  NoncontiguousBufferInputStream nbis(&meta);
  if (req_header.ParseFromZeroCopyStream(&nbis)) {
    nbis.Flush();

    if (TRPC_UNLIKELY(buff.ByteSize() < req_header.attachment_size())) {
      TRPC_FMT_ERROR("Decode body and attachment error. res size:{}, attachment_size:{}", buff.ByteSize(),
                     req_header.attachment_size());
      return false;
    }

    req_body = buff.Cut(buff.ByteSize() - req_header.attachment_size());
    req_attachment = std::move(buff);
    return true;
  } else {
    TRPC_LOG_ERROR("Decode req_header ParseFromZeroCopyStream error.");
    return false;
  }
}

bool TrpcRequestProtocol::ZeroCopyEncode(NoncontiguousBuffer& buff) {
  req_header.set_attachment_size(req_attachment.ByteSize());
  auto pb_header_size = req_header.ByteSizeLong();
  fixed_header.pb_header_size = pb_header_size;
  fixed_header.data_frame_size =
      TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE + pb_header_size + req_body.ByteSize() + req_attachment.ByteSize();

  NoncontiguousBufferBuilder builder;
  auto* unaligned_header = builder.Reserve(TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE);
  if (!fixed_header.Encode(unaligned_header)) {
    TRPC_LOG_ERROR("Encode fixed_header error.");
    return false;
  }
  {
    NoncontiguousBufferOutputStream nbos(&builder);
    if (!req_header.SerializePartialToZeroCopyStream(&nbos)) {
      TRPC_LOG_ERROR("Encode rsp_header error.");
      return false;
    }
    nbos.Flush();
  }
  builder.Append(std::move(req_body));
  if (!req_attachment.Empty()) {
    builder.Append(std::move(req_attachment));
  }

  buff = builder.DestructiveGet();
  return true;
}

void TrpcRequestProtocol::SetKVInfo(std::string key, std::string value) {
  auto trans_info = req_header.mutable_trans_info();
  (*trans_info)[std::move(key)] = std::move(value);
}

const TransInfoMap& TrpcRequestProtocol::GetKVInfos() const {
  return req_header.trans_info();
}

google::protobuf::Map<std::string, std::string>* TrpcRequestProtocol::GetMutableKVInfos() {
  return req_header.mutable_trans_info();
}

uint32_t TrpcRequestProtocol::GetMessageSize() const {
  return fixed_header.data_frame_size;
}

bool TrpcResponseProtocol::ZeroCopyDecode(NoncontiguousBuffer& buff) {
  if (TRPC_UNLIKELY(!fixed_header.Decode(buff))) {
    TRPC_LOG_ERROR("Decode fixed_header error.");
    return false;
  }

  auto meta = buff.Cut(fixed_header.pb_header_size);

  NoncontiguousBufferInputStream nbis(&meta);
  if (rsp_header.ParseFromZeroCopyStream(&nbis)) {
    nbis.Flush();

    if (TRPC_UNLIKELY(buff.ByteSize() < rsp_header.attachment_size())) {
      TRPC_FMT_ERROR("Decode body and attachment error. res size:{}, attachment_size:{}", buff.ByteSize(),
                     rsp_header.attachment_size());
      return false;
    }

    rsp_body = buff.Cut(buff.ByteSize() - rsp_header.attachment_size());
    rsp_attachment = std::move(buff);
    return true;
  } else {
    TRPC_LOG_ERROR("Decode req_header ParseFromZeroCopyStream error.");
    return false;
  }
}

bool TrpcResponseProtocol::ZeroCopyEncode(NoncontiguousBuffer& buff) {
  rsp_header.set_attachment_size(rsp_attachment.ByteSize());
  uint32_t rsp_header_size = rsp_header.ByteSizeLong();
  uint32_t buff_size =
      TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE + rsp_header_size + rsp_body.ByteSize() + rsp_attachment.ByteSize();
  fixed_header.data_frame_size = buff_size;
  fixed_header.pb_header_size = rsp_header_size;

  NoncontiguousBufferBuilder builder;
  auto* unaligned_header = builder.Reserve(TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE);
  if (TRPC_UNLIKELY(!fixed_header.Encode(unaligned_header))) {
    TRPC_LOG_ERROR("Encode fixed_header error.");
    return false;
  }

  {
    NoncontiguousBufferOutputStream nbos(&builder);
    if (TRPC_UNLIKELY(!rsp_header.SerializePartialToZeroCopyStream(&nbos))) {
      TRPC_LOG_ERROR("Encode rsp_header error.");
      return false;
    }
    nbos.Flush();
  }

  builder.Append(std::move(rsp_body));
  if (!rsp_attachment.Empty()) {
    builder.Append(std::move(rsp_attachment));
  }

  buff = builder.DestructiveGet();
  return true;
}

void TrpcResponseProtocol::SetKVInfo(std::string key, std::string value) {
  auto trans_info = rsp_header.mutable_trans_info();
  (*trans_info)[std::move(key)] = std::move(value);
}

const TransInfoMap& TrpcResponseProtocol::GetKVInfos() const {
  return rsp_header.trans_info();
}

google::protobuf::Map<std::string, std::string>* TrpcResponseProtocol::GetMutableKVInfos() {
  return rsp_header.mutable_trans_info();
}

uint32_t TrpcResponseProtocol::GetMessageSize() const {
  return fixed_header.data_frame_size;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace {
// @brief Reports whether type of data-frame is ok.
bool CheckTrpcDataFrameType(const TrpcFixedHeader& fixed_header, const TrpcDataFrameType& expect) {
  if (TRPC_UNLIKELY(expect != fixed_header.data_frame_type)) {
    TRPC_LOG_ERROR("invalid data frame type"
                   << ", expect:" << static_cast<int>(expect)
                   << ", actual:" << static_cast<uint32_t>(fixed_header.data_frame_type) << ".");
    return false;
  }
  return true;
}
}  // namespace

namespace {
// @brief Reports whether type of streaming-frame is ok.
bool CheckTrpcStreamFrameType(const TrpcFixedHeader& fixed_header, const TrpcStreamFrameType& expect) {
  if (TRPC_UNLIKELY(expect != fixed_header.stream_frame_type)) {
    TRPC_LOG_ERROR("invalid stream frame type"
                   << ", expect:" << static_cast<int>(expect)
                   << ", actual:" << static_cast<uint32_t>(fixed_header.stream_frame_type) << ".");
    return false;
  }
  return true;
}
}  // namespace

namespace {
// @brief Encodes streaming frame protocol message. It is available for frame of INIT, CLOSE, FEEDBACK.
bool EncodeStreamFrame(const TrpcFixedHeader& fixed_header, const google::protobuf::Message& pb,
                              NoncontiguousBufferBuilder* builder) {
  auto* header_buffer = builder->Reserve(fixed_header.ByteSizeLong());

  if (TRPC_UNLIKELY(!fixed_header.Encode(header_buffer))) {
    TRPC_LOG_ERROR("encode fixed header of stream frame failed");
    return false;
  }
  {
    NoncontiguousBufferOutputStream nbos(builder);
    if (!pb.SerializePartialToZeroCopyStream(&nbos)) {
      TRPC_LOG_ERROR("failed to serialize metadata of stream frame failed.");
      return false;
    }
    nbos.Flush();
  }
  return true;
}
}  // namespace

namespace {
// @brief Decodes streaming frame, It is available for frame of INIT, CLOSE, FEEDBACK.
bool DecodeStreamFrame(NoncontiguousBuffer* buffer, TrpcFixedHeader* fixed_header,
                              google::protobuf::Message* pb) {
  if (TRPC_UNLIKELY(!fixed_header->Decode(*buffer))) {
    TRPC_LOG_ERROR("decode fixed header of stream frame failed.");
    return false;
  }
  auto metadata_size = fixed_header->data_frame_size - fixed_header->ByteSizeLong();
  if (metadata_size > buffer->ByteSize()) {
    TRPC_LOG_ERROR("no enough bytes to construct stream metadata.");
    return false;
  }
  auto metadata = buffer->Cut(metadata_size);
  NoncontiguousBufferInputStream nbis(&metadata);
  if (TRPC_UNLIKELY(!pb->ParseFromZeroCopyStream(&nbis))) {
    TRPC_LOG_ERROR("failed to deserialize metadata of stream frame.");
    return false;
  }
  nbis.Flush();
  return true;
}
}  // namespace

bool TrpcStreamInitFrameProtocol::ZeroCopyDecode(NoncontiguousBuffer& buffer) {
  if (!DecodeStreamFrame(&buffer, &fixed_header, &stream_init_metadata)) {
    return false;
  }
  if (TRPC_UNLIKELY(!Check())) {
    return false;
  }
  return true;
}

bool TrpcStreamInitFrameProtocol::ZeroCopyEncode(NoncontiguousBuffer& buffer) {
  if (TRPC_UNLIKELY(!Check())) {
    return false;
  }
  fixed_header.data_frame_size = ByteSizeLong();
  NoncontiguousBufferBuilder builder;
  if (!EncodeStreamFrame(fixed_header, stream_init_metadata, &builder)) {
    return false;
  }
  buffer = builder.DestructiveGet();
  return true;
}

bool TrpcStreamInitFrameProtocol::Check() const noexcept {
  if (!CheckTrpcDataFrameType(fixed_header, TrpcDataFrameType::TRPC_STREAM_FRAME) ||
      !CheckTrpcStreamFrameType(fixed_header, TrpcStreamFrameType::TRPC_STREAM_FRAME_INIT)) {
    return false;
  }
  return true;
}

bool TrpcStreamDataFrameProtocol::ZeroCopyDecode(NoncontiguousBuffer& buffer) {
  if (TRPC_UNLIKELY(!fixed_header.Decode(buffer))) {
    TRPC_LOG_ERROR("decode fixed header of stream frame failed.");
    return false;
  }
  body = buffer.Cut(fixed_header.data_frame_size - fixed_header.ByteSizeLong());
  return true;
}

bool TrpcStreamDataFrameProtocol::ZeroCopyEncode(NoncontiguousBuffer& buffer) {
  if (TRPC_UNLIKELY(!Check())) {
    return false;
  }
  fixed_header.data_frame_size = ByteSizeLong();
  NoncontiguousBufferBuilder builder;
  auto* header_buffer = builder.Reserve(fixed_header.ByteSizeLong());
  if (TRPC_UNLIKELY(!fixed_header.Encode(header_buffer))) {
    TRPC_LOG_ERROR("encode fixed header of stream frame failed");
    return false;
  }
  builder.Append(std::move(body));
  buffer = builder.DestructiveGet();
  return true;
}

bool TrpcStreamDataFrameProtocol::Check() const noexcept {
  if (!CheckTrpcDataFrameType(fixed_header, TrpcDataFrameType::TRPC_STREAM_FRAME) ||
      !CheckTrpcStreamFrameType(fixed_header, TrpcStreamFrameType::TRPC_STREAM_FRAME_DATA)) {
    return false;
  }
  return true;
}

bool TrpcStreamFeedbackFrameProtocol::ZeroCopyDecode(NoncontiguousBuffer& buffer) {
  if (!DecodeStreamFrame(&buffer, &fixed_header, &stream_feedback_metadata)) {
    return false;
  }
  if (TRPC_UNLIKELY(!Check())) {
    return false;
  }
  return true;
}

bool TrpcStreamFeedbackFrameProtocol::ZeroCopyEncode(NoncontiguousBuffer& buffer) {
  if (TRPC_UNLIKELY(!Check())) {
    return false;
  }
  fixed_header.data_frame_size = ByteSizeLong();
  NoncontiguousBufferBuilder builder;
  if (!EncodeStreamFrame(fixed_header, stream_feedback_metadata, &builder)) {
    return false;
  }
  buffer = builder.DestructiveGet();
  return true;
}

bool TrpcStreamFeedbackFrameProtocol::Check() const noexcept {
  if (!CheckTrpcDataFrameType(fixed_header, TrpcDataFrameType::TRPC_STREAM_FRAME) ||
      !CheckTrpcStreamFrameType(fixed_header, TrpcStreamFrameType::TRPC_STREAM_FRAME_FEEDBACK)) {
    return false;
  }
  return true;
}

bool TrpcStreamCloseFrameProtocol::ZeroCopyDecode(NoncontiguousBuffer& buffer) {
  if (!DecodeStreamFrame(&buffer, &fixed_header, &stream_close_metadata)) {
    return false;
  }
  if (TRPC_UNLIKELY(!Check())) {
    return false;
  }
  return true;
}

bool TrpcStreamCloseFrameProtocol::ZeroCopyEncode(NoncontiguousBuffer& buffer) {
  if (TRPC_UNLIKELY(!Check())) {
    return false;
  }
  fixed_header.data_frame_size = ByteSizeLong();
  NoncontiguousBufferBuilder builder;
  if (!EncodeStreamFrame(fixed_header, stream_close_metadata, &builder)) {
    return false;
  }
  buffer = builder.DestructiveGet();
  return true;
}

bool TrpcStreamCloseFrameProtocol::Check() const noexcept {
  if (!CheckTrpcDataFrameType(fixed_header, TrpcDataFrameType::TRPC_STREAM_FRAME) ||
      !CheckTrpcStreamFrameType(fixed_header, TrpcStreamFrameType::TRPC_STREAM_FRAME_CLOSE)) {
    return false;
  }
  return true;
}

}  // namespace trpc
