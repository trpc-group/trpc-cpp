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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. flare
// Copyright (C) 2019 THL A29 Limited, a Tencent company.
// flare is licensed under the BSD 3-Clause License.
//

#include "trpc/util/buffer/zero_copy_stream.h"

#include "trpc/util/check.h"

namespace trpc {

// The following source codes are from flare.
// Copied and modified from
// https://github.com/Tencent/flare/blob/master/flare/base/buffer/zero_copy_stream.cc.

NoncontiguousBufferInputStream::NoncontiguousBufferInputStream(NoncontiguousBuffer* ref)
    : buffer_(ref) {}

NoncontiguousBufferInputStream::~NoncontiguousBufferInputStream() { Flush(); }

void NoncontiguousBufferInputStream::Flush() { PerformPendingSkips(); }

bool NoncontiguousBufferInputStream::Next(const void** data, int* size) {
  PerformPendingSkips();
  if (buffer_->Empty()) {
    return false;
  }

  auto&& fc = buffer_->FirstContiguous();
  *data = fc.data();
  *size = fc.size();

  skip_before_read_ = fc.size();
  read_ += fc.size();
  return true;
}

void NoncontiguousBufferInputStream::BackUp(int count) {
  TRPC_ASSERT(skip_before_read_ >= static_cast<std::size_t>(count));
  skip_before_read_ -= count;
  read_ -= count;
}

bool NoncontiguousBufferInputStream::Skip(int count) {
  PerformPendingSkips();

  if (buffer_->ByteSize() < static_cast<std::size_t>(count)) {
    return false;
  }
  read_ += count;
  buffer_->Skip(count);
  return true;
}

google::protobuf::int64 NoncontiguousBufferInputStream::ByteCount() const { return read_; }

void NoncontiguousBufferInputStream::PerformPendingSkips() {
  if (skip_before_read_) {
    buffer_->Skip(skip_before_read_);
    skip_before_read_ = 0;
  }
}

NoncontiguousBufferOutputStream::NoncontiguousBufferOutputStream(
    NoncontiguousBufferBuilder* builder)
    : builder_(builder) {}

NoncontiguousBufferOutputStream::~NoncontiguousBufferOutputStream() { Flush(); }

void NoncontiguousBufferOutputStream::Flush() {
  if (using_bytes_) {
    builder_->MarkWritten(using_bytes_);
    using_bytes_ = 0;
  }
}

bool NoncontiguousBufferOutputStream::Next(void** data, int* size) {
  if (using_bytes_) {
    builder_->MarkWritten(using_bytes_);
  }
  *data = builder_->data();
  *size = builder_->SizeAvailable();
  using_bytes_ = *size;
  TRPC_CHECK(*size);
  return true;
}

void NoncontiguousBufferOutputStream::BackUp(int count) { using_bytes_ -= count; }

google::protobuf::int64 NoncontiguousBufferOutputStream::ByteCount() const {
  return builder_->ByteSize();
}

}  // namespace trpc
