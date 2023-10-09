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

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>

namespace trpc::container {

/// @brief Memory pool with fixed size, not thread safe.
///        1.When initailized:
///        Whole continous memory block consist of n small blocks, in which avail_head_index_
///        ponits to the index of first available block, index of second available block resides
///        in header of first available block, index of third available block resides in header of
///        second block, and so on.
///        2.When allocated:
///        Get available block through avail_head_index_, which been reset to index of next available
///        block
///        3.When released:
///        avail_head_index_ is set to index of released block.
class FixedMemoryPool {
 public:
  /// @brief Constructor.
  explicit FixedMemoryPool(size_t item_num, size_t item_mem_size) noexcept : item_num_(item_num) {
    item_mem_size_ = std::max(item_mem_size, sizeof(size_t));
    size_t mod_size = item_mem_size_ % sizeof(size_t);
    item_mem_size_ = (mod_size > 0) ? (item_mem_size_ - mod_size + sizeof(size_t)) : item_mem_size_;

    void* mem_ptr = ::aligned_alloc(64, item_mem_size_ * item_num_);
    assert(mem_ptr != nullptr);

    unsigned char* ptr = static_cast<unsigned char*>(mem_ptr);
    for (uint32_t i = 0; i != item_num_; ptr += item_mem_size_) {
      ++i;
      memcpy(ptr, &i, sizeof(uint32_t));
    }

    mem_ptr_ = static_cast<unsigned char*>(mem_ptr);
    avail_head_index_ = 0;
    avail_num_ = item_num_;
  }

  /// @brief Destructor.
  ~FixedMemoryPool() {
    assert(mem_ptr_ != nullptr);
    ::free(static_cast<void*>(mem_ptr_));
  }

  /// @brief Allocate a block.
  bool Allocate(void** mem_ptr) noexcept {
    if (avail_num_ == 0) {
      return false;
    }

    unsigned char* ptr = mem_ptr_ + (avail_head_index_ * item_mem_size_);

    --avail_num_;

    avail_head_index_ = *(reinterpret_cast<uint32_t *>(ptr));

    memset(ptr, 0, item_mem_size_);

    *mem_ptr = ptr;

    return true;
  }

  /// @brief Release a block.
  bool Deallocate(void* mem_ptr) noexcept {
    unsigned char* prelease = static_cast<unsigned char*>(mem_ptr);
    if (prelease < mem_ptr_ || prelease > (mem_ptr_ + item_mem_size_ * item_num_)) {
      // Not allocated from current pool.
      return false;
    }

    if (avail_num_ >= item_num_) {
      assert(false);
      return false;
    }

    assert((prelease - mem_ptr_) % item_mem_size_ == 0);

    *(reinterpret_cast<uint32_t *>(prelease)) = avail_head_index_;

    avail_head_index_ = static_cast<uint32_t>((prelease - mem_ptr_) / item_mem_size_);

    ++avail_num_;

    return true;
  }

 private:
  FixedMemoryPool() = delete;

 private:
  unsigned char* mem_ptr_{nullptr};

  uint32_t item_num_{0};

  uint32_t avail_head_index_{0};

  uint32_t avail_num_{0};

  uint32_t item_mem_size_{0};
};

/// @brief Pre-allocated fixed-size memory pool, use std::allocator if size exceed kCacheItemSize.
/// @note  Used for containers which use link to store data, such as std::list/std::unordered_map/
///        std::mulitmap, to avoid small memory fragments.
template <typename T, std::size_t kCacheItemSize>
class FixedArenaAllocator : public std::allocator<T> {
 public:
  FixedArenaAllocator() : fixed_pool_(new FixedMemoryPool(kCacheItemSize, sizeof(T))) {}

  /// @brief Copy constructor.
  template <typename U, std::size_t UNum>
  FixedArenaAllocator(const FixedArenaAllocator<U, UNum>& rhs) {}

  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  typedef T value_type;

  template <typename Other>
  struct rebind {
    typedef FixedArenaAllocator<Other, kCacheItemSize> other;
  };

  pointer address(reference r) const noexcept { return std::addressof(r); }
  const_pointer address(const_reference r) const noexcept { return std::addressof(r); }

  /// @brief Allocate new memory.
  pointer allocate(size_type n, const void* = 0) {
    pointer p = nullptr;
    if (fixed_pool_) {
      assert(n == 1 && "data allocate n mast be 1");
      void* stack_ptr = nullptr;
      if (fixed_pool_->Allocate(&stack_ptr)) {
        p = static_cast<pointer>(stack_ptr);
      } else {
        p = std::allocator<T>::allocate(1);
      }
    } else {
      p = std::allocator<T>::allocate(n);
    }

    assert(p && "allocate failed");
    return p;
  }

  /// @brief Release memory.
  void deallocate(pointer p, size_type n) {
    if (fixed_pool_) {
      assert(n == 1 && "data deallocate n mast be 1");
      if (!fixed_pool_->Deallocate(p)) {
        std::allocator<T>::deallocate(p, 1);
      }
    } else {
      std::allocator<T>::deallocate(p, n);
    }
  }

  size_type max_size() const noexcept {
    return std::numeric_limits<size_type>::max();
  }

 private:
  std::unique_ptr<FixedMemoryPool> fixed_pool_;
};

}  // namespace trpc::container
