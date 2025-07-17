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

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "trpc/util/internal/never_destroyed.h"
#include "trpc/util/likely.h"
#include "trpc/util/object_pool/util.h"

namespace trpc::object_pool::global {

/// The default maximum number of objects in the pool
static std::size_t kDefaultMaxObjectNum = 16384;

/// @brief Data statistics for object creation and release within the object pool.
struct alignas(64) Statistics {
  size_t get_from_free_slots{0};         // the total number of 'slots' obtained from the 'free_slots' in 'LocalPool'
  size_t pop_free_slots_from_global{0};  // the total number of 'free_list' removed from the 'GlobalPool'
  size_t get_from_block{0};              // the total number of 'slot' obtained from blocks in 'LocalPool'
  size_t pop_block_from_global{0};       // the total number of 'blocks' removed from the 'GlobalPool'
  size_t push_free_slots_to_global{0};   // the total number of 'free_list' added to the 'GlobalPool'
  size_t global_new_block_chunk{0};      // the total number of allocated 'block_chunk' in 'GlobalPool'
  size_t slot_from_system{0};            // the total number of allocated 'slot' by 'aligned_alloc'
};

/// @brief Get thread-level data statistics related to the object pool.
template <typename T>
Statistics& GetTlsStatistics() {
  thread_local Statistics statistics;
  return statistics;
}

namespace detail {

constexpr std::size_t kBlockSize = 32;                             // the number of 'slots' in a 'Block'
constexpr std::size_t kBlockChunkSize = 32;                        // the number of 'blocks' in a 'BlockChunk'
constexpr std::size_t kFreeSlotsSize = 32;                         // the length of the 'free_list'
constexpr std::size_t kGlobalPoolNum = 1;                          // the number of the 'GlobalPool'
constexpr std::size_t kChunkSlots = kBlockSize * kBlockChunkSize;  // the number of 'slots' in a 'BlockChunk'

/// @brief The objects in the object pool that contains a concrete type T internally.
template <typename T>
struct alignas(128) Slot {
  T val;                            // object with concrete type T 
  Slot* next{nullptr};              // the next pointer of 'slot'
  bool need_free_to_system{false};  // whether it needs to be returned to the system
};

/// @brief A Class contains multiple Slots
template <typename T>
struct alignas(128) Block {
  Slot<T> slots[kBlockSize];  // array of Slot
  size_t idx{0};              // The current index of the Slot being used.
};

/// @brief A Class contains multiple Blocks
template <typename T>
struct alignas(128) BlockChunk {
  Block<T> blocks[kBlockChunkSize];  // array of Block
  size_t idx{0};                     // The current index of the Block being used.
};

/// @brief The linked list that stores free objects
template <typename T>
struct FreeSlots {
  Slot<T>* head{nullptr};
  size_t length{0};
};

/// @brief A global resource pool that shared by multiple threads. 
///        There can be multiple resource pools, with the default number being 1.
template <typename T>
class alignas(64) GlobalPool {
 public:
  /// @brief Management class of FreeSlots
  struct FreeSlotsManager {
    size_t free_num{0};                        // the number of FreeSlots
    std::vector<FreeSlots<T>> free_slots_vec;  // container used to store FreeSlots
  };

  struct FreeDeleter {
    void operator()(BlockChunk<T>* p) { free(p); }
  };

  /// @brief Management class of Block
  struct BlockManager {
    std::vector<std::unique_ptr<BlockChunk<T>, FreeDeleter>> block_chunks;
  };

 public:
  GlobalPool() noexcept {
    if constexpr (TRPC_HAS_CLASS_MEMBER(ObjectPoolTraits<T>, kMaxObjectNum)) {
      max_slot_num_ = std::max<size_t>(ObjectPoolTraits<T>::kMaxObjectNum, kChunkSlots);
    }
    // Allocate a BlockChunk during initialization.
    if (!NewBlockChunk()) {
      exit(-1);
    }

    free_slots_manager_.free_slots_vec.resize(1024);
  }

  /// @brief Reclaim FreeSlots to FreeSlotsManager
  /// @param [in] free_slots FreeSlot need reclaimed
  void PushFreeSlots(FreeSlots<T>& free_slots) noexcept;

  /// @brief Get FreeSlots from FreeSlotsManager
  /// @param[out] free_slots The acquired FreeSlots
  /// @return Return true on success, otherwise return false
  bool PopFreeSlots(FreeSlots<T>& free_slots) noexcept;

  /// @brief Get Block from BlockChunk
  /// @note Return nullptr if allocate BlockChunk failed
  Block<T>* PopBlock() noexcept;

 private:
  // Batch request Blocks from the system
  bool NewBlockChunk() noexcept;

 private:
  BlockManager block_manager_;
  std::mutex block_mtx_;
  FreeSlotsManager free_slots_manager_;
  std::mutex free_slots_mtx_;
  // maximum number limit for Slot
  size_t max_slot_num_{kDefaultMaxObjectNum};
  // the current number of Slot being used
  size_t current_slot_num_{0};
};

template <typename T>
void GlobalPool<T>::PushFreeSlots(FreeSlots<T>& free_slots) noexcept {
  ++GetTlsStatistics<T>().push_free_slots_to_global;

  {
    std::unique_lock lock(free_slots_mtx_);
    if (free_slots_manager_.free_num >= free_slots_manager_.free_slots_vec.size()) {
      free_slots_manager_.free_slots_vec.resize(free_slots_manager_.free_slots_vec.size() + 1000);
    }

    free_slots_manager_.free_slots_vec[free_slots_manager_.free_num++] = free_slots;
  }

  // reset free_slots variable
  free_slots.head = nullptr;
  free_slots.length = 0;
}

template <typename T>
bool GlobalPool<T>::PopFreeSlots(FreeSlots<T>& free_slots) noexcept {
  std::unique_lock lock(free_slots_mtx_);
  if (free_slots_manager_.free_num > 0) {
    // get one free slot from 'free_slots_manager_'
    free_slots = free_slots_manager_.free_slots_vec[--free_slots_manager_.free_num];
    lock.unlock();
    return true;
  }

  return false;
}

template <typename T>
bool GlobalPool<T>::NewBlockChunk() noexcept {
  if (TRPC_UNLIKELY(current_slot_num_ >= max_slot_num_)) {
    return false;
  }

  ++GetTlsStatistics<T>().global_new_block_chunk;
  auto* new_block_chunk =
      reinterpret_cast<BlockChunk<T>*>(aligned_alloc(alignof(BlockChunk<T>), sizeof(BlockChunk<T>)));
  if (TRPC_UNLIKELY(!new_block_chunk)) {
    return false;
  }

  new_block_chunk->idx = 0;
  for (auto&& p : new_block_chunk->blocks) {
    p.idx = 0;
  }

  current_slot_num_ += kChunkSlots;

  // Add the newly allocated BlockChunk to the 'block_manager_'.
  block_manager_.block_chunks.template emplace_back(new_block_chunk);
  return true;
}

template <typename T>
Block<T>* GlobalPool<T>::PopBlock() noexcept {
  std::unique_lock lock(block_mtx_);
  auto* block_chunk = block_manager_.block_chunks.back().get();

  // there are available Blocks
  if (TRPC_LIKELY(block_chunk && block_chunk->idx < kBlockChunkSize)) {
    auto res_idx = block_chunk->idx++;
    lock.unlock();
    ++GetTlsStatistics<T>().pop_block_from_global;
    return &block_chunk->blocks[res_idx];
  }

  // If the current BlockChunk is exhausted, allocate a new one.
  if (TRPC_LIKELY(NewBlockChunk())) {
    block_chunk = block_manager_.block_chunks.back().get();
    auto res_idx = block_chunk->idx++;
    lock.unlock();
    ++GetTlsStatistics<T>().pop_block_from_global;
    return &block_chunk->blocks[res_idx];
  }

  return nullptr;
}

/// @brief Local resource pool of thread
template <typename T>
class alignas(64) LocalPool {
 public:
  explicit LocalPool(GlobalPool<T>* global_pool) noexcept;
  ~LocalPool();

  T* New() noexcept;

  void Delete(T* obj) noexcept;

 private:
  GlobalPool<T>* global_pool_;

  Block<T>* block_{nullptr};
  FreeSlots<T> free_slots_;
};

template <typename T>
LocalPool<T>::LocalPool(GlobalPool<T>* global_pool) noexcept : global_pool_(global_pool) {}

template <typename T>
LocalPool<T>::~LocalPool() {
  while (block_ && block_->idx < kBlockSize) {
    if (TRPC_UNLIKELY(free_slots_.length == kFreeSlotsSize)) {
      global_pool_->PushFreeSlots(free_slots_);
    }
    auto* slot = &block_->slots[block_->idx++];
    slot->next = free_slots_.head;
    free_slots_.head = slot;
    ++free_slots_.length;
  }

  if (free_slots_.length > 0) {
    global_pool_->PushFreeSlots(free_slots_);
  }
}

template <typename T>
T* LocalPool<T>::New() noexcept {
  Slot<T>* slot = nullptr;

  if (TRPC_LIKELY(free_slots_.head != nullptr)) {
    ++GetTlsStatistics<T>().get_from_free_slots;
    slot = free_slots_.head;
    free_slots_.head = slot->next;
    --free_slots_.length;
    slot->need_free_to_system = false;
    return reinterpret_cast<T*>(slot);
  } else if (global_pool_->PopFreeSlots(free_slots_)) {
    ++GetTlsStatistics<T>().pop_free_slots_from_global;
    slot = free_slots_.head;
    free_slots_.head = slot->next;
    --free_slots_.length;
    slot->need_free_to_system = false;
    return reinterpret_cast<T*>(slot);
  } else if (block_ && block_->idx < kBlockSize) {
    ++GetTlsStatistics<T>().get_from_block;
    slot = &block_->slots[block_->idx++];
    slot->need_free_to_system = false;
    return reinterpret_cast<T*>(slot);
  }

  block_ = global_pool_->PopBlock();
  if (TRPC_LIKELY(block_)) {
    slot = &block_->slots[block_->idx++];
    slot->need_free_to_system = false;
    return reinterpret_cast<T*>(slot);
  }

  // allocate from pool failed, try to get object from system
  slot = static_cast<Slot<T>*>(aligned_alloc(alignof(Slot<T>), sizeof(Slot<T>)));
  if (TRPC_LIKELY(slot)) {
    slot->need_free_to_system = true;
    ++GetTlsStatistics<T>().slot_from_system;
    return reinterpret_cast<T*>(slot);
  }

  return nullptr;
}

template <typename T>
void LocalPool<T>::Delete(T* obj) noexcept {
  auto* slot = reinterpret_cast<Slot<T>*>(obj);

  if (TRPC_UNLIKELY(slot->need_free_to_system == true)) {
    slot->need_free_to_system = false;
    --GetTlsStatistics<T>().slot_from_system;
    free(slot);
    return;
  }
  // If the length of the free list in LocalPool reaches kFreeSlotsSize, recycle the free list to GlobalPool.
  if (TRPC_UNLIKELY(free_slots_.length == kFreeSlotsSize)) {
    global_pool_->PushFreeSlots(free_slots_);
  }

  slot->next = free_slots_.head;
  free_slots_.head = slot;
  ++free_slots_.length;
}

template <typename T>
LocalPool<T>* GetLocalPoolSlow() noexcept {
  // GlobalPool is not released and will end with the process (the destruction order of 'static' in multiple
  // compilation units is not guaranteed). 'NeverDestroyed' is used to prevent 'asan' from reporting leaks.
  static trpc::internal::NeverDestroyed<std::unique_ptr<GlobalPool<T>[]>> global_pool {
    std::make_unique<GlobalPool<T>[]>(kGlobalPoolNum)
  };
  static std::atomic<uint32_t> pool_idx = 0;
  // When accessing the object pool for the first time, create a LocalPool and use the Round-Robin algorithm to
  // assign a GlobalPool to it.
  thread_local std::unique_ptr<LocalPool<T>> local_pool = std::make_unique<LocalPool<T>>(
      &global_pool.GetReference()[(pool_idx.fetch_add(1, std::memory_order_relaxed)) % kGlobalPoolNum]);
  return local_pool.get();
}

template <typename T>
inline LocalPool<T>* GetLocalPool() noexcept {
  thread_local LocalPool<T>* local_pool = GetLocalPoolSlow<T>();
  return local_pool;
}

}  // namespace detail

template <typename T>
T* New() noexcept {
  auto* local_pool = detail::GetLocalPool<T>();
  if (TRPC_LIKELY(local_pool)) {
    return local_pool->New();
  }
  return nullptr;
}

template <typename T>
void Delete(T* obj) noexcept {
  auto* local_pool = detail::GetLocalPool<T>();
  if (TRPC_LIKELY(local_pool)) {
    local_pool->Delete(obj);
    return;
  }
  free(obj);
}

}  // namespace trpc::object_pool::global
