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

#include "trpc/util/buffer/noncontiguous_buffer.h"

#include <algorithm>
#include <utility>

namespace trpc {

namespace detail {

// Boyer-Moore string-search algorithm: https://en.wikipedia.org/wiki/Boyer%E2%80%93Moore_string-search_algorithm
template <size_t Len>
void MakeDelta1(std::array<ptrdiff_t, Len>& delta1, std::basic_string_view<uint8_t> pattern) {
  delta1.fill(pattern.length());
  for (size_t i = 0; i < pattern.length(); i++) {
    delta1[pattern[i]] = pattern.length() - 1 - i;
  }
}

// true if the suffix of word starting from word[pos] is a prefix
// of word
bool IsPrefix(std::basic_string_view<uint8_t> word, ptrdiff_t pos) {
  return !strncmp(reinterpret_cast<const char*>(&word[0]), reinterpret_cast<const char*>(&word[pos]),
                  word.length() - pos);
}

// length of the longest suffix of word ending on word[pos].
// SuffixLength("dddbcabc", 8, 4) = 2
ptrdiff_t SuffixLength(std::basic_string_view<uint8_t> word, ptrdiff_t pos) {
  ptrdiff_t i;
  // increment suffix length i to the first mismatch or beginning
  // of the word
  for (i = 0; (word[pos - i] == word[word.length() - 1 - i]) && (i <= pos); i++) {
  }
  return i;
}

// GOOD SUFFIX RULE.
// delta2 table: given a mismatch at pat[pos], we want to align
// with the next possible full match could be based on what we
// know about pat[pos+1] to pat[patlen-1].
//
// In case 1:
// pat[pos+1] to pat[patlen-1] does not occur elsewhere in pat,
// the next plausible match starts at or after the mismatch.
// If, within the substring pat[pos+1 .. patlen-1], lies a prefix
// of pat, the next plausible match is here (if there are multiple
// prefixes in the substring, pick the longest). Otherwise, the
// next plausible match starts past the character aligned with
// pat[patlen-1].
//
// In case 2:
// pat[pos+1] to pat[patlen-1] does occur elsewhere in pat. The
// mismatch tells us that we are not looking at the end of a match.
// We may, however, be looking at the middle of a match.
//
// The first loop, which takes care of case 1, is analogous to
// the KMP table, adapted for a 'backwards' scan order with the
// additional restriction that the substrings it considers as
// potential prefixes are all suffixes. In the worst case scenario
// pat consists of the same letter repeated, so every suffix is
// a prefix. This loop alone is not sufficient, however:
// Suppose that pat is "ABYXCDBYX", and text is ".....ABYXCDEYX".
// We will match X, Y, and find B != E. There is no prefix of pat
// in the suffix "YX", so the first loop tells us to skip forward
// by 9 characters.
// Although superficially similar to the KMP table, the KMP table
// relies on information about the beginning of the partial match
// that the BM algorithm does not have.
//
// The second loop addresses case 2. Since SuffixLength may not be
// unique, we want to take the minimum value, which will tell us
// how far away the closest potential match is.
void MakeDelta2(std::unique_ptr<ptrdiff_t[]>& delta2, std::basic_string_view<uint8_t> pattern) {
  ptrdiff_t pat_len_minus_one = pattern.length() - 1;
  ptrdiff_t last_prefix_index = pat_len_minus_one;

  // first loop
  for (ptrdiff_t p = pat_len_minus_one; p >= 0; p--) {
    if (IsPrefix(pattern, p + 1)) {
      last_prefix_index = p + 1;
    }
    delta2[p] = last_prefix_index + (pat_len_minus_one - p);
  }

  // second loop
  for (ptrdiff_t p = 0; p < pat_len_minus_one; p++) {
    auto slen = SuffixLength(pattern, p);
    if (pattern[p - slen] != pattern[pat_len_minus_one - slen]) {
      delta2[pat_len_minus_one - slen] = pat_len_minus_one - p + slen;
    }
  }
}

std::vector<std::pair<size_t, trpc::BufferView>>::const_iterator BinarySearch(
    const std::vector<std::pair<size_t, trpc::BufferView>>& buffers, size_t val,
    std::vector<std::pair<size_t, trpc::BufferView>>::const_iterator hint) {
  if (hint->first <= val && val < hint->first + hint->second.size()) {
    return hint;
  }
  return std::lower_bound(buffers.begin(), --buffers.end(), val,
                          [](const auto& entry, size_t value) { return entry.first + entry.second.size() <= value; });
}

// The following source codes are from flare.
// Copied and modified from
// https://github.com/Tencent/flare/blob/master/flare/base/buffer.cc.
void FlattenToSlowSlow(const NoncontiguousBuffer& nb, void* buffer, std::size_t size) {
  TRPC_CHECK(nb.ByteSize() >= size, "No enough data.");
  std::size_t copied = 0;
  for (auto iter = nb.begin(); iter != nb.end() && copied != size; ++iter) {
    auto len = std::min(size - copied, iter->size());
    memcpy(reinterpret_cast<char*>(buffer) + copied, iter->data(), len);
    copied += len;
  }
}

}  // namespace detail

BufferBuilder::BufferBuilder() { AllocateBuffer(); }

void BufferBuilder::AllocateBuffer() {
  used_ = 0;
  current_ = MakeBlockRef(memory_pool::Allocate());
}

NoncontiguousBoyerMooreSearcher::NoncontiguousBoyerMooreSearcher(std::string_view pattern)
    : pattern_(pattern), delta2(std::make_unique<ptrdiff_t[]>(pattern.length())) {
  detail::MakeDelta1(delta1, {reinterpret_cast<const uint8_t*>(pattern.data()), pattern.length()});
  detail::MakeDelta2(delta2, {reinterpret_cast<const uint8_t*>(pattern.data()), pattern.length()});
}

size_t NoncontiguousBoyerMooreSearcher::operator()(const NoncontiguousBuffer& buffer) const noexcept {
  // The empty pattern must be considered specially
  if (pattern_.empty()) {
    return 0;
  }

  std::vector<std::pair<size_t, trpc::BufferView>> buffers;
  buffers.reserve(buffer.size());
  size_t accumulate_size = 0;
  for (const auto& block : buffer) {
    buffers.emplace_back(accumulate_size, trpc::BufferView{block.data(), block.size()});
    accumulate_size += block.size();
  }
  size_t i = pattern_.length() - 1;  // str-idx
  auto iter = buffers.cbegin();
  while (i < buffer.ByteSize()) {
    ptrdiff_t j = pattern_.length() - 1;  // pat-idx
    iter = detail::BinarySearch(buffers, i, iter);
    while (j >= 0 && iter->second.data()[i - iter->first] == pattern_[j]) {
      if (--i < iter->first) {
        --iter;
      }
      --j;
    }
    if (j < 0) {
      return i + 1;
    }

    i += std::max(delta1[static_cast<uint8_t>(iter->second.data()[i - iter->first])], delta2[j]);
  }
  return NoncontiguousBuffer::npos;
}

NoncontiguousBuffer::NoncontiguousBuffer(const NoncontiguousBuffer& nb) : byte_size_(nb.byte_size_) {
  for (auto&& e : nb.buffers_) {
    auto* b = object_pool::MakeLwUnique<BufferBlock>().Leak();
    *b = e;
    buffers_.push_back(b);
  }
  is_contiguous_ = nb.is_contiguous_;
}

NoncontiguousBuffer& NoncontiguousBuffer::operator=(const NoncontiguousBuffer& nb) {
  if (TRPC_UNLIKELY(&nb == this)) {
    return *this;
  }
  Clear();
  byte_size_ = nb.byte_size_;
  for (auto&& e : nb.buffers_) {
    auto* b = object_pool::MakeLwUnique<BufferBlock>().Leak();
    *b = e;
    buffers_.push_back(b);
  }
  is_contiguous_ = nb.is_contiguous_;
  return *this;
}

void NoncontiguousBuffer::SkipSlow(std::size_t bytes) noexcept {
  byte_size_ -= bytes;

  while (bytes) {
    auto&& first = buffers_.front();
    auto os = std::min(bytes, first.size());
    if (os == first.size()) {
      auto* block = buffers_.pop_front();
      block->Clear();
      object_pool::Delete<BufferBlock>(block);
    } else {
      TRPC_DCHECK_LT(os, first.size());
      first.Skip(os);
    }
    bytes -= os;
  }
}

void NoncontiguousBuffer::ClearSlow() noexcept {
  byte_size_ = 0;
  while (!buffers_.empty()) {
    BufferBlock* block = buffers_.pop_front();
    block->Clear();
    object_pool::Delete<BufferBlock>(block);
  }
}

void NoncontiguousBufferBuilder::InitializeNextBlock() {
  if (current_) {
    TRPC_CHECK(SizeAvailable());
    return;
  }
  current_ = MakeBlockRef(memory_pool::Allocate());

  used_ = 0;
}

void NoncontiguousBufferBuilder::FlushCurrentBlock() {
  if (!used_) {
    // The current block is clean and does not need to be flushed.
    return;
  }
  auto b = object_pool::MakeLwUnique<BufferBlock>();
  b->Reset(0, used_, std::move(current_));
  nb_.Append(std::move(b));
  used_ = 0;
}

void NoncontiguousBufferBuilder::AppendSlow(const void* ptr, std::size_t length) {
  while (length) {
    auto copying = std::min(length, SizeAvailable());
    memcpy(data(), ptr, copying);
    MarkWritten(copying);
    ptr = static_cast<const char*>(ptr) + copying;
    length -= copying;
  }
}

object_pool::LwUniquePtr<BufferBlock> CreateBufferBlockSlow(std::string_view s) {
  BufferBuilder bb;
  std::size_t copied = 0;
  TRPC_CHECK_LE(s.size(), bb.SizeAvailable(),
                "Data is too large to be copied in a single `Buffer`. Use "
                "`NoncontiguousBuffer` instead.");
  auto len = std::min(s.size() - copied, bb.SizeAvailable());
  memcpy(bb.data(), s.data() + copied, len);
  return bb.Seal(len);
}

object_pool::LwUniquePtr<BufferBlock> CreateBufferBlockSlow(const void* ptr, std::size_t size) {
  return CreateBufferBlockSlow(std::string_view(reinterpret_cast<const char*>(ptr), size));
}

NoncontiguousBuffer CreateBufferSlow(std::string_view s) {
  NoncontiguousBufferBuilder nbb;
  nbb.Append(s);
  return nbb.DestructiveGet();
}

NoncontiguousBuffer CreateBufferSlow(const void* ptr, std::size_t size) {
  NoncontiguousBufferBuilder nbb;
  nbb.Append(ptr, size);
  return nbb.DestructiveGet();
}

std::string FlattenSlow(const NoncontiguousBuffer& nb, std::size_t max_bytes) {
  max_bytes = std::min(max_bytes, nb.ByteSize());
  std::string rc;
  std::size_t left = max_bytes;
  rc.reserve(max_bytes);
  for (auto iter = nb.begin(); iter != nb.end() && left; ++iter) {
    auto len = std::min(left, iter->size());
    rc.append(iter->data(), len);
    left -= len;
  }
  return rc;
}

std::string FlattenSlowUntil(const NoncontiguousBuffer& nb, std::string_view delim, std::size_t max_bytes) {
  if (nb.Empty()) {
    return {};
  }

  // The caller should be aware that this method should not be used to operate on large amounts of data 
  // as it may be slow. To optimize performance, copying is not used.
  std::string_view current(nb.FirstContiguous().data(), nb.FirstContiguous().size());
  if (auto pos = current.find(delim); pos != std::string_view::npos) {
    auto expected_bytes = std::min(pos + delim.size(), max_bytes);
    return {nb.FirstContiguous().data(), nb.FirstContiguous().data() + expected_bytes};
  }

  // The requested data was not found in the current block, so the following logic may be slow.
  std::string rc;
  for (auto iter = nb.begin(); iter != nb.end() && rc.size() < max_bytes; ++iter) {
    auto old_len = rc.size();
    rc.append(iter->data(), iter->size());
    if (auto pos = rc.find(delim, old_len - std::min(old_len, delim.size())); pos != std::string::npos) {
      rc.erase(rc.begin() + pos + delim.size(), rc.end());
      break;
    }
  }
  if (rc.size() > max_bytes) {
    rc.erase(rc.begin() + max_bytes, rc.end());
  }
  return rc;
}

}  // namespace trpc
