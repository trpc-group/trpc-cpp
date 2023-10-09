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

#include "trpc/compressor/compressor_type.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

/// @brief Compression interfaces for user programing.
namespace trpc::compressor {

/// @brief Initializes compression plugins.
/// @return Returns true on success.
bool Init();

/// @brief Destroy compression plugins, e.g. frees "Compressor" instances.
void Destroy();

/// @brief Compresses the input bytes and put the compressed bytes into output buffer.
/// @param type is the compression algorithm.
/// @param[in] in is the input bytes.
/// @param[out] out saves the compressed bytes.
/// @param level indicates the compression quality.
/// @return Returns true on success, false otherwise. Keep in mind: it always returns false when |type| is "kNone".
bool Compress(CompressType type, const NoncontiguousBuffer& in, NoncontiguousBuffer& out, LevelType level = kDefault);

/// @brief Decompresses the compressed bytes and put the uncompressed bytes into output buffer.
/// @param type is the compression algorithm.
/// @param[in] in is the compressed bytes.
/// @param[out] out saves the uncompressed bytes.
/// @return Returns true on success, false otherwise. Keep in mind: it always returns false when |type| is "kNone".
bool Decompress(CompressType type, const NoncontiguousBuffer& in, NoncontiguousBuffer& out);

/// @brief Compresses the input bytes and put the compressed bytes into output buffer if the |type| is not "kNone".
/// @param type is the compression algorithm.
/// @param data is the input bytes(it will be overwritten if compressed successfully).
/// @param level indicates the compression quality.
/// @return Returns true on success, false otherwise. Keep in mind: it always returns ture when |type| is "kNone".
bool CompressIfNeeded(CompressType type, NoncontiguousBuffer& data, LevelType level = kDefault);

/// @brief Decompresses the compressed bytes and put the uncompressed bytes into output buffer if the |type| is not
/// "kNone".
/// @param type is the compression algorithm.
/// @param data is the input bytes(it will be overwritten if uncompressed successfully).
/// @return Returns true on success, false otherwise. Keep in mind: it always returns ture when |type| is "kNone".
bool DecompressIfNeeded(CompressType type, NoncontiguousBuffer& data);

}  // namespace trpc::compressor
