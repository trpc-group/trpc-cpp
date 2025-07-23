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

#include "trpc/compressor/compressor.h"

#include <limits>

namespace trpc::compressor {

uint64_t Compressor::MaxUncompressedLength() const { return std::numeric_limits<uint64_t>::max(); }

bool Compressor::Compress(const NoncontiguousBuffer& in, NoncontiguousBuffer& out, LevelType level) {
  if (in.ByteSize() <= MaxUncompressedLength()) {
    return DoCompress(in, out, level);
  }
  return false;
}

bool Compressor::Decompress(const NoncontiguousBuffer& in, NoncontiguousBuffer& out) {
  if (in.ByteSize() > 0) {
    return DoDecompress(in, out);
  }
  return false;
}

}  // namespace trpc::compressor
