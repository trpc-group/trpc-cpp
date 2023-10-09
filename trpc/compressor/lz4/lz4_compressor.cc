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

#include "trpc/compressor/lz4/lz4_compressor.h"

#include <string.h>

#include <utility>

#include "trpc/compressor/lz4/lz4_util.h"
#include "trpc/util/buffer/zero_copy_stream.h"

namespace trpc::compressor {

bool Lz4FrameCompressor::DoCompress(const trpc::NoncontiguousBuffer& in, trpc::NoncontiguousBuffer& out,
                                    LevelType level) {
  LZ4F_compressionContext_t ctx;
  const size_t ctx_status = LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);
  if (LZ4F_isError(ctx_status)) {
    return false;
  }
  bool result = lz4::DoCompress(ctx, in, out);
  LZ4F_freeCompressionContext(ctx);
  return result;
}

bool Lz4FrameCompressor::DoDecompress(const trpc::NoncontiguousBuffer& in, trpc::NoncontiguousBuffer& out) {
  LZ4F_dctx* ctx;
  const size_t ctx_status = LZ4F_createDecompressionContext(&ctx, LZ4F_VERSION);
  if (LZ4F_isError(ctx_status)) {
    TRPC_FMT_ERROR("LZ4F_dctx creation error: {}", LZ4F_getErrorName(ctx_status));
    return false;
  }
  bool result = lz4::DoDecompress(ctx, in, out);
  LZ4F_freeDecompressionContext(ctx);
  return result;
}

}  // namespace trpc::compressor
