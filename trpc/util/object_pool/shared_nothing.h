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

#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <vector>

#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/object_pool/util.h"

namespace trpc::object_pool::shared_nothing {

/// The default maximum number of objects in the pool.
static size_t kDefaultMaxObjectNum = 4096;

/// @brief Data statistics for object creation and release within the object pool.
///        One CPU represents one thread.
struct alignas(64) Statistics {
  size_t allocs_num{0};             // Total number of object allocations in the current thread's object pool.
  size_t frees_num{0};              // Total number of object releases in the current thread's object pool.
  size_t cross_cpu_frees_num{0};    // Number of objects reclaimed from the idle list across CPUs.
  size_t foreign_frees_num{0};      // Number of times objects were reclaimed to the idle list  across CPUs.
  size_t slot_chunks_alloc_num{0};  // Number of times the current thread's object pool executed a 'chunk' allocation.
  size_t slot_from_system{0};       // Number of times a 'slot' was allocated from the system.
};

/// @brief Get thread-level data statistics related to the object pool.
template <typename T>
Statistics& GetTlsStatistics() {
  thread_local Statistics statistics;
  return statistics;
}

namespace detail {

static constexpr uint32_t kMaxCpus = 1024;             // The maximum number of allowed threads.
static constexpr uint32_t kInvalidCpuId = 0xffffffff;  // Invalid CPU ID.
static constexpr uint32_t kInvalidSlotChunkId = 0;     // Invalid SlotChunk ID, valid IDs should > 0.
static constexpr uint32_t kPageSize = 4096;            // Memory page size.
static constexpr uint32_t kMinChunkSize = 8;           // The minimum number of objects contained in a chunk.

template <typename T>
class SharedNothingPoolImp;

/// @brief The global management class for 'shared-nothing' object pool.
template <typename T>
struct SharedNothingPool {
  static SharedNothingPoolImp<T>* all_cpus[kMaxCpus];
};

template <typename T>
SharedNothingPoolImp<T>* SharedNothingPool<T>::all_cpus[kMaxCpus];

uint32_t GetCpuId();

/// @brief The objects in the object pool that contains a concrete type T internally.
template <typename T>
struct alignas(64) Slot {
  T val;                                   // object with concrete type T
  Slot* next{nullptr};                     // pointer of the next Slot
  bool need_free_to_system{false};         // whether it needs to be returned to the system
  uint32_t cpu_id{kInvalidCpuId};          // the CPU ID of the current thread's counter (not the system CPU core ID)
  uint32_t chunk_id{kInvalidSlotChunkId};  // the chunk ID corresponding to this Slot
};

/// @brief The linked list that stores free objects.
template <typename T>
struct FreeSlots {
  Slot<T>* head{nullptr};
  Slot<T>* tail{nullptr};
  size_t length{0};
};

/// @brief The linked list of SlotChunk objects.
struct ChunkListLink {
  uint32_t next{0};  // the next available chunk
};

/// @brief Class used for managing the slot objects in the same chunk block.
template <typename T>
struct SlotChunk {
  void* chunk_addr;       // The starting address of the chunk block, used for releasing memory when the thread exits.
  ChunkListLink link;     // The next available SlotChunk.
  FreeSlots<T> freeslot;  // The linked list of free objects.
};

/// @brief The management of SlotChunk objects, that using for reclaiming objects allocated from a contiguous block of
///        memory to the same SlotChunk, in order to improve memory contiguity reduce TLB misses.
template <typename T>
class SlotChunkManager {
 public:
  /// @brief Get the first SlotChunk object currently available
  SlotChunk<T>& Front() { return slot_chunks_[front_]; }

  /// @brief Check if the first SlotChunk object is empty
  /// @return If empty, return true. Otherwise, return false.
  bool Empty() { return !front_; }

  /// @brief Get SlotChunk object by index
  SlotChunk<T>& GetSlotChunk(uint32_t index) {
    assert(index <= slot_chunks_.size() && index > 0);
    return slot_chunks_[index];
  }

  /// @brief Resize the size of 'slot_chunks_' which used to store SlotChunk objects
  /// @note Used to ensure that the size of slot_chunks_ remains consistent with the allocated times of SlotChunk
  void DoResize(uint32_t size) {
    uint32_t slot_chunks_size = slot_chunks_.size();
    if (size >= slot_chunks_size) {
      uint32_t new_size = std::max(slot_chunks_size * 2, size + 100);
      slot_chunks_.resize(new_size);
    }
  }

  /// @brief Reclaim SlotChunk objects
  void PushFront(SlotChunk<T>& slot_chunk, uint32_t slot_chunk_id) {
    assert(slot_chunk_id < slot_chunks_.size());
    slot_chunk.link.next = front_;
    front_ = slot_chunk_id;
  }

  /// @brief Update the front chunk ID to the next one
  void PopFront() { front_ = slot_chunks_[front_].link.next; }

  /// @brief  Get the value of the front chunk ID
  uint32_t GetFrontChunkId() { return front_; }

 private:
  std::vector<SlotChunk<T>> slot_chunks_;  // used to store SlotChunk objects (the first element is a sentinel element)
  uint32_t front_{0};                      // used to identify the first available SlotChunk
};

/// @brief Implementation of shared-nothing pool.
template <typename T>
class alignas(64) SharedNothingPoolImp {
 public:
  SharedNothingPoolImp();

  ~SharedNothingPoolImp();

  /// @brief Allocate a new object
  T* New();

  /// @brief Deallocate an object
  void Delete(T* obj);

  /// @brief Deallocate an object across CPUs
  void DeleteCrossCpu(T* obj);

 private:
  // Reclaim free list objects across CPUs
  void DrainCrossCpuFreelist();

  // Delete an object and return its memory to the system
  bool DeleteToSystem(Slot<T>* slot);

 private:
  FreeSlots<T> freeslots_;
  SlotChunkManager<T> slot_chunk_manager_;
  // The maximum chunk ID allocated by the system, which is also the total number of allocation from system
  uint32_t chunk_id_{0};
  // The maximum number of Slot objects in freeslots_, beyond which they are reclaimed.
  uint32_t max_free_num_;
  // The target number of Slot objects in freeslots_ during allocation and reclamation.
  uint32_t goal_num_;
  // The number of slot objects allocated from the system each time.
  uint32_t chunk_size_;
  // The maximun number of Slot objects allocated.
  size_t max_slot_num_{kDefaultMaxObjectNum};
  // The cross-CPU object reclamation list.
  alignas(64) std::atomic<Slot<T>*> xcpu_freelist_{nullptr};
  // The current number of Slot objects allocated.
  static std::atomic<uint32_t> current_slot_num_;
};

template <typename T>
std::atomic<uint32_t> SharedNothingPoolImp<T>::current_slot_num_{0};

template <typename T>
SharedNothingPoolImp<T>::SharedNothingPoolImp() {
  slot_chunk_manager_.DoResize(100);
  max_free_num_ = std::max<uint32_t>(64, kPageSize * 2 / static_cast<uint32_t>(sizeof(Slot<T>)));
  uint32_t min_free_num = max_free_num_ / 2;
  goal_num_ = (max_free_num_ + min_free_num) / 2;
  chunk_size_ = std::max<uint32_t>(kMinChunkSize, kPageSize / static_cast<uint32_t>(sizeof(Slot<T>)));

  if constexpr (TRPC_HAS_CLASS_MEMBER(ObjectPoolTraits<T>, kMaxObjectNum)) {
    // Ensure that there are at least chunk_size_ objects.
    max_slot_num_ = std::max<size_t>(ObjectPoolTraits<T>::kMaxObjectNum, chunk_size_);
  }
}

template <typename T>
SharedNothingPoolImp<T>::~SharedNothingPoolImp() {
  // Reclaiming Slot objects from freeslots_ to slot_chunk_manager_
  while (freeslots_.length > 0) {
    auto* slot = freeslots_.head;
    freeslots_.head = slot->next;
    freeslots_.length--;

    SlotChunk<T>& slot_chunk = slot_chunk_manager_.GetSlotChunk(slot->chunk_id);
    if (!slot_chunk.freeslot.head) {
      slot_chunk_manager_.PushFront(slot_chunk, slot->chunk_id);
    }
    slot->next = slot_chunk.freeslot.head;
    slot_chunk.freeslot.head = slot;
    slot_chunk.freeslot.length++;
  }

  // Releasing objects in slot_chunk_manager_ in units of slotchunk.
  uint32_t free_chunk_count = 0;
  while (!slot_chunk_manager_.Empty()) {
    uint32_t chunk_id = slot_chunk_manager_.GetFrontChunkId();
    auto& slot_chunk = slot_chunk_manager_.Front();
    slot_chunk_manager_.PopFront();
    if (slot_chunk.freeslot.length == chunk_size_) {
      free_chunk_count++;
      free(slot_chunk.chunk_addr);
      current_slot_num_.fetch_sub(chunk_size_, std::memory_order::memory_order_relaxed);
    } else {
      TRPC_FMT_ERROR("Memory leak, chunk_id = {}, chunk_addr = {}", chunk_id, slot_chunk.chunk_addr);
    }
  }

  // If free_chunk_count is not equal to the total number of allocated chunks (chunk_id_), print an error message.
  if (free_chunk_count != chunk_id_) {
    TRPC_FMT_ERROR("Memory leak: alloc count = {}, free count = {}", chunk_id_, free_chunk_count);
  }
}

template <typename T>
T* SharedNothingPoolImp<T>::New() {
  DrainCrossCpuFreelist();

  if (TRPC_UNLIKELY(!freeslots_.head)) {
    // Allocate goal number of targets from slot_chunk_manager_ to freeslots_ first.
    while (!slot_chunk_manager_.Empty() && freeslots_.length < goal_num_) {
      auto& slot_chunk = slot_chunk_manager_.Front();
      slot_chunk_manager_.PopFront();

      // Add the freeslot obtained from the slot chunk directly to freeslots_.
      if (slot_chunk.freeslot.tail) {
        slot_chunk.freeslot.tail->next = freeslots_.head;
        freeslots_.head = slot_chunk.freeslot.head;
        freeslots_.length += slot_chunk.freeslot.length;
        slot_chunk.freeslot.length = 0;
        slot_chunk.freeslot.head = nullptr;
        slot_chunk.freeslot.tail = nullptr;
      }
    }

    // If slot_chunk_manager_ cannot allocate goal number of targets, then allocate from the system.
    uint32_t current_slot_num = current_slot_num_.load(std::memory_order::memory_order_relaxed);
    while (freeslots_.length < goal_num_ && current_slot_num < max_slot_num_) {
      void* chunk_addr = aligned_alloc(alignof(Slot<T>), sizeof(Slot<T>) * chunk_size_);
      if (TRPC_UNLIKELY(chunk_addr == nullptr)) {
        break;
      }

      ++chunk_id_;  // Increment the allocation count
      slot_chunk_manager_.DoResize(chunk_id_);
      slot_chunk_manager_.GetSlotChunk(chunk_id_).chunk_addr = chunk_addr;

      Slot<T>* ptr = reinterpret_cast<Slot<T>*>(chunk_addr);
      // Initialize the Slot and add the allocated Slot to freeslots_.
      for (uint32_t i = 0; i < chunk_size_; ++i, ++ptr) {
        // initialize the Slot
        ptr->chunk_id = chunk_id_;
        ptr->need_free_to_system = false;

        // add the allocated Slot to freeslots_
        ptr->next = freeslots_.head;
        freeslots_.head = ptr;
        freeslots_.length++;
      }

      // Increment the allocation count in data statistics.
      ++GetTlsStatistics<T>().slot_chunks_alloc_num;
      current_slot_num_.fetch_add(chunk_size_, std::memory_order::memory_order_relaxed);
    }
  }

  Slot<T>* slot = nullptr;

  if (TRPC_UNLIKELY(!freeslots_.head)) {
    // allocate from system
    slot = static_cast<Slot<T>*>(aligned_alloc(alignof(Slot<T>), sizeof(Slot<T>)));
    if (TRPC_LIKELY(slot)) {
      slot->need_free_to_system = true;
      ++GetTlsStatistics<T>().slot_from_system;
      return reinterpret_cast<T*>(slot);
    }
    return nullptr;
  }

  // Allocate an object from freeslots_.
  ++GetTlsStatistics<T>().allocs_num;
  slot = freeslots_.head;
  slot->need_free_to_system = false;
  freeslots_.head = slot->next;
  freeslots_.length--;

  return reinterpret_cast<T*>(slot);
}

template <typename T>
void SharedNothingPoolImp<T>::Delete(T* obj) {
  Slot<T>* slot = reinterpret_cast<Slot<T>*>(obj);
  // If the object was not allocated from the system, free it and return.
  if (DeleteToSystem(slot)) {
    return;
  }

  slot->next = freeslots_.head;
  freeslots_.head = slot;
  freeslots_.length++;

  ++GetTlsStatistics<T>().frees_num;

  // If there are too many objects in freeslots_, return them to slot_chunk_manager_.
  if (freeslots_.length > max_free_num_) {
    while (freeslots_.head && freeslots_.length > goal_num_) {
      auto* slot = freeslots_.head;
      freeslots_.head = slot->next;
      freeslots_.length--;

      // Return the Slot to slot_chunk_manager_.
      SlotChunk<T>& slot_chunk = slot_chunk_manager_.GetSlotChunk(slot->chunk_id);
      if (!slot_chunk.freeslot.tail) {
        slot_chunk_manager_.PushFront(slot_chunk, slot->chunk_id);
        slot_chunk.freeslot.head = slot;
      } else {
        slot_chunk.freeslot.tail->next = slot;
      }

      slot->next = nullptr;
      slot_chunk.freeslot.tail = slot;
      slot_chunk.freeslot.length++;
    }
  }
}

template <typename T>
void SharedNothingPoolImp<T>::DeleteCrossCpu(T* obj) {
  Slot<T>* p = reinterpret_cast<Slot<T>*>(obj);
  if (DeleteToSystem(p)) {
    return;
  }

  auto& list = xcpu_freelist_;
  auto old = list.load(std::memory_order_relaxed);
  do {
    p->next = old;
  } while (!list.compare_exchange_weak(old, p, std::memory_order_release, std::memory_order_relaxed));

  ++GetTlsStatistics<T>().foreign_frees_num;
}

template <typename T>
void SharedNothingPoolImp<T>::DrainCrossCpuFreelist() {
  if (!xcpu_freelist_.load(std::memory_order_relaxed)) {
    return;
  }

  uint64_t free_num = 0;
  auto p = xcpu_freelist_.exchange(nullptr, std::memory_order_acquire);
  while (p) {
    auto n = p->next;

    p->next = freeslots_.head;
    freeslots_.head = p;
    freeslots_.length++;

    p = n;

    ++free_num;
  }

  GetTlsStatistics<T>().cross_cpu_frees_num += free_num;
}

template <typename T>
bool SharedNothingPoolImp<T>::DeleteToSystem(Slot<T>* slot) {
  if (TRPC_UNLIKELY(slot->need_free_to_system == true)) {
    slot->need_free_to_system = false;
    free(slot);
    --GetTlsStatistics<T>().slot_from_system;
    return true;
  }
  return false;
}

template <typename T>
SharedNothingPoolImp<T>* GetSharedNothingPool(uint32_t cpu_id) {
  thread_local std::unique_ptr<SharedNothingPoolImp<T>> tls_pool = nullptr;
  if (!tls_pool) {
    tls_pool = std::make_unique<SharedNothingPoolImp<T>>();

    SharedNothingPool<T>::all_cpus[cpu_id] = tls_pool.get();
  }

  return tls_pool.get();
}

}  // namespace detail

template <typename T>
T* New() {
  uint32_t tls_cpu_id = detail::GetCpuId();
  T* ptr = detail::GetSharedNothingPool<T>(tls_cpu_id)->New();

  detail::Slot<T>* slot = reinterpret_cast<detail::Slot<T>*>(ptr);
  slot->cpu_id = tls_cpu_id;
  return ptr;
}

template <typename T>
void Delete(T* ptr) {
  detail::Slot<T>* slot = reinterpret_cast<detail::Slot<T>*>(ptr);
  uint32_t cpu_id = slot->cpu_id;
  assert(cpu_id != detail::kInvalidCpuId);

  uint32_t tls_cpu_id = detail::GetCpuId();
  if (cpu_id == tls_cpu_id) {
    detail::GetSharedNothingPool<T>(cpu_id)->Delete(ptr);
  } else {
    detail::SharedNothingPoolImp<T>* pool = detail::SharedNothingPool<T>::all_cpus[cpu_id];
    assert(pool != nullptr);

    pool->DeleteCrossCpu(ptr);
  }
}

}  // namespace trpc::object_pool::shared_nothing
