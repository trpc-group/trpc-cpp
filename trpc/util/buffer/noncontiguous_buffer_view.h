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

#pragma once

#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc::detail {

// The following source codes are from flare.
// Copied and modified from
// https://github.com/Tencent/flare/blob/master/flare/base/buffer/view.h.

/// @brief Bytes view of NoncontiguousBuffer. Implement the basic operation of reading each byte through an iterator on
///        a NoncontiguousBuffer.
/// @note Currently it is only used within the framework and should not be used by users.
///       During iteration, the NoncontiguousBuffer should not be changed.
class NoncontiguousBufferView {
 public:
  /// @brief This iterator is a const forward iterator.
  class const_iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = const char;
    using pointer = const char*;
    using reference = const char&;

    const_iterator() noexcept = default;
    const_iterator(const const_iterator& from) noexcept
        : cur_(from.cur_), end_(from.end_), pos_(from.pos_), distance_(from.distance_) {}

    reference operator*() const noexcept;
    pointer operator->() const noexcept;
    const_iterator& operator++() noexcept;
    const_iterator operator+(difference_type n) noexcept;
    bool operator!=(const const_iterator& other) const noexcept;
    bool operator==(const const_iterator& other) const noexcept;
    difference_type operator-(const const_iterator& other) const noexcept;

   private:
    friend class NoncontiguousBufferView;
    const_iterator(NoncontiguousBuffer::const_iterator cur, NoncontiguousBuffer::const_iterator end, std::size_t pos,
                   std::size_t distance) noexcept
        : cur_(cur), end_(end), pos_(pos), distance_(distance) {}

   private:
    // the block of the NoncontiguousBuffer that the current position is in
    NoncontiguousBuffer::const_iterator cur_;
    // the end block of the NoncontiguousBuffer
    NoncontiguousBuffer::const_iterator end_;
    // the byte position in the "cur_" block
    std::size_t pos_{0};
    // the distance from the starting point
    std::size_t distance_{0};
  };

  explicit NoncontiguousBufferView(const NoncontiguousBuffer& buf)
      : begin_(buf.begin()), end_(buf.end()), byte_size_(buf.ByteSize()) {}

  const_iterator begin() const noexcept { return const_iterator(begin_, end_, 0, 0); }

  const_iterator end() const noexcept { return const_iterator(end_, end_, 0, byte_size_); }

  /// @brief Gets a string of length "len" starting from "iter".
  /// @param iter the const_iterator of NoncontiguousBufferView
  /// @param len the length to get
  /// @return the result string
  std::string GetString(const const_iterator& iter, std::size_t len);

 private:
  NoncontiguousBuffer::const_iterator begin_;
  NoncontiguousBuffer::const_iterator end_;
  std::size_t byte_size_{0};
};

}  // namespace trpc::detail
