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

#include "lz4frame.h"

#include "trpc/compressor/compressor_type.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc::compressor::lz4 {

/// @brief Lz4 compress
/// @param ctx Compress context
/// @param[in] in Data before compressed
/// @param[out] out Data after compressed
/// @return true: success, false: failed
bool DoCompress(LZ4F_compressionContext_t& ctx, const NoncontiguousBuffer& in, NoncontiguousBuffer& out);

/// @brief Lz4 decompress
/// @param ctx Compress context
/// @param[in] in Data before decompressed
/// @param[out] out Data after decompressed
/// @return true: success, false: failed
bool DoDecompress(LZ4F_dctx* ctx, const NoncontiguousBuffer& in, NoncontiguousBuffer& out);

}  // namespace trpc::compressor::lz4
