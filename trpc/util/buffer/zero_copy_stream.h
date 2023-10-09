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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. flare
// Copyright (C) 2019 THL A29 Limited, a Tencent company.
// flare is licensed under the BSD 3-Clause License.
//

#pragma once

#include <cstddef>

#include "google/protobuf/io/zero_copy_stream.h"

#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

// The following source codes are from flare.
// Copied and modified from
// https://github.com/Tencent/flare/blob/master/flare/base/buffer/zero_copy_stream.h.

/// @brief This class provides you a way to parse Protocol Buffers from
/// `NoncontiguousBuffer` without flattening the later.
///
/// The buffer given to this class is consumed. You need to make a copy before
/// hand if that's not what you want.
///
/// Usage:
///
/// NoncontiguousBufferInputStream nbis(&buffer);
/// TRPC_ASSERT(message.ParseFromZeroCopyStream(&nbis));
/// nbis.Flush();
///
class NoncontiguousBufferInputStream
    : public google::protobuf::io::ZeroCopyInputStream {
 public:
  explicit NoncontiguousBufferInputStream(NoncontiguousBuffer* ref);
  ~NoncontiguousBufferInputStream();

  /// @brief Synchronizes with `NoncontiguousBuffer`.
  /// You must call this method before touching the buffer again.
  /// On destruction, the stream is automatically synchronized with the buffer.
  void Flush();

  bool Next(const void** data, int* size) override;
  void BackUp(int count) override;
  bool Skip(int count) override;
  google::protobuf::int64 ByteCount() const override;

 private:
  void PerformPendingSkips();

 private:
  std::size_t skip_before_read_{0};
  std::size_t read_{0};
  NoncontiguousBuffer* buffer_;
};

/// @brief This class provides you a way to serialize Protocol Buffers into
/// `NoncontiguousBuffer`.
///
/// Usage:
///
/// NoncontiguousBufferOutputStream nbos;
/// TRPC_ASSERT(message.SerializeToZeroCopyStream(&nbos));
/// nbos.Flush();
///
class NoncontiguousBufferOutputStream
    : public google::protobuf::io::ZeroCopyOutputStream {
 public:
  explicit NoncontiguousBufferOutputStream(NoncontiguousBufferBuilder* builder);
  ~NoncontiguousBufferOutputStream();

  /// @brief Flush internal state. Must be called before touching `builder`.
  /// On destruction, we automatically synchronizes with `builder`.
  void Flush();

  bool Next(void** data, int* size) override;
  void BackUp(int count) override;
  google::protobuf::int64 ByteCount() const override;

 private:
  std::size_t using_bytes_{};
  NoncontiguousBufferBuilder* builder_;
};

}  // namespace trpc
