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

#include "trpc/compressor/trpc_compressor.h"

#include <utility>

#include "trpc/compressor/compressor_factory.h"
#include "trpc/compressor/gzip/gzip_compressor.h"
#include "trpc/compressor/lz4/lz4_compressor.h"
#include "trpc/compressor/snappy/snappy_compressor.h"
#include "trpc/compressor/zlib/zlib_compressor.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc::compressor {

bool Init() {
  auto* factory = CompressorFactory::GetInstance();
  // gzip
  TRPC_ASSERT(factory->Register(MakeRefCounted<GzipCompressor>()));
  // zlib
  TRPC_ASSERT(factory->Register(MakeRefCounted<ZlibCompressor>()));

  // snappy
  TRPC_ASSERT(factory->Register(MakeRefCounted<SnappyCompressor>()));
  // snappy block format
  TRPC_ASSERT(factory->Register(MakeRefCounted<SnappyBlockCompressor>()));

  // lz4 frame
  TRPC_ASSERT(factory->Register(MakeRefCounted<Lz4FrameCompressor>()));

  return true;
}

void Destroy() {
  CompressorFactory::GetInstance()->Clear();
}

bool CompressIfNeeded(CompressType type, NoncontiguousBuffer& data, LevelType level) {
  if (type == kNone) {
    return true;
  }
  NoncontiguousBuffer out;
  if (TRPC_UNLIKELY(!Compress(type, data, out, level))) {
    return false;
  }
  data = std::move(out);
  return true;
}

bool Compress(CompressType type, const NoncontiguousBuffer& in, NoncontiguousBuffer& out, LevelType level) {
  // Returns false on compressor::kNone
  auto compressor = CompressorFactory::GetInstance()->Get(type);
  if (TRPC_UNLIKELY(!compressor)) {
    return false;
  }
  return compressor->Compress(in, out, level);
}

bool DecompressIfNeeded(CompressType type, NoncontiguousBuffer& data) {
  if (type == kNone) {
    return true;
  }
  NoncontiguousBuffer out;
  if (TRPC_UNLIKELY(!Decompress(type, data, out))) {
    return false;
  }
  data = std::move(out);
  return true;
}

bool Decompress(CompressType type, const NoncontiguousBuffer& in, NoncontiguousBuffer& out) {
  // Returns false on  compressor::kNone
  auto compressor = CompressorFactory::GetInstance()->Get(type);
  if (TRPC_UNLIKELY(!compressor)) {
    return false;
  }
  return compressor->Decompress(in, out);
}

}  // namespace trpc::compressor
