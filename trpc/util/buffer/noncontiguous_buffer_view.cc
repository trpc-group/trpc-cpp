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

#include "trpc/util/buffer/noncontiguous_buffer_view.h"

namespace trpc::detail {

// The following source codes are from flare.
// Copied and modified from
// https://github.com/Tencent/flare/blob/master/flare/base/buffer/view.h.
namespace {
using BufferViewConstIter = NoncontiguousBufferView::const_iterator;
}  // namespace

const char& BufferViewConstIter::operator*() const noexcept { return cur_->data()[pos_]; }

const char* BufferViewConstIter::operator->() const noexcept { return cur_->data() + pos_; }

BufferViewConstIter& BufferViewConstIter::operator++() noexcept {
  if (cur_ != end_) {
    ++pos_;
    ++distance_;
    if (pos_ >= cur_->size()) {
      pos_ = 0;
      ++cur_;
    }
  }
  return *this;
}

BufferViewConstIter BufferViewConstIter::operator+(BufferViewConstIter::difference_type n) noexcept {
  BufferViewConstIter it = *this;
  ssize_t diff = 0;
  while (n > 0) {
    if (!(it.cur_ != it.end_)) {
      return it;
    }
    diff = it.cur_->size() - it.pos_;
    if (diff > n) {
      it.pos_ += n;
      it.distance_ += n;
      return it;
    }
    ++it.cur_;
    it.pos_ = 0;
    it.distance_ += diff;
    n -= diff;
  }
  return it;
}

bool BufferViewConstIter::operator!=(const BufferViewConstIter& other) const noexcept {
  return cur_ != other.cur_ || pos_ != other.pos_ || distance_ != other.distance_;
}

bool BufferViewConstIter::operator==(const BufferViewConstIter& other) const noexcept { return !(*this != other); }

std::ptrdiff_t BufferViewConstIter::operator-(const BufferViewConstIter& other) const noexcept {
  return distance_ - other.distance_;
}

std::string NoncontiguousBufferView::GetString(const const_iterator& iter, std::size_t len) {
  std::string s;
  NoncontiguousBuffer::const_iterator cur_block = iter.cur_;
  std::size_t cur_pos = iter.pos_;
  while (cur_block != end_) {
    std::size_t diff = cur_block->size() - cur_pos;
    if (len < diff) {
      s.append(cur_block->data() + cur_pos, len);
      break;
    }
    s.append(cur_block->data() + cur_pos, diff);
    len -= diff;
    cur_pos = 0;
    ++cur_block;
  }
  return s;
}

}  // namespace trpc::detail
