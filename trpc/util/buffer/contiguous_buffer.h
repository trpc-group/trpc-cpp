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

#include <cstddef>

#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

/// @brief Implementation of contiguous buffer
class ContiguousBuffer : public RefCounted<ContiguousBuffer> {
 public:
  enum class WorkMode { BUFFER_INNER, BUFFER_DELEGATE_READONLY, BUFFER_DELEGATE };

 public:
  explicit ContiguousBuffer(size_t initial_size = 8196);
  /// @note mem_ptr is allocated by new[]
  ContiguousBuffer(char* mem_ptr, size_t size);
  ContiguousBuffer(ContiguousBuffer&& other);
  ContiguousBuffer& operator=(ContiguousBuffer&& other);
  ContiguousBuffer(const ContiguousBuffer& other) = delete;
  ContiguousBuffer& operator=(const ContiguousBuffer& other) = delete;
  ~ContiguousBuffer();

  /// @brief Get buffer readable ptr
  const char* GetReadPtr() const {
    return mem_ptr_ + read_pos_;
  }

  /// @brief After reading data from the buffer, update the size of the buffer read offset
  void AddReadLen(size_t len) {
    TRPC_ASSERT(len <= ReadableSize());
    read_pos_ += len;
  }

  /// @brief Get the size of readable data
  size_t ReadableSize() const {
    TRPC_ASSERT(read_pos_ <= write_pos_);
    return write_pos_ - read_pos_;
  }

  /// @brief Get buffer writable ptr
  char* GetWritePtr() {
    return mem_ptr_ + write_pos_;
  }

  /// @brief After writing data to the buffer, update the size of the buffer write offset
  void AddWriteLen(size_t len) {
    write_pos_ += len;
    TRPC_ASSERT(write_pos_ <= mem_size_);
  }

  /// @brief Get the size of writable data
  size_t WritableSize() const {
    TRPC_ASSERT(write_pos_ <= mem_size_);
    return mem_size_ - write_pos_;
  }

  /// @brief Reset buffer size
  void Resize(size_t size) noexcept;

  /// @brief Reset the buffer read offset
  void ResetReadPos(size_t read_pos = 0) {
    read_pos_ = read_pos;
  }

  /// @brief Reset the buffer write offset
  void ResetWritePos(size_t write_pos = 0) {
    write_pos_ = write_pos;
  }

  /// @brief Release buffer ownership
  void DestructiveReset();

 private:
  char* Begin() { return mem_ptr_; }
  void Reset();

 private:
  size_t mem_size_{0};
  size_t read_pos_{0};
  size_t write_pos_{0};
  char* mem_ptr_{nullptr};
};

// for compatible
using Buffer = ContiguousBuffer;
using BufferPtr = RefPtr<Buffer>;

}  // namespace trpc
