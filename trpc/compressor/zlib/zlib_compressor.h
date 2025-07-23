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

#include "trpc/compressor/common/zlib_util.h"
#include "trpc/compressor/compressor.h"

namespace trpc::compressor {

/// @brief Implementation of zlib compress/decompress
class ZlibCompressor : public Compressor {
 public:
  CompressType Type() const override { return kZlib; }

 protected:
  bool DoCompress(const NoncontiguousBuffer& in, NoncontiguousBuffer& out, LevelType level) override {
    return zlib::Compress(kZlib, in, out, level);
  }

  bool DoDecompress(const NoncontiguousBuffer& in, NoncontiguousBuffer& out) override {
    return zlib::Decompress(kZlib, in, out);
  }
};

}  // namespace trpc::compressor
