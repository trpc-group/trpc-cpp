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

#include "trpc/compressor/compressor_type.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

/// @brief A wrapper of zlib, gzip zlib are available.
namespace trpc::compressor::zlib {

/// @brief Converts compression level to zlib compression level.
/// @param level is compression level use by tRPC frame.
/// @return Returns zlib compression level.
int ConvertLevel(LevelType level);

/// @brief Returns the length of window bits (used by gzip/zlib).
/// @param type is compression algorithm.
int GetWindowBits(CompressType type);

/// @brief Compresses the input bytes and put the compressed bytes into output buffer.
/// @param type is the compression algorithm.
/// @param[in] in is the input bytes.
/// @param[out] out saves the compressed bytes.
/// @param level indicates the compression quality.
/// @return Returns true on success, false otherwise.
bool Compress(CompressType type, const NoncontiguousBuffer& in, NoncontiguousBuffer& out, LevelType level);

/// @brief Decompresses the compressed bytes and put the uncompressed bytes into output buffer.
/// @param type is the compression algorithm.
/// @param[in] in is the compressed bytes.
/// @param[out] out saves the uncompressed bytes.
/// @return Returns true on success, false otherwise.
bool Decompress(CompressType type, const NoncontiguousBuffer& in, NoncontiguousBuffer& out);

}  // namespace trpc::compressor::zlib
