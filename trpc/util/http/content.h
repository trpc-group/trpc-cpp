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

#include <optional>
#include <string>

#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc::http {

/// @brief A simple wrapper for reading or writing the body of HTTP message.
/// @private For internal use purpose only.
class ContentProvider {
 public:
  /// @brief Reads the message content.
  ///
  /// @param data is message read buffer.
  /// @param len is buffer length.
  ///
  /// @return >0: Successfully read byte count; 0: EOF; -1: error.
  ssize_t Read(uint8_t* data, std::size_t len);

  /// @brief Writes chunked message content.
  ///
  /// @param data is the message buffer.
  /// @param len is data length (if no more data, set len=0 to indicate EOF).
  /// @return returns the byte size count actually written to.
  ssize_t Write(const uint8_t* data, std::size_t len);

  /// @brief Gets or sets the sequence number of the request message.
  void SetSequenceId(uint32_t sequence_id) { sequence_id_ = sequence_id; }
  uint32_t GetSequenceId() const { return sequence_id_; }

  /// @brief Gets or Sets request content (contiguous buffer).
  const std::string& GetContent() const { return GetCachedContent(); }
  std::string* GetMutableContent() { return &GetCachedContent(); }
  void SetContent(std::string&& content) { content_ = std::move(content); }

  /// @brief Gets or Sets request content (non-contiguous buffer, better performance).
  const NoncontiguousBuffer& GetNonContiguousBufferContent() const { return content_buffer_; }
  NoncontiguousBuffer* GetMutableNonContiguousBufferContent() { return &content_buffer_; }
  void SetNonContiguousBufferContent(NoncontiguousBuffer&& content) { content_buffer_ = std::move(content); }

  /// @brief Gets or Sets content length.
  std::size_t ContentLength() const {
    return content_length_.value_or(content_ ? content_->size() : content_buffer_.ByteSize());
  }
  void SetContentLength(std::optional<size_t> content_length) { content_length_ = content_length; }

  std::string SerializeToString() const;
  void SerializeToString(NoncontiguousBuffer& buff) const&;
  void SerializeToString(NoncontiguousBuffer& buff) &&;

 private:
  std::string& GetCachedContent() const {
    if (!content_) content_ = FlattenSlow(content_buffer_);
    return content_.value();
  }

  template <typename T>
  static void SerializeToStringImpl(T&& self, NoncontiguousBuffer& buff);

 private:
  std::optional<size_t> content_length_;
  // Response content (contiguous buffer).
  mutable std::optional<std::string> content_;
  // Response content (non-contiguous buffer, better performance).
  NoncontiguousBuffer content_buffer_;
  // Message content sequence identifier.
  uint32_t sequence_id_{0};
};

}  // namespace trpc::http
