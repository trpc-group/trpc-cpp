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
#include "trpc/util/ref_ptr.h"

namespace trpc::compressor {

/// @brief Base class for compress/decompress
class Compressor : public RefCounted<Compressor> {
 public:
  Compressor() = default;
  virtual ~Compressor() = default;

  /// @brief Returns the type of compression algorithm. Compression type is used to distinguish different compression
  /// algorithms, it is also used to distinguish different implementations of the same algorithms, e.g. snappy has
  /// stream and block format.
  virtual CompressType Type() const = 0;

  /// @brief Returns the max length of bytes to be uncompressed.
  /// It is limited by compression algorithm. e.g. Snappy has a 4 GB limit.
  virtual uint64_t MaxUncompressedLength() const;

  /// @brief Compresses the input bytes and put the compressed bytes into output buffer.
  /// @param[in] in is the input bytes.
  /// @param[out] out saves the compressed bytes.
  /// @param level indicates the compression quality.
  /// @return Returns true on success, false otherwise.
  bool Compress(const NoncontiguousBuffer& in, NoncontiguousBuffer& out, LevelType level = kDefault);

  /// @brief Decompresses the compressed bytes and put the uncompressed bytes into output buffer.
  ///
  /// @param[in] in is the compressed bytes.
  /// @param[out] out saves the uncompressed bytes.
  /// @return Returns true on success, false otherwise.
  bool Decompress(const NoncontiguousBuffer& in, NoncontiguousBuffer& out);

 protected:
  /// @brief Compresses the input bytes and put the compressed bytes into output buffer.
  /// @param[in] in is the input bytes.
  /// @param[out] out saves the compressed bytes.
  /// @param level indicates the compression quality.
  /// @return Returns true on success, false otherwise.
  virtual bool DoCompress(const NoncontiguousBuffer& in, NoncontiguousBuffer& out, LevelType level) = 0;

  /// @brief Decompresses the compressed bytes and put the uncompressed bytes into output buffer.
  ///
  /// @param[in] in is the compressed bytes.
  /// @param[out] out saves the uncompressed bytes.
  /// @return Returns true on success, false otherwise.
  virtual bool DoDecompress(const NoncontiguousBuffer& in, NoncontiguousBuffer& out) = 0;
};

using CompressorPtr = RefPtr<Compressor>;

}  // namespace trpc::compressor
