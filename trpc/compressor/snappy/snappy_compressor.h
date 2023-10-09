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

#include "trpc/compressor/compressor.h"

namespace trpc::compressor {

/// @brief Implementation of snappy compress/decompress
class SnappyCompressor : public Compressor {
 public:
  [[nodiscard]] CompressType Type() const override { return kSnappy; }

 protected:
  /// @note It may copy bytes if the size of buffer required by snappy algorigthm is greater than 4 KB.
  bool DoCompress(const trpc::NoncontiguousBuffer& in, trpc::NoncontiguousBuffer& out, LevelType level) override;

  /// @note It may copy bytes if the size of buffer required by snappy algorigthm is greater than 4 KB.
  bool DoDecompress(const trpc::NoncontiguousBuffer& in, trpc::NoncontiguousBuffer& out) override;
};

/// @brief Snappy compression algorithm in block format.
class SnappyBlockCompressor : public SnappyCompressor {
 public:
  [[nodiscard]] CompressType Type() const override { return kSnappyBlock; }
};

}  // namespace trpc::compressor
