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

#include <cstdint>

#include "trpc/codec/trpc/trpc.pb.h"

namespace trpc::compressor {

/// @brief There are multi methods to compress data. Compression type is used to distinguish different compression
/// algorithms. It is expressed as integer in the range of 0 - 255 ( 0 - 127 is are planed for use by the framework).
using CompressType = uint8_t;

/// @brief Type 0 is the default, it means no compression is used.
constexpr CompressType kNone = TrpcCompressType::TRPC_DEFAULT_COMPRESS;
/// @brief Type 1 is Gzip.
constexpr CompressType kGzip = TrpcCompressType::TRPC_GZIP_COMPRESS;
/// @brief Type 2 is Snappy.
constexpr CompressType kSnappy = TrpcCompressType::TRPC_SNAPPY_COMPRESS;
/// @brief Type 3 is Zlib.
constexpr CompressType kZlib = TrpcCompressType::TRPC_ZLIB_COMPRESS;
/// @brief Type 5 is Snappy in block format (stream format is not available).
constexpr CompressType kSnappyBlock = TrpcCompressType::TRPC_SNAPPY_BLOCK_COMPRESS;
/// @brief lz4 frame.
constexpr CompressType kLz4Frame = TrpcCompressType::TRPC_LZ4_FRAME_COMPRESS;
/// @brief It is not a compression algorithm, it is the number of compression algorithms.
constexpr CompressType kMaxType{255};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// @brief The compression level is a measure of the compression quality. It is expressed as an integer.
using LevelType = uint8_t;

/// @brief Level 0 provides the best performance at the expense of quality.
constexpr LevelType kFastest{0};
/// @brief Level 2 provides the best quality at the expense of performance.
constexpr LevelType kBest{2};
/// @brief Compression quality and performance are conflicting goals. Level 1 is a compromise.
constexpr LevelType kDefault{1};

}  // namespace trpc::compressor
