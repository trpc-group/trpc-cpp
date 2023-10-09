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

#include "trpc/compressor/snappy/snappy_compressor.h"

#include <string.h>

#include <utility>

#include "snappy-sinksource.h"
#include "snappy.h"

#include "trpc/compressor/common/util.h"

namespace trpc::compressor {

namespace {

/// @brief A wrapper of input bytes stream for non-contiguous buffer.
class SnappyInputSource : public snappy::Source {
 public:
  explicit SnappyInputSource(trpc::NoncontiguousBuffer b) : buffer_(std::move(b)) {}

  [[nodiscard]] size_t Available() const override { return buffer_.ByteSize(); }

  /// @brief Returns the first continguous buffer.
  /// @param[out] len is size of the buffer.
  /// @return Returns the pointer of buffer if buffer is not empty, nullptr otherwise.
  const char* Peek(size_t* len) override {
    if (!Available()) {
      *len = 0;
      return nullptr;
    }
    auto&& c = buffer_.FirstContiguous();
    *len = c.size();
    return c.data();
  }

  /// @brief Skip n bytes in buffer. The skipped bytes will be dropped.
  void Skip(size_t n) override { buffer_.Cut(n); }

 private:
  trpc::NoncontiguousBuffer buffer_;
};

/// @brief A bytes stream to store compressed bytes or uncmpressed bytes processed by snappy algorithm.
class SnappyOutputSink : public snappy::Sink {
 public:
  explicit SnappyOutputSink(trpc::NoncontiguousBufferBuilder* out) : out_(out) {}

  ~SnappyOutputSink() override { Flush(); }

  /// @bried Appends a buffer to stream.
  void Append(const char* data, size_t n) override {
    if (data == next_data_) {
      int len = static_cast<int>(n);
      TRPC_ASSERT(len <= next_size_);
      out_.BackUp(next_size_ - len);
    } else {
      out_.BackUp(next_size_);
      // Copy bytes to stream if needed (Block size of NoncontiguousBuffer is 4 KB).
      TRPC_ASSERT(CopyDataToOutputStream(data, n, &out_));
    }
  }

  /// @brief Gets a available contiguous buffer.
  /// @param length is expected size of the buffer.
  /// @param scratch is the contiguous buffer who's size is greater than |length|, transfered by snappy.
  /// @return Returns the pointer of a available contiguous buffer. It is always available.
  /// @note A buffer block (contiguous buffer) in noncontiguous buffer is returned if it is available, |scratch| will
  /// be returned otherwise.
  char* GetAppendBuffer(size_t length, char* scratch) override {
    TRPC_ASSERT(out_.Next(&next_data_, &next_size_));
    if (next_size_ >= static_cast<int>(length)) {
      return reinterpret_cast<char*>(next_data_);
    } else {
      return scratch;
    }
  }

  /// @brief Flush the internal state of stream. It must be called when compressed or uncompressed successfully.
  /// It is also RAII.
  void Flush() { out_.Flush(); }

 private:
  trpc::NoncontiguousBufferOutputStream out_;
  void* next_data_ = nullptr;
  int next_size_ = 0;
};

}  // namespace

bool SnappyCompressor::DoCompress(const trpc::NoncontiguousBuffer& in, trpc::NoncontiguousBuffer& out,
                                  LevelType level) {
  SnappyInputSource source(in);
  trpc::NoncontiguousBufferBuilder builder;
  SnappyOutputSink sink(&builder);
  snappy::Compress(&source, &sink);
  sink.Flush();
  out = builder.DestructiveGet();
  return true;
}

bool SnappyCompressor::DoDecompress(const trpc::NoncontiguousBuffer& in, trpc::NoncontiguousBuffer& out) {
  SnappyInputSource source(in);
  trpc::NoncontiguousBufferBuilder builder;
  SnappyOutputSink sink(&builder);
  if (!snappy::Uncompress(&source, &sink)) {
    return false;
  }
  sink.Flush();
  out = builder.DestructiveGet();
  return true;
}

}  // namespace trpc::compressor
